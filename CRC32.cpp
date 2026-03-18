#include "benchmark_common.hpp"
#include <random>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>

uint32_t crc32_update(uint32_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xEDB88320U;
        else
            crc >>= 1;
    }
    return crc;
}

uint32_t crc32_buf(const std::vector<uint8_t> &buf)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint8_t b : buf)
        crc = crc32_update(crc, b);
    return ~crc;
}

uint32_t worker_crc32(int tid, int blocks, int blockSize, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    std::mt19937 rng(12345 + tid * 17);
    std::uniform_int_distribution<int> dist(0, 255);

    std::vector<uint8_t> buf(blockSize);
    uint32_t final_crc = 0;

    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            buf[i] = (uint8_t)dist(rng);
        }
        final_crc ^= crc32_buf(buf);
    }

    return final_crc;
}

uint32_t run_crc32_once(int threads)
{
    const int blockSize = 512 * 1024 * 1024;
    const int blocksPerThread = 12;

    std::vector<std::thread> pool;
    std::vector<uint32_t> partial(threads, 0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_crc32(i, blocksPerThread, blockSize, threads); });
    }

    for (auto &th : pool)
        th.join();

    uint32_t total = 0;
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
    uint32_t checksum = 0;

    for (int r = 0; r < cfg.runs; ++r)
    {
        uint32_t current = 0;
        double elapsed = measure_once([&]()
                                      { current = run_crc32_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("CRC32", cfg, times, checksum);
    return 0;
}