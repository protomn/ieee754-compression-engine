#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include "encoder.hpp"

#pragma GCC optimize("unroll-loops")
void runFastBenchmark(const std::string &name, comp::FastEncoder &encoder,
                      const std::vector<double> &data)
{
    auto start = std::chrono::high_resolution_clock::now();

    for(const auto &d : data)
    {
        encoder.fastAppend(d);
    }

    encoder.fin();

    auto end = std::chrono::high_resolution_clock::now();
    auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double throughput = (data.size() / (time_len / 1000000.0)) / 1e6;

    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " M ticks/sec.\n";
}

void runBasicBenchmark(const std::string &name, comp::Encoder &encoder,
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

    double throughput = (data.size() / (time_len / 1000000.0)) / 1e6;

    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " M ticks/sec.\n";
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

    //Benchmark fast encoder
    comp::FastEncoder encoder1(TICKS * 32);
    runFastBenchmark("Fast Encoder", encoder1, data);

    //against basic encoder
    comp::Encoder encoder2(TICKS * 32);
    runBasicBenchmark("Basic Encoder", encoder2, data);

    //and adaptive encoder
    comp::AdaptiveEncoder encoder3(TICKS * 32);
    runBasicBenchmark("Adaptive Encoder", encoder3, data);

    return 0;

}