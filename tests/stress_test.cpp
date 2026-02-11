#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <type_traits>

#include "encoder.hpp"
#include "decoder.hpp"
#include "precision_encoder.hpp"
#include "precision_decoder.hpp"
#include "ieee754.hpp"
#include <fast_float.h>

namespace fs = std::filesystem;

/*
CSV Parser to parse data generated via python script

Handles 2 formats:
    - price only (format1)
    - timestamp,price (format2)
*/

std::vector<double> parse_csv(const std::string &filename)
{
    std::vector<double> prices;
    prices.reserve(100000000);
    std::ifstream file(filename, std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("std::vector<double> parse_csv() - Failed to open file: " + filename);
    }

    std::string line;

    if(std::getline(file, line))
    {
        if(line.find("price") == std::string::npos && line.find('.') == std::string::npos)
        {

        }
        else
        {
            file.seekg(0);
        }
    }

    while(std::getline(file, line))
    {
        //Skip empty lines
        if(line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
        {
            continue;
        }

        const char *startPtr;
        const char *endPtr = line.data() + line.size();
        size_t lastComma = line.find_last_of(',');

        startPtr = (lastComma == std::string::npos) ? line.data() : line.data() + lastComma + 1;

        double val{};
        auto result = fast_float::from_chars(startPtr, endPtr, val);

        if(result.ec == std::errc())
        {
            prices.push_back(val);
        }
    }

    return prices;
}

/*
Analyze Data Characteristics
*/

void analyze_data(const std::vector<double> &data_)
{
    std::cout << "\nData Characteristics: \n";

    size_t n{data_.size()};

    //Price stats
    double min_price = *std::min_element(data_.begin(), data_.end());
    double max_price = *std::max_element(data_.begin(), data_.end());
    double sum{0.0};

    for(const auto &d : data_)
    {
        sum += d;
    }

    double mean = sum / n;

    double var{0.0};
    for(const auto &d : data_)
    {
        double diff = d - mean;
        var += diff * diff;
    }

    double deviation = std::sqrt(var / n);

    std::cout << "\nPrice Stats: \n";
    std::cout << "\nTotal ticks: " << n << '\n';
    std::cout << "\nPrice range: $" << std::fixed << std::setprecision(2)
              << min_price << " - $" << max_price << '\n';
    std::cout << "\nMean price: $" << mean << "\n";
    std::cout << "\nStd deviation: $" << deviation << '\n';

    //Change Stats
    size_t identical{0};
    size_t small_change{0};
    size_t med_change{0};
    double max_change{0.0};

    for(size_t i{1}; i < n; ++i)
    {
        double change = std::abs(data_[i] - data_[i - 1]);

        if(change == 0.0)
        {
            identical++;
        }
        else if(change <= 0.01)
        {
            small_change++;
        }
        else if(change <= 0.05)
        {
            med_change++;
        }
        
        max_change = std::max(max_change, change);
    }

    std::cout << "\nChange Patterns: \n";
    std::cout << "Identical consecutive: " << identical 
              << " (" << std::fixed << std::setprecision(1)
              << 100.0 * identical / (n-1) << "%)\n";

    std::cout << "Small changes (<=$0.01): " << small_change
              << " (" << 100.0 * small_change / (n-1) << "%)\n";

    std::cout << "Medium changes (<=$0.05): " << med_change
              << " (" << 100.0 * med_change / (n-1) << "%)\n";

    std::cout << "Maximum change: $" << std::setprecision(4) 
              << max_change << '\n';
}

/*
Test Encoding/Decoding Correctness
*/
template <typename Encoder, typename Decoder>
bool correctness(const std::vector<double> &data_,
                 const std::string &name_)
{
    std::cout << "\nTesting " << name_ << " correctness.\n";
    std::cout.flush();

    //Encode
    Encoder encoder(data_.size() * 128);
    for(const auto &d: data_)
    {
        encoder.append(d);
    }

    encoder.fin();

    Decoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

    for(size_t i{0}; i < data_.size(); ++i)
    {
        double decoded{};
        if(!decoder.next(decoded))
        {
            std::cout << "FAIL at index: " << i << '\n';
            return false;
        }

        if(std::abs(decoded - data_[i]) > 1e-9)
        {
            std::cout << "\nFAIL\n";
            std::cout << "Index " << i << ":\n";
            std::cout << "Expected: " << std::setprecision(20) << data_[i] << '\n';
            std::cout << "Got: " << std::setprecision(20) << decoded << '\n';
            std::cout << "Bits: " << std::hex << comp::to_uint64(data_[i])
                     << " vs " << comp::to_uint64(decoded) << std::dec << '\n';
            return false;
        }
    }

    std::cout << "PASS (lossless).\n";
    return true;
}

/*
Benchmark Encoder Performance
*/
template <typename Encoder>
void benchmarkEnc(const std::vector<double> &data_,
               const std::string &name_)
{
    const int RUNS{3};
    std::vector<double> times;
    size_t compressed_size{0};

    for(int run{0}; run < RUNS; ++run)
    {
        Encoder encoder(data_.size() * 128);

        auto start = std::chrono::high_resolution_clock::now();

        for(const auto &d : data_)
        {
            if constexpr(std::is_same_v<Encoder, comp::FastEncoder> || std::is_same_v<Encoder, comp::FastPrecisionEncoder>)
            {
                encoder.fastAppend(d);
            }
            else
            {
                encoder.append(d);
            }
        }

        encoder.fin();

        auto end = std::chrono::high_resolution_clock::now();
        auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        times.push_back(time_len);

        if(run == 0)
        {
            compressed_size = encoder.sizeInBytes();
        }
    }

    //Calculate stats
    std::sort(times.begin(), times.end());
    double median_time = times[RUNS / 2];
    double throughput = (median_time > 0) ? (data_.size() / median_time) : 0.0;

    size_t rawSize = data_.size() * sizeof(double);
    double compression_ratio = (double)rawSize / compressed_size;
    double savedSpace = 100.0 * (1.0 - 1.0 / compression_ratio);

    std::cout << std::left << std::setw(30) << name_;

    std::cout << std::right << std::setw(12) << std::fixed << std::setprecision(2) 
              << throughput << " M/s";

    std::cout << std::setw(15) << compressed_size << " bytes";

    std::cout << std::setw(12) << std::setprecision(2) << compression_ratio << ":1";

    std::cout << std::setw(12) << std::setprecision(1) << savedSpace << "%\n";
}

/*
Benchmark Decoder Performance
*/
template <typename Encoder, typename Decoder>
void benchmarkDec(const std::vector<double> &data_,
                  const std::string &name_)
{
    //Encode data
    Encoder encoder(data_.size() * 128);

    for(const auto &d : data_)
    {
        if constexpr(std::is_same_v<Encoder, comp::FastEncoder> || std::is_same_v<Encoder, comp::FastPrecisionEncoder>)
        {
            encoder.fastAppend(d);
        }
        else
        {
            encoder.append(d);
        }
    }

    encoder.fin();

    const int RUNS{3};
    std::vector<double> times;

    volatile double sink{0.0}; //To prevent compiler from optimizing out the loop

    for(int run{0}; run < RUNS; ++run)
    {
        Decoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

        auto start = std::chrono::high_resolution_clock::now();

        double out{};
        size_t count{0};

        if constexpr(std::is_same_v<Decoder, comp::FastDecoder> || std::is_same_v<Decoder, comp::FastPrecisionDecoder>)
        {
            while(decoder.fastNxt(out) && count < data_.size())
            {
                sink = out;
                count++;
            }
        }
        else
        {
            while(decoder.next(out) && count < data_.size())
            {
                sink = out;
                count++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto time_len = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        times.push_back(time_len);
    }

    //Calculate stats
    std::sort(times.begin(), times.end());
    double median_time = times[RUNS / 2];
    double throughput = (median_time > 0) ? (data_.size() / median_time) : 0.0;

    std::cout << std::left << std::setw(30) << name_;

    std::cout << std::right << std::setw(12) << std::fixed << std::setprecision(2) 
              << throughput << " M/s\n";
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <data.csv>\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " market_data.csv\n";
        return 1;
    }

    try
    {
        std::string data_path = argv[1]; 

        std::cout << "\nLoading data from: " << data_path << '\n';

        std::vector<double> data = parse_csv(data_path);
        std::cout << "Loaded " << data.size() << " data points.\n";

        try
        {
            data = parse_csv(data_path);
            std::cout << "Loaded " << data.size() << " data points.\n";
        }
        catch(const std::exception &e)
        {
            std::cerr << "ERROR: " << e.what() << '\n';
            return 1;
        }

        //Analyze data
        analyze_data(data);

        //Correctness tests
        std::cout << "\nTesting Correctness: \n";

        bool all_passed{true};

        all_passed &= correctness<comp::Encoder, comp::Decoder>(data, "Basic Encoder/Decoder");
        all_passed &= correctness<comp::AdaptiveEncoder, comp::AdaptiveDecoder>(data, "Adaptive Encoder/Adaptive Decoder");
        all_passed &= correctness<comp::PrecisionEncoder, comp::PrecisionDecoder>(data, "Precision Encoder/Precision Decoder");
        all_passed &= correctness<comp::AdaptivePrecisionEncoder, comp::AdaptivePrecisionDecoder>(data, "Adaptive Precision Encoder/Adaptive Precision Decoder");

        if(!all_passed)
        {
            std::cout << "\nCorrectness Test Failed\n";
            return 1;
        }

        std::cout << "\nCorrectness test passed.\nLossless compression/decompression verified.\n";

        //Performance benchmarking
        std::cout << "\nEncoder Performance Benchmarking\n";

        std::cout << std::left << std::setw(30) << "Encoder";
        std::cout << std::right << std::setw(12) << "Throughput";
        std::cout << std::setw(15) << "Compressed";
        std::cout << std::setw(12) << "Ratio";
        std::cout << std::setw(12) << "Saved\n";
        
        benchmarkEnc<comp::Encoder>(data, "Basic Encoder");
        benchmarkEnc<comp::AdaptiveEncoder>(data, "Adaptive Encoder");
        benchmarkEnc<comp::FastEncoder>(data, "Fast Encoder");
        benchmarkEnc<comp::PrecisionEncoder>(data, "Adaptive Encoder");
        benchmarkEnc<comp::AdaptivePrecisionEncoder>(data, "Adaptive Precision Encoder");
        benchmarkEnc<comp::FastPrecisionEncoder>(data, "Fast Precision Encoder");

        std::cout << "\nEncoder Performance Benchmarking\n";

        std::cout << std::left << std::setw(30) << "Decoder";
        std::cout << std::right << std::setw(12) << "Throughput\n";

        benchmarkDec<comp::Encoder, comp::Decoder>(data, "Basic Decoder");
        benchmarkDec<comp::AdaptiveEncoder, comp::AdaptiveDecoder>(data, "Adaptive Decoder");
        benchmarkDec<comp::FastEncoder, comp::FastDecoder>(data, "Fast Decoder");
        benchmarkDec<comp::PrecisionEncoder, comp::PrecisionDecoder>(data, "Precision Encoder");
        benchmarkDec<comp::AdaptivePrecisionEncoder, comp::AdaptivePrecisionDecoder>(data, "Adaptive Precision Decoder");
        benchmarkDec<comp::FastPrecisionEncoder, comp::FastPrecisionDecoder>(data, "Fast Precision Encoder");

        std::cout << "\nSummary: \n";
        std::cout << "All tests complete!\n";

        return 0;
    }
    catch(const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}