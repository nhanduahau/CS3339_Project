#include "benchmark_common.hpp"
#include <queue>
#include <random>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>
#include <functional>

using pii = std::pair<int, int>;

uint64_t run_dijkstra_graph(const std::vector<std::vector<pii>> &graph, int src)
{
    const int INF = 1e9;
    int n = (int)graph.size();
    std::vector<int> dist(n, INF);
    std::priority_queue<pii, std::vector<pii>, std::greater<pii>> pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty())
    {
        auto [d, u] = pq.top();
        pq.pop();
        if (d != dist[u])
            continue;

        for (auto [v, w] : graph[u])
        {
            if (dist[v] > d + w)
            {
                dist[v] = d + w;
                pq.push({dist[v], v});
            }
        }
    }

    uint64_t checksum = 0;
    for (int x : dist)
        checksum += (uint64_t)x;
    return checksum;
}

uint64_t worker_dijkstra(int tid, int repeats, int nodes, int edgesPerNode, int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    std::mt19937 rng(1000 + tid);
    std::uniform_int_distribution<int> nodeDist(0, nodes - 1);
    std::uniform_int_distribution<int> weightDist(1, 100);

    std::vector<std::vector<pii>> graph(nodes);

    for (int u = 0; u < nodes; ++u)
    {
        for (int e = 0; e < edgesPerNode; ++e)
        {
            int v = nodeDist(rng);
            int w = weightDist(rng);
            if (v != u)
                graph[u].push_back({v, w});
        }
    }

    uint64_t total = 0;
    for (int r = 0; r < repeats; ++r)
    {
        total ^= run_dijkstra_graph(graph, rng() % nodes);
    }

    return total;
}

uint64_t run_dijkstra_once(int threads)
{
    const int nodes = 100000;
    const int edgesPerNode = 100;
    const int repeats = 40;

    std::vector<std::thread> pool;
    std::vector<uint64_t> partial(threads, 0);

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back([&, i]()
                          { partial[i] = worker_dijkstra(i, repeats, nodes, edgesPerNode, threads); });
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
                                      { current = run_dijkstra_once(cfg.threads); });

        times.push_back(elapsed);
        checksum = current;
    }

    print_summary("dijkstra", cfg, times, checksum);
    return 0;
}