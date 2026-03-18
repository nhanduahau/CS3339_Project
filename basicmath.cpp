#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

double worker(int tid, int threads, long long iterations)
{
#ifdef _WIN32
    if (threads == 1)
    {
        SetThreadAffinityMask(GetCurrentThread(), 1ULL);
    }
#endif

    long long chunk = iterations / threads;
    long long start = tid * chunk;
    long long end = (tid == threads - 1) ? iterations : start + chunk;

    double sum = 0.0;
    for (long long i = start; i < end; ++i)
    {
        double x = (i % 10000) * 0.0001 + 1.0;
        double local = 1.0;
        for (int k = 0; k < 50; ++k)
        {
            double a = sin(x) * cos(x) + sqrt(x) + log(x) + tanh(x);
            double b = exp(-x * 0.1) + atan(x) + cbrt(x + 0.5);
            local = local * 0.999999 + a * b;
            sum += a + b + local / (k + 1.0);
            x += 0.000001;
        }
    }
    return sum;
}

int main()
{
    int mode;
    cout << "basicmath benchmark\n";
    cout << "Enter mode (1=single, 2=multi): ";
    cin >> mode;

    int threads = (mode == 2) ? (int)thread::hardware_concurrency() : 1;
    if (threads <= 0)
        threads = 1;

    const long long iterations = 40'000'000LL;
    const int runs = 5;
    double totalElapsedSeconds = 0.0;

    cout << fixed << setprecision(6);
    cout << "Threads used: " << threads << "\n";

    for (int run = 1; run <= runs; ++run)
    {
        vector<thread> pool;
        vector<double> partial(threads, 0.0);

        auto t1 = chrono::high_resolution_clock::now();

        for (int i = 0; i < threads; ++i)
        {
            pool.emplace_back([&, i]()
                              { partial[i] = worker(i, threads, iterations); });
        }

        for (auto &th : pool)
            th.join();

        auto t2 = chrono::high_resolution_clock::now();
        double total = 0.0;
        for (double v : partial)
            total += v;

        chrono::duration<double> elapsed = t2 - t1;
        totalElapsedSeconds += elapsed.count();

        cout << "Run " << run << " checksum: " << total << "\n";
        cout << "Run " << run << " elapsed: " << elapsed.count() << " seconds\n";
    }

    cout << "Average elapsed (5 runs): " << (totalElapsedSeconds / runs) << " seconds\n";
    return 0;
}