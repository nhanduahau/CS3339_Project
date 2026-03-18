#include "benchmark_common.hpp"
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>

inline int popcount64(uint64_t x)
{
    int c = 0;
    while (x)
    {
        x &= (x - 1);
        ++c;
    }
    return c;
}

uint64_t worker_bitcount(int tid, int threads, long long iterations)
{
    long long chunk = iterations / threads;
    long long start = tid * chunk;
    long long end = (tid == threads - 1) ? iterations : start + chunk;

    if (threads == 1)
        pin_current_thread_to_core0();

    uint64_t total = 0;
    uint64_t seed = 0x9E3779B97F4A7C15ULL ^ (uint64_t)tid;

    for (long long i = start; i < end; ++i)
    {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;

        uint64_t x = seed ^ ((uint64_t)i * 0xD6E8FEB86659FD93ULL);

        // Do several dependent bit-mixing rounds to increase per-iteration complexity.
        for (int round = 0; round < 8; ++round)
        {
            uint64_t a = x ^ (x >> 13) ^ (x << 11);
            uint64_t b = ~a;
            uint64_t c = a ^ (a >> 17) ^ (a << 29);
            uint64_t d = (c * 0x9E3779B97F4A7C15ULL) ^ (c >> 23);

            total += popcount64(a);
            total += popcount64(b);
            total += popcount64(c);
            total += popcount64(d);

            x = d ^ (d << 7) ^ (d >> 3);
        }
    }

    return total;
}

uint64_t run_bitcount_once(int threads)
{
    const long long iterations = 200'000'000LL;

    std::vector<std::thread> pool;
    std::vector<uint64_t> partial(threads, 0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_bitcount(i, threads, iterations); });
    }

    for (auto &th : pool)
        th.join();

    uint64_t total = 0;
    for (auto v : partial)
        total += v;
    return total;
}

int main(int argc, char *argv[])
{
    BenchmarkConfig cfg;
    try
    {
        cfg = parse_config_from_argv(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::vector<double> times;
    uint64_t checksum = 0;

    for (int r = 0; r < cfg.runs; ++r)
    {
        uint64_t current = 0;
        double elapsed = measure_once([&]()
                                      { current = run_bitcount_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("bitcount", cfg, times, checksum);
    return 0;
}