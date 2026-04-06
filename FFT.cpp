#include "benchmark_common.hpp"
#include <complex>
#include <cmath>
#include <random>
#include <vector>
#include <thread>
#include <iostream>

/*
 * PURPOSE:
 * This benchmark measures the computation of the Fast Fourier Transform (FFT)
 * which tests complex number arithmetic and data manipulation.
 */

const double PI_VAL = 3.14159265358979323846;

// Perform a Fast Fourier Transform using the iterative Radix-2 Cooley-Tukey algorithm
void fft(std::vector<std::complex<double>> &a)
{
    int n = (int)a.size();

    // Reorder the elements using bit-reversal permutation
    // This allows the actual FFT stages to be computed purely iteratively
    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        // Calculate the reversed bits of the index via bitwise shifts and XORs
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    // Iterate over each stage of the FFT, processing progressively larger paired chunks
    for (int len = 2; len <= n; len <<= 1)
    {
        // Calculate the base complex angle for the current stage size
        double ang = -2 * PI_VAL / len;
        std::complex<double> wlen(std::cos(ang), std::sin(ang));

        for (int i = 0; i < n; i += len)
        {
            std::complex<double> w(1);
            for (int j = 0; j < len / 2; ++j)
            {
                // Execute the continuous butterfly operation core calculation
                // Multiply by the twiddle factor (w) and mix pairs
                std::complex<double> u = a[i + j];
                std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                // Accumulate the phase rotation for the next pair
                w *= wlen;
            }
        }
    }
}

double worker_fft(int tid, int repeats, int n, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    std::mt19937 rng(900 + tid);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    double checksum = 0.0;
    std::vector<std::complex<double>> data(n);

    for (int r = 0; r < repeats; ++r)
    {
        for (int i = 0; i < n; ++i)
        {
            data[i] = std::complex<double>(dist(rng), dist(rng));
        }

        fft(data);

        for (int i = 0; i < n; i += n / 64)
        {
            checksum += std::abs(data[i]);
        }
    }

    return checksum;
}

double run_fft_once(int threads)
{
    const int n = 1 << 22; // Size of the input array (4 million complex numbers)
    const int totalRepeats = 200; // Number of times to repeat the FFT computation for better timing accuracy

    std::vector<std::thread> pool;
    std::vector<double> partial(threads, 0.0);

    const int baseRepeats = totalRepeats / threads;
    const int extraRepeats = totalRepeats % threads;

    for (int i = 0; i < threads; ++i)
    {
        int myRepeats = baseRepeats + (i < extraRepeats ? 1 : 0);
        pool.emplace_back([&, i, myRepeats]()
        {
            partial[i] = worker_fft(i, myRepeats, n, threads);
        });
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
                                      { current = run_fft_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("FFT", cfg, times, checksum);
    return 0;
}