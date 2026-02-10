#pragma once

#include "ieee754.hpp"
#include "bitstream.hpp"

namespace comp
{
    /*
    Once we can write and read bits to and from the linear buffer, we need to be able to encode them

    I used the same algorithm used by Facebook Gorilla (and InfluxDB, etc)

    The idea is as follows:
        1. Store the first value as raw 64-bits.
        2. For subsequent values:
            - Compute XOR delta: del = value ^ prev_value
            - If del == 0: write a single control bit '0' (signals value hasn't changed)
            - If del == 1: write '1' (signals value has changed), then encode leading/trailing zeros
        
    The encoding format for the changed values is:
        - 1 bit: control bit (always 1)
        - 5 bits: leading zeros count [0 - 31]
        - 6 bits: meaningful bit length [1 - 63]
        - N bits: meaningful bits (where N = bit length)

    The encoder is optimized for time series data where subsequent values differ ever so slightly.
    */
    class Encoder
    {
        public:

            /*
            Parameter buffer_size_ is the maximum number of values to be encoded, NOT the number of bytes.

            The constructor internally allocates a buffer sized for the worst case
            based on the number of elements to be encoded:
                - First value: 64 bits
                - Subsequent values: up to 1+5+6+62 = 76 bits per value

            Recommended buffer_size_ * 128 for good safety margin.
            */
            explicit Encoder(size_t buffer_size_)
                : writer_(buffer_size_), prev_val(0),
                  prev_lzs(31), prev_len(64), first_val(true) { }

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
                        lzs = 31; //Clamped to 31 bits because leading zeros are encoded in 5 bits (2^5 - 1 = 31)
                    }

                    int len{64 - lzs - tzs};

                    if(len <= 0) //For when all bits are leading/trailing zeros
                    {
                        len = 1;
                    }
                    if(len > 63)
                    {
                        len = 63; //Clamping to 6-bit max;
                        /*
                        Why?
                            This is because the length is encoded in 6 bits (max = 2^6 - 1 = 63)
                            Without this len = 64 would truncate to 0, leading to UB in the decoder.
                        */
                    }

                    writer_.write_bits(lzs, 5);
                    writer_.write_bits(len, 6);

                    uint64_t meaningful_bits = (del >> tzs) & ((1ULL << len) - 1);
                    writer_.write_bits(meaningful_bits, len);

                    prev_val = u64_val;
                }
            }

            /*
            Finalize the compressed stream
            Flushes any remaining bits in the scratch buffer to the main buffer
            Must be called before accessing any compressed data!
            */
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
            int prev_lzs; //track previous leading zeros
            int prev_len; //track previous meaningful length
            bool first_val;
            [[maybe_unused]] bool first_del;
    };

    class AdaptiveEncoder : public Encoder
    {
        public:

            using Encoder::Encoder; //Inherit the constructor
            void append(double val) override;
    };

    class FastEncoder : public Encoder
    {
        public:
            
            using Encoder::Encoder;

            //using a non-vitual, always-inlined function for fastest execution
            __attribute__((always_inline))
            inline void fastAppend(double val)
            {
                uint64_t u64_val = to_uint64(val);

                if(__builtin_expect(first_val, 0))
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

                    //Calculate leading/trailing zeros using hardware instructions
                    int lzs = __builtin_clzll(del);
                    int tzs = __builtin_ctzll(del);

                    //Clamp lzs to 31 bits
                    if(lzs >= 32)
                    {
                        lzs = 31;
                    }

                    int len = 64 - lzs - tzs;

                    if(len <= 0)
                    {
                        len = 1;
                    }
                    if(len > 63)
                    {
                        len = 63; // Clamping to 6-bit max
                    }

                    uint64_t header = (static_cast<uint64_t>(lzs) << 6) | (len & 0x3F);
                    writer_.write_bits(header, 11);
                    writer_.write_bits(del >> tzs, len);

                    prev_val = u64_val;
                }
            }
    };
}