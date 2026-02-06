#include <iostream>
#include <vector>
#include <iomanip>
#include "ieee754.hpp"
#include "bitstream.hpp"
#include "encoder.hpp"
#include "decoder.hpp"

int main()
{
    using namespace comp;

    //Setup data
    std::vector<double> test_data = { 150.25, 150.25, 150.26, 150.26, 150.27, 151.00 };

    //Compress data
    Encoder encoder(1024);

    for(const auto &d : test_data)
    {
        encoder.append(d);
    }

    encoder.fin();

    std::cout << "Original size: " << test_data.size() * sizeof(double) << " bytes.\n";
    std::cout << "Encoded size: " << encoder.sizeInBytes() << " bytes.\n";
    std::cout << "Compression ratio: " << (100 * encoder.sizeInBytes()) / (test_data.size() * sizeof(double)) << '\n';

    //Decompress data
    Decoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

    std::cout << std::fixed << std::setprecision(2);

    bool match{true};

    for(size_t i{0}; i < test_data.size(); ++i)
    {
        double decoded;
        if(decoder.next(decoded))
        {
            bool matched = (decoded == test_data[i]);
            std::cout << "Idx " << i << ": Original = " << test_data[i]
            << " | Decoder = " << decoded << " [" << (matched ? "PASS" : "FAIL") << "]\n";

            if(!matched)
            {
                match = false;
            }
        }
    }

    if(match)
    {
        std::cout << "Data successfully encoded and decoded without loss.\n";
    }
    else
    {
        std::cout << "Data integrity lost. Debug.\n";
    }

    return 0;
}