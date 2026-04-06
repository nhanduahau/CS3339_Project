#include "benchmark_common.hpp"
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <iostream>

/*
 * PURPOSE:
 * This benchmark tests string manipulation and cache performance
 * by repeatedly searching for sub-strings within a large text buffer.
 */

long long worker_stringsearch(const std::string &text,
                              const std::vector<std::string> &patterns,
                              int repeats,
                              int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    long long found = 0;

    for (int r = 0; r < repeats; ++r)
    {
        for (const auto &pat : patterns)
        {
            size_t pos = text.find(pat, 0);
            while (pos != std::string::npos)
            {
                ++found;
                pos = text.find(pat, pos + 1);
            }
        }
    }

    return found;
}

long long run_stringsearch_once(int threads)
{
    const size_t totalTextBytes = 256ULL * 1024ULL * 1024ULL; // Total text size is 256 MB
    const int totalRepeats = 30; // Number of times to repeat the search patterns

    // Generate one global text buffer so every mode sees the same logical input data.
    std::mt19937 rng(2024);
    std::uniform_int_distribution<int> dist(0, 25);
    std::string text(totalTextBytes, 'a');
    for (size_t i = 0; i < totalTextBytes; ++i)
        text[i] = char('a' + dist(rng));

    const std::vector<std::string> patterns = {
        "abc", "hello", "world", "performance", "thread",
        "cache", "benchmark", "search", "random", "zzzz"};

    std::vector<std::thread> pool;
    std::vector<long long> partial(threads, 0);

    const int baseRepeats = totalRepeats / threads;
    const int extraRepeats = totalRepeats % threads;

    for (int i = 0; i < threads; ++i)
    {
        int myRepeats = baseRepeats + (i < extraRepeats ? 1 : 0);
        pool.emplace_back([&, i, myRepeats]()
        {
            partial[i] = worker_stringsearch(text, patterns, myRepeats, threads);
        });
    }

    for (auto &th : pool)
        th.join();

    long long total = 0;
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
    long long checksum = 0;

    for (int r = 0; r < cfg.runs; ++r)
    {
        long long current = 0;
        double elapsed = measure_once([&]()
                                      { current = run_stringsearch_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("stringsearch", cfg, times, checksum);
    return 0;
}