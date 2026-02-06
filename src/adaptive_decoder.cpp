#include <iostream>
#include "decoder.hpp"

namespace comp
{
    bool AdaptiveDecoder::next(double &out)
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
            //Same value
            out = to_double(prev_val);
            return true;
        }
        
        uint64_t reuse = reader_.read_bits(1); // read the second control bit to check if window is reused

        if(reuse == 0)
        {
            //Reuse the previous window
            uint64_t meaningful_bits = reader_.read_bits(prev_len);
            int trail = 64 - prev_lzs - prev_len;
            uint64_t del_rev = (meaningful_bits << trail);

            uint64_t reconstructed = del_rev ^ prev_val;
            out = to_double(reconstructed);
            prev_val = reconstructed;
        }
        else
        {
            // New window
            int lead = (int)reader_.read_bits(5);
            int len = (int)reader_.read_bits(6);

            uint64_t meaningful_bits = reader_.read_bits(len);
            int trail = 64 - len - lead;
            uint64_t del_rev = (meaningful_bits << trail);

            uint64_t reconstructed = del_rev ^ prev_val;
            
            prev_lzs = lead;
            prev_len = len;
            first_del = false;

            out = to_double(reconstructed);
            prev_val = reconstructed;
        }

        return true;
    }
}