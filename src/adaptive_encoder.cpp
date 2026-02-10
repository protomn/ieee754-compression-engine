#include "encoder.hpp"

/*
In this implementation file, I define the adaptive version of the append logic.
The key difference is if the current XOR fits inside the previous window
we only write 2 bits (10) instead of 12 (1 + 5 LZ bits + 6 len bits)
*/

namespace comp
{
    void AdaptiveEncoder::append(double val)
    {
        uint64_t u64_val = to_uint64(val);

        //Handle the first_val as raw 64 bits   
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
            writer_.write_bits(0, 1); // Same value
        }
        else
        {
            writer_.write_bits(1, 1); // Different value

            int lzs = count_lzs(del);
            int tzs = count_tzs(del);

            // Clamp lzs to 31 bits
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
                len = 63;
            }

            // Adaptive check: is the previous window reusable?
            if(!first_del && lzs >= prev_lzs && (lzs + len) <= (prev_lzs + prev_len))
            {
                /*
                Reuse previous encoding window if current XOR fits within it
                Saves 11 bits (5 lzs + 6 len) by writing only the control bit.
                */
                writer_.write_bits(0, 1); //Control bit 0

                uint64_t meaningful_bits = del >> (64 - prev_lzs - prev_len); //Shift del to align with previous window
                writer_.write_bits(meaningful_bits, prev_len);
            }
            else
            {
                // New window
                writer_.write_bits(1, 1); // Control bit 1
                writer_.write_bits(lzs, 5);
                writer_.write_bits(len, 6);

                uint64_t meaningful_bits = (del >> tzs) & ((1ULL << len) - 1);
                writer_.write_bits(meaningful_bits, len);

                //Update state for next data point
                prev_lzs = lzs;
                prev_len = len;
                first_del = false;
            }

            prev_val = u64_val;
        }
    }
}