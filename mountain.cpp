#include <iostream>
#include <vector>
#include<chrono>
#include "fcyc2.h"
#include "clock.h"
#include <iomanip>

#define MINBYTES (1 << 10)
#define MAXBYTES (1 << 27)
#define MAXSTRIDE 32
#define STRIDESTRIDE 2
#define MAXELEMS MAXBYTES/sizeof(int)

std::vector<int> data(MAXELEMS);

void init_data(std::vector<int>& data, int n);
void test(int elems, int stride);
double run(int size, int stride, double Mhz);
int measure_cache_size(int start_size, int max_size);
double measure_latency(int size);

int main() {
    int size;
    int stride;
    double Mhz = 2.3e9; // replace with your CPU clock frequency

    for (size = 16; size <= MAXELEMS; size *= 2) {
        double mbps = 0.0;
        for (stride = 1; stride <= size / 2; stride *= 2) {
            mbps = run(size * sizeof(long), stride) / 1e6;
        }
        printf("size: %d, bandwidth: %f MB/s\n", size, mbps);
    }

    return 0;
}

void init_data(std::vector<int>& data, int n)
{
    for (int i = 0; i < n; i++)
        data[i] = 1;
}

void test(int elems, int stride)
{
    int result = 0;
    for (int i = 0; i < elems; i += stride)
        result += data[i];
    std::cout << result << "\n";
}

double run(int size, int stride, double Mhz)
{
    double cycles;
    int elems = size / sizeof(int);

    test(elems, stride);
    cycles = fcyc2(test, elems, stride, 0);
    double bytesPerSec = (size / stride) / (cycles / Mhz);
    double megaBytesPerSec = bytesPerSec / (1024 * 1024); // Convert to MB/s
    return megaBytesPerSec;
}

int measure_cache_size(int start_size, int max_size)
{
    int size;
    double time, prev_time;

    for (size = start_size; size <= max_size; size *= 2) {
        time = measure_latency(size);
        if (size > start_size && time > 2 * prev_time) {
            // The time has more than doubled, which suggests that
            // the memory no longer fits in the cache.
            break;
        }
        prev_time = time;
    }

    return size / 2;
}

double measure_latency(int size)
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < size; i += sizeof(int)) {
        data[i] = 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    return diff.count() / size;
}