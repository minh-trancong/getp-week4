#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include "fcyc2.h"
#include "clock.h"

#define MINBYTES (1 << 10)
#define MAXBYTES (1 << 27)
#define MAXSTRIDE 6
#define STRIDESTRIDE 1
#define MAXELEMS MAXBYTES/sizeof(int)

int data[MAXELEMS];

void init_data(int *data, int n);
void test(int elems, int stride);
double run(int size, int stride, double Mhz);
double measure_latency();
double measure_storage(int size);

int main()
{
    int size;
    int stride;
    double Mhz;

    init_data(data, MAXELEMS);
    Mhz = mhz(0);

    double latency = measure_latency();
    std::cout << "Memory latency: " << latency << " seconds\n";

    std::cout << "Clock frequency is approx. " << Mhz << " MHz\n";
    std::cout << "Memory mountain (MB/sec)\n";

    std::cout << "\t";
    for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE)
        std::cout << "s" << stride << "\t";
    std::cout << "\n";

    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    for (size = MAXBYTES; size >= MINBYTES; size >>= 1) {
        if (size > (1 << 20))
            std::cout << size / (1 << 20) << "m\t";
        else
            std::cout << size / 1024 << "k\t";

        int elems = size / sizeof(int);
        if (numThreads > 1) {
            // Use multithreading if the system supports it
            for (int t = 0; t < numThreads; ++t) {
                threads.push_back(std::thread([&, elems, stride, t]() {
                    int start = t * (elems / numThreads);
                    int end = (t + 1) * (elems / numThreads);
                    for (int i = start; i < end; i += stride) {
                        test(elems, stride);
                    }
                }
            }

            for (auto& thread : threads) {
                thread.join();
            }

            threads.clear();
        } else {
            // Use single-threaded version otherwise
            for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE) {
                std::cout << run(size, stride, Mhz) << "\t";
            }
            std::cout << "\n";
        }
    }

    double storageBandwidth = measure_storage(MAXBYTES);
    std::cout << "Storage bandwidth: " << storageBandwidth << " MB/sec\n";

    return 0;
}

void init_data(int *data, int n)
{
    for (int i = 0; i < n; i++)
        data[i] = 1;
}

void test(int elems, int stride)
{
    volatile int sink;
    for (int i = 0; i < elems; i += stride)
        sink = data[i];
    (void)sink;
}

double run(int size, int stride, double Mhz)
{
    double cycles;
    int elems = size / sizeof(int);

    test(elems, stride);
    cycles = fcyc2(test, elems, stride, 0);
    return (size / stride) / (cycles / Mhz);
}

double measure_latency() {
    auto start = std::chrono::high_resolution_clock::now();
    volatile int temp = data[0];
    (void)temp;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count();
}

double measure_storage(int size) {
    char* buffer = new char[size];
    std::fill(buffer, buffer + size, 'a');

    auto start = std::chrono::high_resolution_clock::now();
    std::ofstream ofs("tempfile", std::ios::binary);
    ofs.write(buffer, size);
    ofs.close();
    auto end = std::chrono::high_resolution_clock::now();

    delete[] buffer;
    std::remove("tempfile");

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    double megabytes = size / (1024.0 * 1024.0);
    return megabytes / seconds;
}