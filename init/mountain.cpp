#include <cstdlib>
#include <cstdio>
#include "fcyc2.h" /* K-best measurement timing routines */
#include "clock.h" /* routines to access the cycle counter */
#include <iostream>

#define MINBYTES (1 << 10)  /* Working set size ranges from 1 KB */
#define MAXBYTES (1 << 27)  /* ... up to 128 MB */
#define MAXSTRIDE 32        /* Strides range from 1 to 32 */
#define STRIDESTRIDE 2      /* increment stride by this amount each time */
#define MAXELEMS MAXBYTES/sizeof(int)

int data[MAXELEMS];         /* The array we'll be traversing */

void init_data(int *data, int n);
void test(int elems, int stride);
double run(int size, int stride, double Mhz);

int main()
{
    int size;        /* Working set size (in bytes) */
    int stride;      /* Stride (in array elements) */
    double Mhz;      /* Clock frequency */

    init_data(data, MAXELEMS); /* Initialize each element in data to 1 */
    Mhz = mhz(0);              /* Estimate the clock frequency */

    std::cout << "Clock frequency is approx. " << Mhz << " MHz\n";
    std::cout << "Memory mountain (MB/sec)\n";

    std::cout << "\t";
    for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE)
        std::cout << "s" << stride << "\t";
    std::cout << "\n";

    for (size = MAXBYTES; size >= MINBYTES; size >>= 1) {
        if (size > (1 << 20))
            std::cout << size / (1 << 20) << "m\t";
        else
            std::cout << size / 1024 << "k\t";

        for (stride = 1; stride <= MAXSTRIDE; stride += STRIDESTRIDE) {
            std::cout << run(size, stride, Mhz) << "\t";
        }
        std::cout << "\n";
    }
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