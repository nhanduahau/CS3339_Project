#include "benchmark_common.hpp"
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>

/*
 * PURPOSE:
 * This benchmark tests integer processing performance by counting
 * the number of set bits (1s) in arrays of integers.
 */

// Brian Kernighan's algorithm for counting set bits
// It repeatedly clears the least significant set bit (x &= (x - 1))
// The loop runs exactly as many times as there are set bits in x.
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

    auto splitmix64 = [](uint64_t z) -> uint64_t
    {
        z += 0x9E3779B97F4A7C15ULL;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        z = z ^ (z >> 31);
        return z;
    };

    for (long long i = start; i < end; ++i)
    {
        // Generate value only from global iteration index i
        // so single and multi process the same logical input set.
        uint64_t x = splitmix64((uint64_t)i);

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
    const long long iterations = 5'000'000'000LL;

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