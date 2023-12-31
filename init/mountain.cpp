#include <cstdlib>
#include <cstdio>
#include "fcyc2.h" /* K-best measurement timing routines */
#include "clock.h" /* routines to access the cycle counter */
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>

#define MINBYTES (1 << 10)  /* Working set size ranges from 1 KB */
#define MAXBYTES (1 << 27)  /* ... up to 128 MB */
#define MAXSTRIDE 32        /* Strides range from 1 to 32 */
#define STRIDESTRIDE 2      /* increment stride by this amount each time */
#define MAXELEMS MAXBYTES/sizeof(int)

int data[MAXELEMS];         /* The array we'll be traversing */

void init_data(int *data, int n);
void test(int elems, int stride);
double run(int size, int stride, double Mhz);
double measure_latency();
double measure_storage(int size);

int main()
{
    int size;        /* Working set size (in bytes) */
    int stride;      /* Stride (in array elements) */
    double Mhz;      /* Clock frequency */

    init_data(data, MAXELEMS); /* Initialize each element in data to 1 */
    Mhz = mhz(0);              /* Estimate the clock frequency */

    // Measure and print the memory latency
    double latency = measure_latency();
    std::cout << "Memory latency: " << latency << " seconds\n";

    std::cout << "Clock frequency is approx. " << Mhz << " MHz\n";
    std::cout << "Memory mountain (MB/sec)\n";

    std::cout << "\t";
    for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE)
        std::cout << "s" << stride << "\t";
    std::cout << "\n";

    // Number of threads to use
    int numThreads = std::thread::hardware_concurrency();

    // Vector to store threads
    std::vector<std::thread> threads;

    for (size = MAXBYTES; size >= MINBYTES; size >>= 1) {
        if (size > (1 << 20))
            std::cout << size / (1 << 20) << "m\t";
        else
            std::cout << size / 1024 << "k\t";

        for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE) {
            std::cout << run(size, stride, Mhz) << "\t";
        }
        std::cout << "\n";


        // Create and store threads
        for (int t = 0; t < numThreads; ++t) {
            threads.push_back(std::thread([&, elems, stride]() { // Capture elems and stride by value
                int start = t * (elems / numThreads);
                int end = (t + 1) * (elems / numThreads);
                for (int i = start; i < end; i += stride) {
                    test(i, stride);
                }
            }));
        }

        // Join threads
        for (auto& thread : threads) {
            thread.join();
        }

        // Clear the threads vector for the next iteration
        threads.clear();
    }

    // Measure and print the storage bandwidth
    double storageBandwidth = measure_storage(MAXBYTES);
    std::cout << "Storage bandwidth: " << storageBandwidth << " MB/sec\n";

    return 0;
}

void init_data(int *data, int n)
{
    int i;

    for (i = 0; i < n; i++)
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

    test(elems, stride);                     /* warm up the cache */
    cycles = fcyc2(test, elems, stride, 0);  /* call test(elems,stride) */
    return (size / stride) / (cycles / Mhz); /* convert cycles to MB/s */
}

// Function to measure latency of memory access
double measure_latency() {
    auto start = std::chrono::high_resolution_clock::now();
    volatile int temp = data[0]; // Access the first element of the array
    (void)temp; // Add this line
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count(); // Return time in seconds
}

// Function to measure storage bandwidth
double measure_storage(int size) {
    char* buffer = new char[size];
    std::fill(buffer, buffer + size, 'a'); // Fill buffer with some data

    auto start = std::chrono::high_resolution_clock::now();
    std::ofstream ofs("tempfile", std::ios::binary);
    ofs.write(buffer, size); // Write data to file
    ofs.close();
    auto end = std::chrono::high_resolution_clock::now();

    delete[] buffer;
    std::remove("tempfile"); // Delete the temporary file

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    double megabytes = size / (1024.0 * 1024.0); // Convert size to megabytes
    return megabytes / seconds; // Return bandwidth in MB/sec
}