#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include "encoder.hpp"
#include "decoder.hpp"

/*
Decoding is far faster than encoding (making this benchmark test seem moot).
This is because decoding does not involve any calculation of leading/trailing zeros.
Decoding is pure shifting and XORing operations, which the CPU can do extremely fast
resulting in sky-high throughputs.
This benchmarks thus only shows the read latency of the compressed data.
*/

void runBenchmark(const std::string &name, comp::Decoder &decoder, int count)
{
    double out{};
    auto start = std::chrono::high_resolution_clock::now();

    for(int i{0}; i < count; ++i)
    {
        decoder.next(out);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double throughput = (count / (time_len / 1e9)) / 1e6;
    std::cout << name << " throughtput: " << throughput << " M ticks/sec.\n";
}   

int main()
{
    const int TICKS = 10000000;
    std::vector<double> data(TICKS, 150.25);

    for(int i{0}; i < TICKS; ++i)
    {
        if(i % 10 == 0)
        {
            data[i] += 0.05;
        }
    }

    //Prep buffer
    comp::Encoder encoder1(TICKS * 1024); //Allocating extra space to avoid accidental overflow
    comp::AdaptiveEncoder encoder2(TICKS * 1024);

    for(const auto &d : data)
    {
        encoder1.append(d);
        encoder2.append(d);
    }
    
    //Flush buffers
    encoder1.fin();
    encoder2.fin();

    //Benchmark decoders

    //Basic decoder
    comp::Decoder decoder1(encoder1.get_buffer().front(), encoder1.get_buffer().size());
    runBenchmark("Basic Decoder", decoder1, TICKS);

    //Adaptive Decoder
    comp::AdaptiveDecoder decoder2(encoder2.get_buffer().front(), encoder2.get_buffer().size());
    runBenchmark("Adaptive Decoder", decoder2, TICKS);

    return 0;
}