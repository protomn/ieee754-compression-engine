#include "encoder.hpp"

namespace comp
{
    /*
    Fast Encoder Implementation

    Processes large arrays of doubles using FastEncoder logic
    which inherits from the Encoder class.

    Reduces function call overhead by processing in bulk and
    utilizing built-in hardware instructions.
    */

    void encode_fast(const double *__restrict__ data, size_t count, FastEncoder &enc)
    {        
        for(size_t i{0}; i < count; ++i)
        {
            enc.fastAppend(data[i]);
        }
    }
}