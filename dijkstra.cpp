#include "benchmark_common.hpp"
#include <queue>
#include <random>
#include <cstdint>
#include <vector>
#include <thread>
#include <iostream>
#include <functional>

/*
 * PURPOSE:
 * This benchmark tests memory access patterns and priority queue performance
 * by calculating the shortest paths in a graph using Dijkstra's algorithm.
 */

using pii = std::pair<int, int>;

// Run Dijkstra's shortest path algorithm from a given source node to all other nodes.
// It computes the minimum distance using a priority queue.
uint64_t run_dijkstra_graph(const std::vector<std::vector<pii>> &graph, int src)
{
    const int INF = 1e9;
    int n = (int)graph.size();
    // Initialize all distances to infinity
    std::vector<int> dist(n, INF);
    // Min-heap priority queue to always process the closest node next
    std::priority_queue<pii, std::vector<pii>, std::greater<pii>> pq;

    dist[src] = 0;
    // Push the source node with a distance of 0
    pq.push({0, src});

    while (!pq.empty())
    {
        // Extract the node with the minimum distance
        auto [d, u] = pq.top();
        pq.pop();
        // If we found a shorter path previously, skip processing
        if (d != dist[u])
            continue;

        // Iterate over all adjacent edges of the current node u
        for (auto [v, w] : graph[u])
        {
            // Relaxation: update path if passing through u is shorter
            if (dist[v] > d + w)
            {
                dist[v] = d + w;
                pq.push({dist[v], v});
            }
        }
    }

    uint64_t checksum = 0;
    // Calculate a simple checksum consisting of the sum of shortest distances
    for (int x : dist)
        checksum += (uint64_t)x;
    return checksum;
}

uint64_t worker_dijkstra(int tid,
                         const std::vector<std::vector<pii>> &graph,
                         const std::vector<int> &sources,
                         int beginIdx,
                         int endIdx,
                         int threads)
{
    if (threads == 1)
        pin_current_thread_to_core0();

    uint64_t total = 0;
    for (int i = beginIdx; i < endIdx; ++i)
    {
        total ^= run_dijkstra_graph(graph, sources[i]);
    }

    return total;
}

uint64_t run_dijkstra_once(int threads)
{
    const int nodes = 100000; // Total number of nodes in the graph (100,000 nodes)
    const int edgesPerNode = 1000;  // Each node has 1000 outgoing edges on average (1 billion edges total)
    const int totalQueries = 500; // Total number of source nodes used for repeated Dijkstra runs

    // Build a random graph with the specified number of nodes and edges per node
    std::mt19937 build_rng(1000);
    std::uniform_int_distribution<int> nodeDist(0, nodes - 1);
    std::uniform_int_distribution<int> weightDist(1, 100);

    std::vector<std::vector<pii>> graph(nodes);
    for (int u = 0; u < nodes; ++u)
    {
        for (int e = 0; e < edgesPerNode; ++e)
        {
            int v = nodeDist(build_rng);
            int w = weightDist(build_rng);
            if (v != u)
                graph[u].push_back({v, w});
        }
    }

    // Generate random source nodes for the Dijkstra's algorithm queries
    std::mt19937 src_rng(2000);
    std::vector<int> sources(totalQueries);
    for (int i = 0; i < totalQueries; ++i)
        sources[i] = src_rng() % nodes;

    std::vector<std::thread> pool;
    std::vector<uint64_t> partial(threads, 0);

    int base = totalQueries / threads;
    int extra = totalQueries % threads;

    int start = 0;
    for (int i = 0; i < threads; ++i)
    {
        int cnt = base + (i < extra ? 1 : 0);
        int beginIdx = start;
        int endIdx = beginIdx + cnt;
        start = endIdx;

        pool.emplace_back([&, i, beginIdx, endIdx]()
        {
            partial[i] = worker_dijkstra(i, graph, sources, beginIdx, endIdx, threads);
        });
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