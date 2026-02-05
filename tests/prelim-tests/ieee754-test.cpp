#include "ieee754.hpp"
#include <iostream>

int main()
{
    using namespace comp;

    //Test Case 1: Simple Price Movement
    double price1{150.25};
    double price2{150.26};

    //Bit inspection
    print_bits(price1);
    print_bits(price2);

    //XOR Deltas
    /*
    The values may be numerically different
    but bitwise they're nearly identical.
    */
    uint64_t int1 = to_uint64(price1);
    uint64_t int2 = to_uint64(price2);

    uint64_t del = int1 ^ int2;

    //Delta hex
    std::cout << "Hex: 0x" << std::hex << del << std::dec << '\n';

    int lead = count_lzs(del);
    int trail = count_tzs(del);

    std::cout << "Leading zeros: " << lead << '\n';
    std::cout << "Trailing zeros: " << trail << '\n';
    
    //Number of useful bits
    std::cout << (64 - lead - trail) << '\n';

    return 0;
}