#include <iostream>
#include <vector>
#include <queue>
#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

using namespace std;
using namespace std::chrono;


const string benchmark_file = "bfs_benchmark.txt";
const int cube_size = 100;
const int trials = 5;


class Vertex {
public:
    int x, y, z;
    Vertex(int x, int y, int z): x(x), y(y), z(z) {}
};

parlay::sequence<Vertex> compute_adjacent_vertices(Vertex &vertex, int size) {
    parlay::sequence<Vertex> vertices = parlay::sequence<Vertex>();
    if (vertex.x < size - 1) {
        vertices.emplace_back(vertex.x + 1, vertex.y, vertex.z);
    }
    if (vertex.y < size - 1) {
        vertices.emplace_back(vertex.x, vertex.y + 1, vertex.z);
    }
    if (vertex.z < size - 1) {
        vertices.emplace_back(vertex.x, vertex.y, vertex.z + 1);
    }
    return vertices;
}

void sequential_bfs(const Vertex *root, int size) {
    queue<Vertex> q;
    q.push(*root);
    vector<vector<vector<bool>>> visited(size, vector<vector<bool>>(size, vector<bool>(size, false)));
    visited[root->x][root->y][root->z] = true;
    while (!q.empty()) {
        Vertex vert = q.front();
        q.pop();
        parlay::sequence <Vertex> adjacent_vertices = compute_adjacent_vertices(vert, size);
        for (auto v: adjacent_vertices) {
            if (!visited[v.x][v.y][v.z]) {
                visited[v.x][v.y][v.z] = true;
                q.push(v);
            }
        }
    }
}

void parallel_bfs(const Vertex *root, int size) {
    parlay::sequence<Vertex> q(1, *root);
    parlay::sequence<parlay::sequence<parlay::sequence<atomic_flag>>> visited =
            parlay::tabulate<parlay::sequence<parlay::sequence<atomic_flag> >>(size, [&](const int x) {
                return parlay::tabulate< parlay::sequence<atomic_flag>>(size, [&](const int y) {
                    return parlay::tabulate<atomic_flag>(size, [&](const int z) {return false; }); }); });
    visited[root->x][root->y][root->z].test_and_set(memory_order_relaxed);
    while (!q.empty()) {
        parlay::sequence<unsigned long> adj_vert_counts = parlay::map(q, [&](Vertex v) {
            return compute_adjacent_vertices(v, size).size(); });
        pair<parlay::sequence<unsigned long>, int> adj_vert_counts_scan = parlay::scan(adj_vert_counts);
        parlay::sequence<unsigned long> adj_vert_counts_prefix_sum = adj_vert_counts_scan.first;
        unsigned long adj_vert_counts_total_sum = adj_vert_counts_scan.second;

        parlay::sequence<Vertex> new_q = parlay::tabulate<Vertex>(adj_vert_counts_total_sum, [&](const int i) {
            return Vertex(-1, 0, 0); });
        parlay::parallel_for(0, q.size(), [&](int i) {
            Vertex node = q[i];
            const parlay::sequence<Vertex> adjacent_vertices = compute_adjacent_vertices(node, size);
            parlay::parallel_for(0, adjacent_vertices.size(), [&](int adj_v_i) {
                Vertex adj_v = adjacent_vertices[adj_v_i];
                if (!visited[adj_v.x][adj_v.y][adj_v.z].test_and_set(memory_order_relaxed)) {
                    new_q[adj_vert_counts_prefix_sum[i] + adj_v_i] = adj_v;
                }
            });
        });
        q = parlay::filter(new_q, [](Vertex vertex) {return vertex.x >= 0; });
    }
}


int main() {
    freopen(benchmark_file.c_str(), "w", stdout);

    float sum_seq_ms = 0;
    float sum_par_ms = 0;

    const Vertex *start = new Vertex(0, 0, 0);

    for (int i = 0; i < trials; i++) {
        time_point start_time = high_resolution_clock::now();
        sequential_bfs(start, cube_size);
        time_point end_time = high_resolution_clock::now();
        long elapsed_ms = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "Sequential bfs elapsed in " << elapsed_ms << " ms" << endl;
        sum_seq_ms += float(elapsed_ms);

        start_time = high_resolution_clock::now();
        parallel_bfs(start, cube_size);
        end_time = high_resolution_clock::now();
        elapsed_ms = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "Parallel bfs elapsed in " << elapsed_ms << " ms" << endl;
        sum_par_ms += float(elapsed_ms);
    }
    cout << string(40, '-') << endl;
    cout << "Avg sequential bfs performance: " << sum_seq_ms / trials << " ms" << endl;
    cout << "Avg parallel bfs performance: " << sum_par_ms / trials << " ms" << endl;
}