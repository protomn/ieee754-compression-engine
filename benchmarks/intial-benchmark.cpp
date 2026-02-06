#include <iostream>
#include <vector>
#include <chrono>
#include "ieee754.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "bitstream.hpp"

int main()
{
    using namespace comp;

    const int TICKS = 1000000;
    std::vector<double> data(TICKS, 150.25);

    for(int i{0}; i < TICKS; i+= 10)
    {
        data[i] += 0.01;
    }

    Encoder encoder(TICKS * 128); //Pre-allocate memory

    auto start = std::chrono::high_resolution_clock::now();

    for(const auto &d : data)
    {
        encoder.append(d);
    }

    encoder.fin();

    auto end = std::chrono::high_resolution_clock::now();
    auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Processed " << TICKS << " ticks in " << time_len << " microseconds.\n";
    std::cout << "Throughput: " << (TICKS / (time_len / 1000000.0)) / 1e6 << " million ticks/sec.\n";

    return 0;
}   