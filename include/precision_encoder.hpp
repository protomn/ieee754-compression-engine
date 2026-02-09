#pragma once

#include "ieee754.hpp"
#include "bitstream.hpp"
#include "encoder.hpp"

#include <array>

namespace comp
{
    /*
    Precision Encoder

    In time-series datasets, often only the lower-order mantissa bits change
    frequently, while the higher order exponent bits remain stable.

    So I created a Precision Encoder that follows the following strategy:
        - Track which mantissa bits actually change between values.
        - Only encode the changing portion.
        - Use run-length encoding for precision levels.
    */

    class PrecisionEncoder
    {
        public:

            explicit PrecisionEncoder(size_t buffer_size)
                : writer_(buffer_size),
                  prev_val(0),
                  prev_lzs(0),
                  significant_bits(52), //We start with the assumption that all mantissa bits matter.
                  first_val(true) { }

            virtual ~PrecisionEncoder() = default;

            virtual void append(double val)
            {
                uint64_t u64_val = to_uint64(val);

                if(first_val)
                {
                    writer_.write_bits(u64_val, 64);
                    prev_val = u64_val;
                    first_val = false;
                    return;
                }

                uint64_t del = u64_val ^ prev_val;

                if(del == 0) //control bit is 0, value is same
                {
                    writer_.write_bits(0, 1);
                    return;
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

                    int len = 64 - lzs - tzs;

                    if(len <= 0)
                    {
                        len = 1;
                    }

                    //Check if the same precision level can be reused
                    if(lzs >= prev_lzs && len <= significant_bits)
                    {
                        //Reuse precision
                        writer_.write_bits(0, 1);

                        int trail = 64 - prev_lzs - significant_bits;
                        uint64_t meaningful_bits = (del >> trail) & ((1ULL << significant_bits) - 1);
                        writer_.write_bits(meaningful_bits, significant_bits);
                    }
                    
                    else
                    {
                        //New precision level needed.
                        writer_.write_bits(1, 1);
                        writer_.write_bits(lzs, 5);
                        writer_.write_bits(len, 6);

                        uint64_t meaningful_bits = (del >> tzs) & ((1ULL << len) - 1);
                        writer_.write_bits(meaningful_bits, len);

                        prev_lzs = lzs;
                        significant_bits = len;
                    }

                    prev_val = u64_val;
                }
            }

            void fin()
            {
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
            int prev_lzs;
            int significant_bits; // current precision level
            bool first_val;
    };

    /*
    Adaptive Precision Encoder

    Extention of the base PrecisionEncoder class
        - more sophisticated precision tracking
        - windowed precision analysis
        - dynamic adjustment
    */

    class AdaptivePrecisionEncoder : public PrecisionEncoder
    {
        public:

            explicit AdaptivePrecisionEncoder(size_t buffer_size);
            void append(double val) override;

            //Stats access
            int getAvPrecision() const { return av_precision; }
            int getPrecisionVol() const { return precision_vol; }
            double getPrecisionReuseRate() const
            {
                return encoded_values > 0 ? (double)precision_reuses / encoded_values : 0.0;
            }

        private:
            
            static constexpr int HISTORY = 16;
            static constexpr int TOL = 2;

            //History tracking
            std::array<int, HISTORY> precision_history;
            std::array<int, HISTORY> lzs_history;
            int hist_idx;
            int hist_count;

            //Stats
            int av_precision;
            int precision_vol;
            int min_recent_pr; // Minimum recent precision
            int max_recent_pr; // Maximum recent precision
            
            //Adaptive states
            int stable_precision_count;
            bool isStable;

            //Counters
            uint64_t encoded_values;
            uint64_t precision_reuses;
            uint64_t precision_changes;

            //Methods
            void updateHistory(int lzs, int len);
            void recomputeStats();
            bool reusePrecision(int lzs, int len) const;
            int predictPrecision() const;
    };

    /*
    Fast Precision Encoder
    Optimized with:
        - inline functions
        - hardware functions
        - branch prediction hints
        - minimal state tracking
     */

    class FastPrecisionEncoder : public PrecisionEncoder
    {
        public:

            using PrecisionEncoder::PrecisionEncoder;

            __attribute__((always_inline, hot))
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

                if(__builtin_expect(del == 0, 1))
                {
                    writer_.write_bits(0, 1);
                    return;
                }

                writer_.write_bits(1, 1); //Values are different

                int lzs = __builtin_clzll(del);
                int tzs = __builtin_ctzll(del);
                
                if(lzs >= 32)
                {
                    lzs = 31;
                }

                int len = 64 - lzs - tzs;

                if(len <= 0)
                {
                    len = 1;
                }

                // Check if we can reuse precision
                if(__builtin_expect(lzs >= prev_lzs && len <= significant_bits, 1))
                {
                    writer_.write_bits(0, 1);
                    
                    int trail = 64 - prev_lzs - significant_bits;
                    uint64_t meaningful_bits = (del >> trail) & ((1ULL << significant_bits) - 1);
                    writer_.write_bits(meaningful_bits, significant_bits);
                }

                else
                {
                    writer_.write_bits(1, 1);
                    writer_.write_bits(lzs, 5);
                    writer_.write_bits(len, 6);

                    uint64_t meaningful_bits = (del >> tzs) & ((1ULL << len) - 1);
                    writer_.write_bits(meaningful_bits, len);

                    prev_lzs = lzs;
                    significant_bits = len;
                }

                prev_val = u64_val;
            }
    };
}