#ifndef BENCHMARK_COMMON_HPP
#define BENCHMARK_COMMON_HPP

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#endif

struct BenchmarkConfig
{
    int mode;    // 1 = single, 2 = multi
    int runs;    // 1..10
    int threads; // 1 or hardware_concurrency
};

inline void pin_current_thread_to_core0()
{
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif
}

inline BenchmarkConfig parse_config_from_argv(int argc, char *argv[])
{
    if (argc != 3)
    {
        throw std::runtime_error(
            "Usage: <program> <mode> <runs>\n"
            "  mode: 1=single core pinned to core 0, 2=multi core max threads\n"
            "  runs: integer from 1 to 10\n"
            "Example:\n"
            "  .\\basicmath 1 3\n");
    }

    BenchmarkConfig cfg{};

    try
    {
        cfg.mode = std::stoi(argv[1]);
        cfg.runs = std::stoi(argv[2]);
    }
    catch (...)
    {
        throw std::runtime_error(
            "Invalid arguments.\n"
            "mode must be 1 or 2, runs must be an integer from 1 to 10.\n"
            "Example: .\\basicmath 2 5\n");
    }

    if (cfg.mode != 1 && cfg.mode != 2)
    {
        throw std::runtime_error("Invalid mode. Use 1 for single or 2 for multi.");
    }

    if (cfg.runs < 1 || cfg.runs > 10)
    {
        throw std::runtime_error("Invalid runs. Runs must be between 1 and 10.");
    }

    if (cfg.mode == 1)
    {
        cfg.threads = 1;
    }
    else
    {
        cfg.threads = (int)std::thread::hardware_concurrency();
        if (cfg.threads <= 0)
            cfg.threads = 1;
    }

    return cfg;
}

template <typename F>
double measure_once(F &&fn)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    fn();
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(t2 - t1).count();
}

inline double mean_of(const std::vector<double> &times)
{
    if (times.empty())
        return 0.0;
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    return sum / static_cast<double>(times.size());
}

template <typename T>
void print_summary(const char *benchmark_name,
                   const BenchmarkConfig &cfg,
                   const std::vector<double> &times,
                   const T &checksum)
{
    std::cout << "\n===== " << benchmark_name << " Summary =====\n";
    std::cout << "Mode: "
              << (cfg.mode == 1 ? "single core pinned to core 0"
                                : "multi core max threads")
              << "\n";
    std::cout << "Threads used: " << cfg.threads << "\n";
    std::cout << "Runs: " << cfg.runs << "\n";

    for (size_t i = 0; i < times.size(); ++i)
    {
        std::cout << "Run " << (i + 1) << ": "
                  << std::fixed << std::setprecision(6)
                  << times[i] << " seconds\n";
    }

    std::cout << "Mean elapsed time: "
              << std::fixed << std::setprecision(6)
              << mean_of(times) << " seconds\n";
    std::cout << "Final checksum/result: " << checksum << "\n";
}

#endif