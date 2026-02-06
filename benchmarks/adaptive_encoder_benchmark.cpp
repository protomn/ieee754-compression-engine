#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include "encoder.hpp"

void runBenchmark(const std::string &name, comp::Encoder &encoder,
                  const std::vector<double> &data)
{
    auto start = std::chrono::high_resolution_clock::now();

    for(const auto &d : data)
    {
        encoder.append(d);
    }

    encoder.fin();

    auto end = std::chrono::high_resolution_clock::now();
    auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double total_bytes = (data.size() * sizeof(double)) / (1024.0 * 1024.0);
    double compressed_bytes = (encoder.sizeInBytes()) / (1024.0 * 1024.0);
    double ratio = (double)(data.size() * sizeof(double)) / encoder.sizeInBytes();
    double throughput = (data.size() / (time_len / 1000000.0)) / 1e6;

    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " M ticks/sec.\n";
    std::cout << "Raw Size: " << total_bytes << " MB.\n";
    std::cout << "Compressed Size: " << compressed_bytes << " MB.\n";
    std::cout << "Compression Ratio: " << ratio << "x\n\n";
}

int main()
{
    const int TICKS = 10000000;
    std::vector<double> data;
    data.reserve(TICKS);

    double price{100.25};

    for(int i{0}; i < TICKS; ++i)
    {
        if(i % 5 == 0)
        {
            price += 0.01;
        }

        data.push_back(price);
    }

    //Benchmarking basic encoder
    comp::Encoder encoder1(TICKS * 64);
    runBenchmark("Basic Encoder", encoder1, data);

    //Benchmarking adaptive encoder
    comp::AdaptiveEncoder encoder2(TICKS * 64);
    runBenchmark("Adaptive Encoder", encoder2, data);

    return 0;
}