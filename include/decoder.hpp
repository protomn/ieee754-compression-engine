#include <iostream>
#include "ieee754.hpp"
#include "bitstream.hpp"

/*
Responsible to taking in the compressed buffer and building the double bit-by-bit.
*/

namespace comp
{
    class Decoder
    {
        public:

            Decoder(const uint64_t *data, size_t wordSize)
                : reader_(data, wordSize),
                  prev_val(0),
                  first_val(true) { }

            bool next(double &out)
            {
                //Handle the first_val sotred as raw 64-bits
                if(first_val)
                {
                    uint64_t u64_val = reader_.read_bits(64);
                    //If the stream is empty, the reader might read 0

                    prev_val = u64_val;
                    out = comp::to_double(u64_val);
                    first_val = false;
                    return true;
                }

                uint64_t control = reader_.read_bits(1); //Read the control bit

                if(control == 0)
                {
                    //If del results in 0, value is identical to the previous
                    out = comp::to_double(prev_val);
                    return true;
                }
                else
                {
                    //Value changed (del results in 1)
                    int lead = (int)reader_.read_bits(5);
                    int len = (int)reader_.read_bits(6);

                    //Read the meaningful bits
                    uint64_t meaningful_bits = reader_.read_bits(len);

                    //Reconstruct the XOR value
                    /*
                    During encoding, we right shifted by trailing zeros
                    where trail = 64 - lead - len
                    */

                    int trail = 64 - len - lead;
                    uint64_t del_rev = (meaningful_bits << trail);

                    //Reconstruct the original bit pattern
                    uint64_t reconstructed = del_rev ^ prev_val;

                    out = comp::to_double(reconstructed);
                    prev_val = reconstructed;
                    return true;
                }
            }

        private:

            bitReader reader_;
            uint64_t prev_val;
            bool first_val;
    };
}