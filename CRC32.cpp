#include "benchmark_common.hpp"
#include <random>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>

/*
 * PURPOSE:
 * This benchmark measures the performance of data integrity algorithms
 * by computing the 32-bit Cyclic Redundancy Check (CRC32).
 */

// Computes the CRC-32 cyclic redundancy check value for a single byte.
// It XORs the byte into the CRC, then processes it bit-by-bit.
// If the lowest bit is 1, the CRC is shifted right and XORed with the standard CRC-32 polynomial (0xEDB88320).
// Otherwise, it is just shifted right by 1 bit.
uint32_t crc32_update(uint32_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; ++i)
    {
        if (crc & 1)
            // 0xEDB88320 is the reversed polynomial for CRC-32 (IEEE 802.3)
            crc = (crc >> 1) ^ 0xEDB88320U;
        else
            crc >>= 1;
    }
    return crc;
}

// Processes an entire buffer of bytes sequentially to produce a final CRC-32 checksum.
// The initial CRC state is 0xFFFFFFFF, and the final result is inverted (~crc).
uint32_t crc32_buf(const std::vector<uint8_t> &buf)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint8_t b : buf)
        crc = crc32_update(crc, b);
    return ~crc;
}

uint32_t worker_crc32(int tid,
                      int beginBlock,
                      int endBlock,
                      int blockSize,
                      int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    uint32_t final_crc = 0;
    std::vector<uint8_t> buf(blockSize);

    for (int blockIdx = beginBlock; blockIdx < endBlock; ++blockIdx)
    {
        // Generate pseudo-random data for the block using a thread-specific seed.
        std::mt19937 rng(12345 + blockIdx * 17);
        std::uniform_int_distribution<int> dist(0, 255);

        for (int i = 0; i < blockSize; ++i)
            buf[i] = static_cast<uint8_t>(dist(rng));

        final_crc ^= crc32_buf(buf);
    }

    return final_crc;
}

uint32_t run_crc32_once(int threads)
{
    const int blockSize = 1024 * 1024 * 1024;
    const int totalBlocks = 12;   // Total data size is 6 GB (12 blocks * 1 GB each)
    std::vector<std::thread> pool;
    std::vector<uint32_t> partial(threads, 0);

    int base = totalBlocks / threads;
    int extra = totalBlocks % threads;

    int start = 0;
    for (int i = 0; i < threads; ++i)
    {
        int cnt = base + (i < extra ? 1 : 0);
        int beginBlock = start;
        int endBlock = beginBlock + cnt;
        start = endBlock;

        pool.emplace_back([&, i, beginBlock, endBlock]()
        {
            partial[i] = worker_crc32(i, beginBlock, endBlock, blockSize, threads);
        });
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