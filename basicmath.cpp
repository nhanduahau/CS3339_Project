#include "benchmark_common.hpp"
#include <cmath>
#include <vector>
#include <thread>
#include <iostream>

double worker_basicmath(int tid, int threads, long long iterations)
{
    long long chunk = iterations / threads;
    long long start = tid * chunk;
    long long end = (tid == threads - 1) ? iterations : start + chunk;

    if (threads == 1)
        pin_current_thread_to_core0();

    double sum = 0.0;
    for (long long i = start; i < end; ++i)
    {
        double x = (i % 10000) * 0.0001 + 1.0;
        for (int k = 0; k < 100; ++k)
        {
            sum += std::sin(x) * std::cos(x) + std::sqrt(x) + std::log(x) + std::tanh(x);
            x += 0.000001;
        }
    }
    return sum;
}

double run_basicmath_once(int threads)
{
    const long long iterations = 50'000'000LL;

    std::vector<std::thread> pool;
    std::vector<double> partial(threads, 0.0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_basicmath(i, threads, iterations); });
    }

    for (auto &th : pool)
        th.join();

    double total = 0.0;
    for (double v : partial)
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
    double checksum = 0.0;

    for (int r = 0; r < cfg.runs; ++r)
    {
        double current = 0.0;
        double elapsed = measure_once([&]()
                                      { current = run_basicmath_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("basicmath", cfg, times, checksum);
    return 0;
}