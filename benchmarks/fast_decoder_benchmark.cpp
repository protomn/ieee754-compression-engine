#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include "encoder.hpp"
#include "decoder.hpp"

static double global{0.0};

void runFastDecBench(const std::string &title, comp::FastDecoder &dec, int cnt)
{
    double out{};
    double checksum{0.0};
    auto start = std::chrono::high_resolution_clock::now();

    for(int i{0}; i < cnt; ++i)
    {
        dec.fastNxt(out);
        checksum += out;
        asm volatile("" : "+r" (checksum));
    }

    auto end = std::chrono::high_resolution_clock::now();

    global = checksum;

    auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    double throughput = (cnt / (time_len / 1e6)) / 1e6;
    std::cout << title << " throughtput: " << throughput << " M ticks/sec.\n";
    std::cout << "Checksum: " << checksum << '\n';
}

void runBasicBenchmark(const std::string &name, comp::Decoder &decoder, int count)
{
    double out{};
    double checksum{0.0}; // to force the cpu to do work so it doesn't skip loops.
    auto start = std::chrono::high_resolution_clock::now();

    for(int i{0}; i < count; ++i)
    {
        if(decoder.next(out))
        {
            checksum += out; //Force the compiler to keep this call.
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    if(checksum == 42.0001) { std::cout << " "; }

    double throughput = (count / (time_len / 1e6)) / 1e6;
    std::cout << name << " throughtput: " << throughput << " M ticks/sec.\n";
    std::cout << name << " checksum: " << checksum << '\n';
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
    comp::Encoder encoder1(TICKS * 32); //Allocating extra space to avoid accidental overflow
    comp::AdaptiveEncoder encoder2(TICKS * 32);
    comp::FastEncoder encoder3(TICKS * 32);

    for(const auto &d : data)
    {
        encoder1.append(d);
        encoder2.append(d);
        encoder3.fastAppend(d);
    }

    encoder1.fin();
    encoder2.fin();
    encoder3.fin();

    comp::Decoder decoder1(encoder1.get_buffer().front(), encoder1.get_buffer().size());
    runBasicBenchmark("Basic Decoder", decoder1, TICKS);

    comp::AdaptiveDecoder decoder2(encoder2.get_buffer().front(), encoder2.get_buffer().size());
    runBasicBenchmark("Adaptive Decoder", decoder2, TICKS);

    comp::FastDecoder decoder3(encoder3.get_buffer().front(), encoder3.get_buffer().size());
    runFastDecBench("Fast Decoder", decoder3, TICKS);

    return 0;
}