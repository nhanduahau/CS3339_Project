#include "benchmark_common.hpp"
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <iostream>

long long worker_stringsearch(int tid, int textSizeMB, int repeats, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    std::mt19937 rng(2024 + tid);
    std::uniform_int_distribution<int> dist(0, 25);

    size_t n = (size_t)textSizeMB * 1024 * 1024;
    std::string text(n, 'a');
    for (size_t i = 0; i < n; ++i)
        text[i] = char('a' + dist(rng));

    std::vector<std::string> patterns = {
        "abc", "hello", "world", "performance", "thread",
        "cache", "benchmark", "search", "random", "zzzz"};

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
    const int textSizeMB = 64;
    const int repeats = 10;

    std::vector<std::thread> pool;
    std::vector<long long> partial(threads, 0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_stringsearch(i, textSizeMB, repeats, threads); });
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