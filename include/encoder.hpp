#pragma once

#include "ieee754.hpp"
#include "bitstream.hpp"

namespace comp
{
    /*
    Once we can write and read bits to and from the linear buffer, we need to be able to encode them

    I used the same algorithm used by Facebook Gorilla (and InfluxDB, etc)

    The idea is as follows:
        1. Calculate XOR: del = curr_val ^ prev_val
        2. Case 0 (Same value): If del == 0, write a single bit 0.
        3. Case 1 (Different value): Write bit 1
            - Calculate the leading and trailing zeros of del
            - If the lzs and tzs are close enough to the previous value's lzs and tzs.
              we reuse the prev length and position.
            - else we write a new control block specifying the new lz count and length.
    */

    class Encoder
    {
        public:

            explicit Encoder(size_t buffer_size_)
                : writer_(buffer_size_), prev_val(0),
                  prev_lzs(31), prev_len(64), first_val(true), first_del(true) { }

            virtual void append(double val)
            {
                uint64_t u64_val = to_uint64(val);

                //Store the first value as raw 64 bits
                if(first_val)
                {
                    writer_.write_bits(u64_val, 64);
                    prev_val = u64_val;
                    first_val = false;
                    return;
                }

                uint64_t del = u64_val ^ prev_val;

                if(del == 0)
                {
                    writer_.write_bits(0, 1);
                }
                else
                {
                    writer_.write_bits(1, 1);

                    int lzs = count_lzs(del);
                    int tzs = count_tzs(del);

                    if(lzs >= 32)
                    {
                        lzs = 31;
                    }

                    int len{64 - lzs - tzs};

                    if(len <= 0)
                    {
                        len = 1;
                    }

                    writer_.write_bits(lzs, 5);
                    writer_.write_bits(len, 6);

                    uint64_t meaningful_bits = (del >> tzs) & ((1ULL << len) - 1);
                    writer_.write_bits(meaningful_bits, len);

                    prev_val = u64_val;
                }
            }

            void fin()
            {
                /*
                To be called to finalize the stream
                */
               writer_.flush();
            }

            size_t sizeInBytes() const
            {
                return writer_.byteSize();
            }

            const linBuffer &get_buffer() const
            {
                return writer_.get_buffer();
            }

        protected:

            bitWriter writer_;
            uint64_t prev_val;
            bool first_val;
            bool first_del; //track if it's the first non-zero del
            int prev_lzs; //track previous leading zeros
            int prev_len; //track previous meaningful length
    };

    class AdaptiveEncoder : public Encoder
    {
        public:

            using Encoder::Encoder; //Inherit the constructor
            void append(double val) override;
    };
}