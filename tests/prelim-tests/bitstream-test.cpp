#include "ieee754.hpp"
#include "bitstream.hpp"
#include <iostream>

int main()
{
    using namespace comp;

    bitWriter writer(1024);

    writer.write_bits(5, 3);
    writer.write_bits(300, 10);
    writer.write_bits(0xABCDEF123456789, 60);
    writer.flush();

    std::cout << "Written: " << writer.byteSize() << " bytes.\n";

    //Decode and read
    bitReader reader(writer.get_buffer().front(), writer.get_buffer().size());

    uint64_t val1 = reader.read_bits(3);
    uint64_t val2 = reader.read_bits(10);
    uint64_t val3 = reader.read_bits(60);

    std::cout << "Read 3 bits: " << val1 << '\n';
    std::cout << "Read 10 bits: " << val2 << '\n';
    std::cout << "Read 60 bits: 0x" << std::hex << val3 << '\n';

    if(val1 == 5 && val2 == 300 && val3 == 0xABCDEF123456789)
    {
        std::cout << "Bitstream logic works!";
    }
    else
    {
        std::cout << "Need to debug";
    }

    return 0;
}