#include "precision_encoder.hpp"
#include "precision_decoder.hpp"

/*
Adaptive Precision Decoder Implementation

Decodes data encoded by AdaptivePrecisionDecoder 

Encoding format is the same as the base PrecisionEncoder
so decoding logic is also the same. 
*/

namespace comp
{
    bool AdaptivePrecisionDecoder::next(double &out)
    {
        return PrecisionDecoder::next(out);
    }
}