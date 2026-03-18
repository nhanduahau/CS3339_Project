#include "benchmark_common.hpp"
#include <cmath>
#include <random>
#include <vector>
#include <thread>
#include <iostream>

const double PI_VAL = 3.14159265358979323846;

double dct8x8(const double in[8][8], double out[8][8])
{
    double checksum = 0.0;

    for (int u = 0; u < 8; ++u)
    {
        for (int v = 0; v < 8; ++v)
        {
            double sum = 0.0;
            for (int x = 0; x < 8; ++x)
            {
                for (int y = 0; y < 8; ++y)
                {
                    sum += in[x][y] * std::cos((2 * x + 1) * u * PI_VAL / 16.0) * std::cos((2 * y + 1) * v * PI_VAL / 16.0);
                }
            }

            double cu = (u == 0) ? (1.0 / std::sqrt(2.0)) : 1.0;
            double cv = (v == 0) ? (1.0 / std::sqrt(2.0)) : 1.0;
            out[u][v] = 0.25 * cu * cv * sum;
            checksum += out[u][v];
        }
    }

    return checksum;
}

double worker_jpeg(int tid, int blocks, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    std::mt19937 rng(500 + tid);
    std::uniform_real_distribution<double> dist(0.0, 255.0);

    double total = 0.0;
    double in[8][8], out[8][8];

    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                in[i][j] = dist(rng) - 128.0;
            }
        }

        total += dct8x8(in, out);

        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                total += std::round(out[i][j] / (1 + ((i + j) % 8)));
            }
        }
    }

    return total;
}

double run_jpeg_once(int threads)
{
    const int blocksPerThread = 150000;

    std::vector<std::thread> pool;
    std::vector<double> partial(threads, 0.0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_jpeg(i, blocksPerThread, threads); });
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
                                      { current = run_jpeg_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("jpeg", cfg, times, checksum);
    return 0;
}