#include "benchmark_common.hpp"
#include <algorithm>
#include <random>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>

uint64_t worker_qsort(int tid, int arraysPerThread, int arrSize, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    std::mt19937_64 rng(123456789ULL + tid * 99991ULL);
    uint64_t checksum = 0;
    std::vector<int> arr(arrSize);

    for (int r = 0; r < arraysPerThread; ++r)
    {
        for (int i = 0; i < arrSize; ++i)
        {
            arr[i] = (int)(rng() % 1'000'000'000ULL);
        }

        // Phase 1: sort random data.
        std::sort(arr.begin(), arr.end());

        // Phase 2: reverse and sort again to add extra sorting work.
        std::reverse(arr.begin(), arr.end());
        std::sort(arr.begin(), arr.end());

        // Phase 3: deterministic remap then sort once more.
        for (int i = 0; i < arrSize; ++i)
        {
            arr[i] ^= (int)((uint64_t)(i + 1) * 2654435761ULL);
        }
        std::sort(arr.begin(), arr.end());

        checksum ^= (uint64_t)arr[0];
        checksum ^= (uint64_t)arr[arrSize / 2] << 1;
        checksum ^= (uint64_t)arr[arrSize - 1] << 2;
    }

    return checksum;
}

uint64_t run_qsort_once(int threads)
{
    const int arraysPerThread = 40;
    const int arrSize = 2'000'000;

    std::vector<std::thread> pool;
    std::vector<uint64_t> partial(threads, 0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_qsort(i, arraysPerThread, arrSize, threads); });
    }

    for (auto &th : pool)
        th.join();

    uint64_t total = 0;
    for (auto v : partial)
        total ^= v;
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
                                      { current = run_qsort_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("qsort", cfg, times, checksum);
    return 0;
}