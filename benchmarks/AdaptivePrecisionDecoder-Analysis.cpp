#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include "encoder.hpp"
#include "precision_encoder.hpp"

/*
Adaptive Precision Encoder Analysis Tool

Tool to visualize how the AdaptivePrecisionEncoder makes decisions
about encoding data as compared to the base Encoder.
*/

namespace fs = std::filesystem;

namespace analysis
{
    struct EncodingDecision
    {
        double value;
        uint64_t xor_val;
        int lzs;
        int tzs;
        int precision;
        bool reused;
        int written;
    };      

    void analyze(const std::vector<double> &data)
    {
        std::cout << "\nAdaptive Precision Encoder Analysis.\n";

        comp::PrecisionEncoder baseEncoder(data.size() * 128);
        comp::AdaptivePrecisionEncoder apEncoder(data.size() * 128);

        for(const auto &d : data)
        {
            baseEncoder.append(d);
            apEncoder.append(d);
        }

        baseEncoder.fin();
        apEncoder.fin();

        std::cout << "\nCompression Comparison.\n";

        std::cout << std::left << std::setw(30) << "Metric" <<
                     std::right << std::setw(20) << "Base Precision" <<
                     std::setw(20) << "Adaptive Precision" <<
                     std::setw(20) << "Improvement" << '\n';

        size_t baseSize = baseEncoder.sizeInBytes();
        size_t apSize = apEncoder.sizeInBytes();
        size_t originalSize = data.size() * sizeof(double);

        double baseRatio = 100 * (baseSize / (double)originalSize);
        double apRatio = 100 * (apSize / (double)originalSize);
        double improvement = 100 * (double)(baseSize - apSize) / (double)baseSize;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::left << std::setw(30) << "Compressed Size (bytes)"
                  << std::right << std::setw(20) << baseSize
                  << std::right << std::setw(20) << apSize
                  << std::right << std::setw(20) << improvement << "%\n";

        std::cout << std::left << std::setw(30) << "Compression Ratio"
                  << std::right << std::setw(20) << baseRatio
                  << std::right << std::setw(20) << apRatio
                  << std::right << std::setw(20) << "-\n";

        std::cout << "Stats\n";

        std::cout << std::left << std::setw(40) << "Average Precision Ratio" 
                  << std::right << std::setw(15) << apEncoder.getAvPrecision() << '\n';

        std::cout << std::left << std::setw(40) << "Precision Volatility (%)"
                  << std::right << std::setw(15) << apEncoder.getPrecisionVol() << '\n';

        std::cout << std::left << std::setw(40) << "Precision Reuse Rate (%)"
                  << std::right << std::setw(15) << std::setprecision(1)
                  << (apEncoder.getPrecisionReuseRate() * 100) << "%\n";

        std::cout << "\nDetailed Behavior\n";

        std::cout << std::left << std::setw(8) << "Index"
                  << std::right << std::setw(15) << "Value"
                  << std::setw(12) << "XOR Bits"
                  << std::setw(10) << "Leading Zeros"
                  << std::setw(10) << "Trailing Zeros"
                  << std::setw(12) << "Precision"
                  << std::setw(18) << "Base Detection"
                  << std::setw(18) << "Adaptive Detection"
                  << '\n';

        //Re-encode first 50 values
        comp::PrecisionEncoder baseTracker(data.size() * 128);
        comp::AdaptivePrecisionEncoder apTracker(data.size() * 128);

        uint64_t prev_val{0};
        bool first{true};

        for(size_t i{0}; i < std::min(size_t(50), data.size()); ++i)
        {
            uint64_t u_val = comp::to_uint64(data[i]);

            if(first)
            {
                std::cout << std::left << std::setw(8) << i
                          << std::right << std::fixed << std::setprecision(6)
                          << std::setw(15) << data[i]
                          << std::setw(12) << '-'
                          << std::setw(10) << '-'
                          << std::setw(10) << '-'
                          << std::setw(12) << "64"
                          << std::setw(18) << "FIRST (64 bits)"
                          << std::setw(18) << "FIRST (64 bits)"
                          << '\n';

                prev_val = u_val;
                first = false;
                continue;
            }

            uint64_t xor_val = u_val ^ prev_val;

            if(xor_val == 0)
            {
                std::cout << std::left << std::setw(8) << i
                          << std::right << std::fixed << std::setprecision(6)
                          << std::setw(15) << data[i]
                          << std::setw(12) << '0'
                          << std::setw(10) << '-'
                          << std::setw(10) << '-'
                          << std::setw(12) << '0'
                          << std::setw(18) << "SAME (1 bit)"
                          << std::setw(18) << "SAME (1 bit)"
                          << '\n';
            }
            else
            {
                int lzs = comp::count_lzs(xor_val);
                int tzs = comp::count_tzs(xor_val);

                if(lzs >= 32) { lzs = 31; }
                int len = 64 - lzs - tzs;
                if(len <= 0) { len = 1; }

                std::cout << std::left << std::setw(8) << i
                          << std::right << std::fixed << std::setprecision(6)
                          << std::setw(15) << data[i]
                          << std::setw(12) << len
                          << std::setw(10) << lzs
                          << std::setw(10) << tzs
                          << std::setw(12) << len
                          << std::setw(18) << "NEW/REUSE"
                          << std::setw(18) << "ADAPTIVE"
                          << '\n';
            }

            prev_val = u_val;
        }

        //Export results to csv
        std::string folder = "../results";
        std::string filename = folder + "/adaptive_precision_analysis.csv";

        try
        {
            if(!fs::exists(folder))
            {
                fs::create_directories(folder);
                std::cout << "Created director: " << fs::absolute(folder) << '\n';
            }
        }
        catch(const fs::filesystem_error &fe)
        {
            std::cerr << "Filesystem error: " << fe.what() << '\n';
            return;
        }

        std::ofstream csv(filename);
        
        if(csv.is_open())
        {
            csv << "Index,Value,XOR_Bits,Leading_Zeros,Trailing_Zeros,Precision\n";

            prev_val = 0;
            first = true;

            for(size_t i{0}; i < data.size(); ++i)
            {
                uint64_t uval = comp::to_uint64(data[i]);

                if(first)
                {
                    csv << i << ',' << data[i] << ",64,0,0,64\n";
                    prev_val = uval;
                    first = false;
                    continue;
                }

                uint64_t xor_v = uval ^ prev_val;

                if(xor_v == 0)
                {
                    csv << i << ',' << data[i] << ",0,0,0,0\n";
                }
                else
                {
                    int lzs = comp::count_lzs(xor_v);
                    int tzs = comp::count_tzs(xor_v);

                    if(lzs >= 32) { lzs = 31; }
                    int len = 64 - lzs - tzs;
                    if(len <= 0) { len = 1; }

                    csv << i << ',' << data[i] << ',' << len << ',' << lzs << ',' << tzs << ',' << len << '\n';
                }

                prev_val = uval;
            }

            csv.close();
            std::cout << "\nAnalysis exported to: results//adaptive_precision_analysis.csv";
        }
    }
}

int main()
{
    const int TICKS = 10000;

    //Perforning Analysis of Generated Test Data
    std::vector<double> data(TICKS, 100.25);

    for(size_t i{0}; i < data.size(); ++i)
    {
        if(i % 5 == 0) { data[i] += 0.05; }
    }

    analysis::analyze(data);

    return 0;
}