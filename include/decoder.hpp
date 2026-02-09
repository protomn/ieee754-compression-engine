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
                  prev_lzs(0),
                  prev_len(0),
                  first_val(true),
                  first_del(true) { }

            virtual ~Decoder() = default;

            virtual bool next(double &out)
            {
                //Handle the first_val sotred as raw 64-bits
                if(first_val)
                {
                    uint64_t u64_val = reader_.read_bits(64);
                    //If the stream is empty, the reader might read 0

                    prev_val = u64_val;
                    out = to_double(u64_val);
                    first_val = false;
                    return true;
                }

                uint64_t control = reader_.read_bits(1); //Read the control bit

                if(control == 0)
                {
                    //If del results in 0, value is identical to the previous
                    out = to_double(prev_val);
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

                    out = to_double(reconstructed);
                    prev_val = reconstructed;
                    return true;
                }
            }

        protected:

            bitReader reader_;
            uint64_t prev_val;
            int prev_lzs;
            [[maybe_unused]] int prev_len;
            bool first_val;
            [[maybe_unused]] bool first_del;
    };

    class AdaptiveDecoder : public Decoder
    {
        public:

            using Decoder::Decoder;
            bool next(double &out) override;
    };

    class FastDecoder : public Decoder
    {
        public:

            using Decoder::Decoder;

            __attribute__((always_inline))
            inline bool fastNxt(double &out)
            {
                if(__builtin_expect(first_val, 0))
                {
                    uint64_t u64_val = reader_.read_bits(64);
                    prev_val = u64_val;
                    out = to_double(u64_val);
                    first_val = false;
                    return true;
                }

                uint64_t control = reader_.read_bits(1);

                if(__builtin_expect(control == 0, 1)) //Check control bit
                {
                    out = to_double(prev_val);
                    return true;
                }
                
                //Value changed
                uint64_t header = reader_.read_bits(11);
                int lead = (int)((header >> 6) & 0x1F);
                int len = (int)(header & 0x3F);

                uint64_t meaningful_bits = reader_.read_bits(len);
                int trail = 64 - lead - len;

                uint64_t reconstructed = (meaningful_bits << trail) ^ prev_val;

                out = to_double(reconstructed);
                prev_val = reconstructed;
                return true;
            
            }
    };
}