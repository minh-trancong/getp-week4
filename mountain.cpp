#include <iostream>
#include <vector>
#include<chrono>
#include "fcyc2.h"
#include "clock.h"

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

int main()
{
    int size;
    int stride;
    double Mhz;

    init_data(data, MAXELEMS);
    Mhz = mhz(0);

    std::cout << "Clock frequency is approx. " << Mhz << " MHz\n";
    std::cout << "Memory mountain (MB/sec)\n";

    std::cout << " ";
    for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE)
        std::cout << "s" << stride << " ";
    std::cout << "\n";

    for (size = MAXBYTES; size >= MINBYTES; size >>= 1) {
        if (size > (1 << 20))
            std::cout << size / (1 << 20) << "m ";
        else
            std::cout << size / 1024 << "k ";

        for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE) {
            std::cout << run(size, stride, Mhz) << " ";
        }
        if (stride > MAXSTRIDE) {
            std::cout << "\n";
        }
        std::cout << "Cache size: " << measure_cache_size(MINBYTES, size) << "\n";
        std::cout << "Latency: " << measure_latency(size) << "\n";
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
    return (size / stride) / (cycles / Mhz);
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