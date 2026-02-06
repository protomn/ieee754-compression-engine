#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <random>
#include "encoder.hpp"
#include "decoder.hpp"

/*
Test file to check if the Adaptive encoder-decoder
encode-decode bits without any loss.
*/

int main()
{
    const int TICKS = 1000000; //1M ticks
    std::vector<double> data;
    data.reserve(TICKS);

    //Generate random walk data
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(-0.05, 0.05);

    double price{150.25};

    for(int i{0}; i < TICKS; ++i)
    {
        price += dis(gen);
        data.push_back(price);
    }

    //Encode the data
    comp::AdaptiveEncoder encoder(TICKS * 1024); //Allocating extra-space to avoid overflow

    for(const auto &d : data)
    {
        encoder.append(d);
    }

    encoder.fin();

    //Decode the data
    const comp::linBuffer &buffer = encoder.get_buffer();
    comp::AdaptiveDecoder decoder(buffer.front(), buffer.size());

    int error{0};

    for(int j{0}; j < TICKS; ++j)
    {
        double decoded;
        if(!decoder.next(decoded))
        {
            std::cerr << "Tick: " << j << " Decoder failed to provide value.\n";
            error++;
            break;
        }

        //Bit level comparison via uint64 casts to avoid rounding quirks
        if(comp::to_uint64(decoded) != comp::to_uint64(decoded))
        {
            if(error < 5)
            {
                std::cerr << "Tick: " << j << " Mismatch! " <<
                "Expected: " << data[j] << " Got: " << decoded << ".\n";
            }
            error++;
        }
    }

    if(error == 0)
    {
        std::cout << "All " << TICKS << " ticks encoded and decoded successfully without loss.\n";
    }
    else
    {
        std::cout << "FAIL. Found " << error << " mismatches. Debug.\n";
    }

    return error == 0 ? 0 : 1;

}