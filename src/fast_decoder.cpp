#include "decoder.hpp"

namespace comp
{
    /*
    Implementation for fast decoder
    Processes a compressed buffer in bulk
    */

    void decode_fast(FastDecoder &dec, double *__restrict__ output, size_t count)
    {
        for(size_t i{0}; i < count; i++)
        {
            dec.fastNxt(output[i]);
        }
    }
}