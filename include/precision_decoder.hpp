#pragma once

#include "ieee754.hpp"
#include "bitstream.hpp"

namespace comp
{
    /*
    Base Precision Decoder Class

    Decodes data compressed by PrecisionEncoder class.
    */

    class PrecisionDecoder
    {
        public: 

            PrecisionDecoder(const uint64_t *data_, size_t wordSize)
                : reader_(data_, wordSize),
                  prev_val(0),
                  prev_lzs(0),
                  significant_bits(52),
                  first_val(true) { }

            virtual ~PrecisionDecoder() = default;

            virtual bool next(double &out)
            {
                if(first_val)
                {
                    uint64_t u64_val = reader_.read_bits(64);
                    prev_val = u64_val;
                    out = to_double(u64_val);
                    first_val = false;
                    return true;
                }

                uint64_t control = reader_.read_bits(1);

                if(control == 0)
                {
                    out = to_double(prev_val);
                    return true;
                }

                //Value changed (check precision control bit)

                uint64_t precision_control = reader_.read_bits(1);

                if(precision_control == 0)
                {
                    //Reuse precision
                    uint64_t meaningful_bits = reader_.read_bits(significant_bits);

                    int trail = 64 - prev_lzs - significant_bits;
                    uint64_t del_rev = meaningful_bits << trail;

                    uint64_t reconstructed = del_rev ^ prev_val;
                    out = to_double(reconstructed);
                    prev_val = reconstructed;
                    return true;
                }
                else
                {
                    //New precision
                    int lead = (int)reader_.read_bits(5);
                    int len = (int)reader_.read_bits(6);

                    uint64_t meaningful_bits = reader_.read_bits(len);

                    int trail = 64 - len - lead;
                    uint64_t del_rev = meaningful_bits << trail;

                    uint64_t reconstructed = del_rev ^ prev_val;
                    out = to_double(reconstructed);
                    prev_val = reconstructed;

                    prev_lzs = lead;
                    significant_bits = len;
                    return true;
                }
            }

        protected:

            bitReader reader_;
            uint64_t prev_val;
            int prev_lzs;
            int significant_bits;
            bool first_val;

    };

    /*
    Adaptive Precision Decoder

    Decodes data encoded by class AdaptivePrecisionEncoder
    */

    class AdaptivePrecisionDecoder : public PrecisionDecoder
    {
        public:

            using PrecisionDecoder::PrecisionDecoder;
            bool next(double &out) override;
    };

    /**
    Fast Precision Decoder

    Optimized decoder with inlined operations.
     */

    class FastPrecisionDecoder : public PrecisionDecoder
    {
        public:

            using PrecisionDecoder::PrecisionDecoder;

            __attribute__((always_inline, hot))
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

                if(__builtin_expect(control == 0, 1))
                {
                    out = to_double(prev_val);
                    return true;
                }

                uint64_t precision_control = reader_.read_bits(1);

                if(precision_control == 0)
                {
                    uint64_t meaningful_bits = reader_.read_bits(significant_bits);
                    int trail = 64 - prev_lzs - significant_bits;
                    uint64_t reconstructed = (meaningful_bits << trail) ^ prev_val;

                    out = to_double(reconstructed);
                    prev_val = reconstructed;
                    return true;
                }
                else
                {
                    int lead = (int)reader_.read_bits(5);
                    int len = (int)reader_.read_bits(6);

                    uint64_t meaningful_bits = reader_.read_bits(len);
                    int trail = 64 - lead - len;
                    uint64_t reconstructed = (meaningful_bits << trail) ^ prev_val;

                    out = to_double(reconstructed);
                    prev_val = reconstructed;

                    prev_lzs = lead;
                    significant_bits = len;

                    return true;
                }
            }
    };
}