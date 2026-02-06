#include "encoder.hpp"
#include <iostream>

int main()
{
    comp::Encoder encoder(1024);

    //Test pattern

    encoder.append(150.25);
    encoder.append(150.25);
    encoder.append(150.26);

    encoder.fin(); //Flushes buffer

    std::cout << "3 doubles compressed to: " << encoder.sizeInBytes() << " bytes.\n";

    return 0;
}