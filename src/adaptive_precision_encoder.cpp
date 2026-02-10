#include "precision_encoder.hpp"

/*
Adaptive Precision Encoder Implementation

An extension of the base PrecisionEncoder class.
Analyzes patterns in the data to predict optimal precision
levels and adaptively adjust the encoding strategy.

    - tracks precision history over a sliding window
    - detects when precision is stable v/ volatile
    - can adjust precision based on trends
    - better data compression for evolving precision patterns.
*/

namespace comp
{
    AdaptivePrecisionEncoder::AdaptivePrecisionEncoder(size_t buffer_size)
        : PrecisionEncoder(buffer_size),
          hist_idx(0),
          hist_count(0),
          av_precision(52),
          precision_vol(0),
          min_recent_pr(64),
          max_recent_pr(0),
          stable_precision_count(0),
          isStable(false),
          encoded_values(0),
          precision_reuses(0),
          precision_changes(0)
        {
            precision_history.fill(0);
            lzs_history.fill(0);  
        }


    void AdaptivePrecisionEncoder::append(double val)
    {
        uint64_t u64_val = to_uint64(val);

        if(first_val)
        {
            writer_.write_bits(u64_val, 64);
            prev_val = u64_val;
            first_val = false;
            return;
        }

        encoded_values++;

        uint64_t del = u64_val ^ prev_val;

        if(del == 0)
        {
            writer_.write_bits(0, 1);
            stable_precision_count++;
            return;
        }

        writer_.write_bits(1, 1);

        int lzs = count_lzs(del);
        int tzs = count_tzs(del);
        
        if(lzs >= 32) { lzs = 31; }

        int len = 64 - lzs - tzs;
        if(len <= 0) { len = 1; }
        if(len > 63) { len = 63; }

        //Adaptive Decision logic: should we reuse precision?
        if(reusePrecision(lzs, len))
        {
            writer_.write_bits(0, 1);

            uint64_t meaningful_bits = (del >> (64 - prev_lzs - significant_bits)) & ((1ULL << significant_bits) - 1);
            writer_.write_bits(meaningful_bits, significant_bits);

            precision_reuses++;
            stable_precision_count++;
        }
        else
        {
            //New precision level needed
            writer_.write_bits(1, 1);
            writer_.write_bits(lzs, 5);
            writer_.write_bits(len, 6);

            uint64_t meaningful_bits = (del >> tzs) & ((1ULL << len) - 1);
            writer_.write_bits(meaningful_bits, len);

            precision_changes++;
            stable_precision_count = 0; //Reset the stability counter

            //Update precision tracking
            prev_lzs = lzs;
            significant_bits = len;

            //Update history and stats
            updateHistory(lzs, len);
        }

        prev_val = u64_val;
    }

    /*
    Updates the circular buffer with latest precision values
    Recomputes stats every 4 updates to amortize cost.
    */
    void AdaptivePrecisionEncoder::updateHistory(int lzs, int len)
    {
        //Add to a circular buffer
        precision_history[hist_idx] = len;
        lzs_history[hist_idx] = lzs;

        //Fast modulo using bitwise AND (HISTORY must be a power of 2)
        hist_idx = (hist_idx + 1) & (HISTORY - 1);

        if(hist_count < HISTORY)
        {
            hist_count++;
        }

        if(hist_count % 4 == 0) //Amortize stats computation
        {
            recomputeStats();
        }
    }

    void AdaptivePrecisionEncoder::recomputeStats()
    {
        if(hist_count == 0)
        {
            return;
        }

        //Compute av precision and range
        int sum{0};
        int min_p{64}; //Start with maximum possible precision
        int max_p{0}; //Start with minimum possible precision

        for(int i{0}; i < hist_count; ++i)
        {
            int p = precision_history[i];
            sum += p;
            min_p = std::min(min_p, p);
            max_p = std::max(max_p, p);
        }

        av_precision = sum / hist_count;
        min_recent_pr = min_p;
        max_recent_pr = max_p;

        //Compute vol
        /*
        High volatility suggest precision is unstable, and frequetly requires new headers.
        */
        int range = max_p - min_p;
        precision_vol = av_precision > 0 ? (range * 100) / av_precision : 0;

        //Regime detection
        /*
        Stable regime: precision stays within a narrow predefined range
        Volatile regime: precision varies widely
        */

        if(precision_vol < 20)
        {
            isStable = true;
        }
        else if(precision_vol > 50)
        {
            isStable = false;
        }

        //Doesn't flip on borderline values (hysteresis)
    }

    bool AdaptivePrecisionEncoder::reusePrecision(int lzs, int len) const
    {
        //Check from base class
        bool fits = (lzs >= prev_lzs && len <= significant_bits);

        if(fits)
        {
            return true;
        }

        //Need sufficient history to make decisions
        if(hist_count < 4)
        {
            return fits;
        }

        if(isStable && stable_precision_count >= 8)
        {
            //Allow increase in precision within tolerance
            int pr_increase = len - significant_bits;
            int lz_ok = lzs >= (prev_lzs - 1);

            if(lz_ok && pr_increase <= TOL)
            {
                //data fits within the window with small expansion
                //cheaper to use a few extra bits than to write a whole new header
                return true;
            }

            int predicted = predictPrecision();

            //Check if matches predicted precision
            if(len <= predicted + TOL && lzs >= prev_lzs)
            {
                return true;
            }
        }
        return false; //In volatile regime
    }

    /*
    Predict next precision level using moving average
    Used in stable regimes to anticipate precision needs
    */
    int AdaptivePrecisionEncoder::predictPrecision() const
    {
        if(hist_count < 3)
        {
            return significant_bits;
        }

        //MA of recent precision values
        int sum{0};
        int count = std::min(hist_count, 8); //look at the last 8 values

        for(int i{0}; i < count; ++i)
        {
            /*
            Look back i steps in the circular buffer (HISTORY must be a power of 2).
            */
            int idx = (hist_idx - 1 - i - HISTORY) & (HISTORY - 1);
            sum += precision_history[idx];
        }

        return sum / count;
    }
}