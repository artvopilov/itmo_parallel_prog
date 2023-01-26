#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include <parlay/parallel.h>

using namespace std;
using namespace std::chrono;

const string benchmark_file = "quicksort_benchmark.txt";
const int length = 1e8;
const int trials = 5;
const int min_length_parallel = 1000;

int compute_partition(vector<int> &array, int left, int right) {
    int x = array[right];
    int x_index = left;
    for (int i = left; i < right; i++) {
        if (array[i] <= x) {
            swap(array[i], array[x_index]);
            x_index++;
        }
    }
    swap(array[right], array[x_index]);
    return x_index;
}


void sequential_quicksort(vector<int> &array, int left, int right) {
    if (left >= right) {
        return;
    }
    int x_ind = compute_partition(array, left, right);
    sequential_quicksort(array, left, x_ind - 1);
    sequential_quicksort(array, x_ind + 1, right);
}


void parallel_quicksort(vector<int> &array, int left, int right) {
    if (left >= right) {
        return;
    }
    if (right - left < min_length_parallel) {
        sequential_quicksort(array, left, right);
        return;
    }
    int x_ind = compute_partition(array, left, right);
    parlay::par_do([&] {parallel_quicksort(array, left, x_ind - 1);},
                   [&] {parallel_quicksort(array, x_ind + 1, right); });
}


int main() {
    freopen(benchmark_file.c_str(), "w", stdout);

    float sum_seq_ms = 0;
    float sum_par_ms = 0;

    for (int j = 0; j < trials; j++) {
        random_device rd;
        mt19937 gen(rd());

        uniform_int_distribution<> dist(1, length);
        vector<int> seq_array;
        vector<int> par_array;
        for (int i = 0; i < length; i++) {
            int el = dist(gen);
            seq_array.push_back(el);
            par_array.push_back(el);
        }

        time_point start_time = high_resolution_clock::now();
        sequential_quicksort(seq_array, 0, length - 1);
        time_point end_time = high_resolution_clock::now();
        long elapsed_ms = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "Sequential sort elapsed in " << elapsed_ms << " ms" << endl;
        sum_seq_ms += float(elapsed_ms);

        start_time = high_resolution_clock::now();
        parallel_quicksort(par_array, 0, length - 1);
        end_time = high_resolution_clock::now();
        elapsed_ms = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "Parallel sort elapsed in " << elapsed_ms << " ms" << endl;
        sum_par_ms += float(elapsed_ms);

    }

    cout << string(40, '-') << endl;
    cout << "Avg sequential sort performance: " << sum_seq_ms / trials << " ms" << endl;
    cout << "Avg parallel sort performance: " << sum_par_ms / trials << " ms" << endl;

    return 0;
}