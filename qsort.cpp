#include "benchmark_common.hpp"
#include <algorithm>
#include <random>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>

/*
 * PURPOSE:
 * This benchmark tests sorting performance and branch prediction
 * by running repeated std::sort passes on arrays of integer values.
 */

uint64_t worker_qsort(int beginArrayIdx, int endArrayIdx, int arrSize, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    uint64_t checksum = 0;
    std::vector<int> arr(arrSize);

    for (int arrayIdx = beginArrayIdx; arrayIdx < endArrayIdx; ++arrayIdx)
    {
        // Seed by global array index so all modes process the same logical array set.
        std::mt19937_64 rng(123456789ULL + (uint64_t)arrayIdx * 99991ULL);

        for (int i = 0; i < arrSize; ++i)
        {
            arr[i] = (int)(rng() % 1'000'000'000ULL);
        }

        // Phase 1: sort random data.
        std::sort(arr.begin(), arr.end());

        // Phase 2: reverse and sort again to add extra sorting work.
        // This changes data order and stresses branch/data-dependent behavior.
        std::reverse(arr.begin(), arr.end());
        std::sort(arr.begin(), arr.end());

        // Phase 3: deterministic remap of current array contents, then sort once more.
        for (int i = 0; i < arrSize; ++i)
        {
            // Scramble data pseudo-randomly using a multiplicative hash (golden ratio sequence).
            // This destroys the previously sorted order and randomizes bit distribution.
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
    const int totalArrays = 16; // Total number of arrays to sort across all threads
    const int arrSize = 10'000'000; // Size of each array (10 million integers)

    std::vector<std::thread> pool;
    std::vector<uint64_t> partial(threads, 0);

    const int baseArrays = totalArrays / threads;
    const int extraArrays = totalArrays % threads;
    int startArrayIdx = 0;

    for (int i = 0; i < threads; ++i)
    {
        int cnt = baseArrays + (i < extraArrays ? 1 : 0);
        int beginArrayIdx = startArrayIdx;
        int endArrayIdx = beginArrayIdx + cnt;
        startArrayIdx = endArrayIdx;

        pool.emplace_back([&, i, beginArrayIdx, endArrayIdx]()
        {
            partial[i] = worker_qsort(beginArrayIdx, endArrayIdx, arrSize, threads);
        });
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