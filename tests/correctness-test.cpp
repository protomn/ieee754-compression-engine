#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "encoder.hpp"
#include "decoder.hpp"

bool correctnessTest()
{
    const int TEST_SIZE = 20;
    std::vector<double> test;
    test.reserve(TEST_SIZE);

    double price{100.25};

    for(int i{0}; i < TEST_SIZE; ++i)
    {
        if(i % 5 == 0)
        {
            price += 0.05;
        }

        test.push_back(price);
    }

    comp::Encoder encoder1(TEST_SIZE * 32);
    comp::AdaptiveEncoder encoder2(TEST_SIZE * 32);
    comp::FastEncoder encoder3(TEST_SIZE * 32);

    for(const auto &d : test)
    {
        encoder1.append(d);
        encoder2.append(d);
        encoder3.fastAppend(d);
    }

    encoder1.fin();
    encoder2.fin();
    encoder3.fin();

    comp::Decoder decoder1(encoder1.get_buffer().front(), encoder1.get_buffer().size());
    comp::AdaptiveDecoder decoder2(encoder2.get_buffer().front(), encoder2.get_buffer().size());
    comp::FastDecoder decoder3(encoder3.get_buffer().front(), encoder3.get_buffer().size());

    std::cout << std::fixed << std::setprecision(10);

    bool all_pass = true;
    int errors_basic = 0;
    int errors_adaptive = 0;
    int errors_fast = 0;

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        double val_basic{}, val_adaptive{}, val_fast{};
        
        decoder1.next(val_basic);
        decoder2.next(val_adaptive);
        decoder3.fastNxt(val_fast);

        bool basic_ok = (std::abs(test[i] - val_basic) < 1e-10);
        bool adaptive_ok = (std::abs(test[i] - val_adaptive) < 1e-10);
        bool fast_ok = (std::abs(test[i] - val_fast) < 1e-10);

        if(!basic_ok) errors_basic++;
        if(!adaptive_ok) errors_adaptive++;
        if(!fast_ok) errors_fast++;

        if(i < 10 || !basic_ok || !adaptive_ok || !fast_ok)
        {
            std::cout << std::setw(5) << i << " | "
                      << std::setw(14) << test[i] << " | "
                      << std::setw(14) << val_basic << " | "
                      << std::setw(14) << val_adaptive << " | "
                      << std::setw(14) << val_fast << " | ";

            if(basic_ok && adaptive_ok && fast_ok)
            {
                std::cout << "PASS\n";
            }
            else
            {
                std::cout << "FAIL ";
                if(!basic_ok) std::cout << "[B] ";
                if(!adaptive_ok) std::cout << "[A] ";
                if(!fast_ok) std::cout << "[F] ";
                std::cout << "\n";
                all_pass = false;
            }
        }
    }

    if(TEST_SIZE > 10)
    {
        std::cout << "  ... (showing first 10 values only, unless errors)\n";
    }

    // Summary
    std::cout << "  SUMMARY\n";
    std::cout << "Total values tested: " << TEST_SIZE << "\n\n";

    std::cout << "Basic Encoder/Decoder:    ";
    if(errors_basic == 0)
        std::cout << "PASS (0 errors)\n";
    else
        std::cout << "FAIL (" << errors_basic << " errors)\n";

    std::cout << "Adaptive Encoder/Decoder: ";
    if(errors_adaptive == 0)
        std::cout << "PASS (0 errors)\n";
    else
        std::cout << "FAIL (" << errors_adaptive << " errors)\n";

    std::cout << "Fast Encoder/Decoder:     ";
    if(errors_fast == 0)
        std::cout << "PASS (0 errors)\n";
    else
        std::cout << "FAIL (" << errors_fast << " errors)\n";

    std::cout << "  CHECKSUM VERIFICATION\n";

    double expected_checksum = 0.0;
    for(const auto &d : test)
    {
        expected_checksum += d;
    }

    comp::Decoder decoder1b(encoder1.get_buffer().front(), encoder1.get_buffer().size());
    comp::AdaptiveDecoder decoder2b(encoder2.get_buffer().front(), encoder2.get_buffer().size());
    comp::FastDecoder decoder3b(encoder3.get_buffer().front(), encoder3.get_buffer().size());

    double checksum_basic = 0.0;
    double checksum_adaptive = 0.0;
    double checksum_fast = 0.0;

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        double val;
        decoder1b.next(val);
        checksum_basic += val;

        decoder2b.next(val);
        checksum_adaptive += val;

        decoder3b.fastNxt(val);
        checksum_fast += val;
    }

    std::cout << std::scientific << std::setprecision(10);
    std::cout << "Expected checksum:  " << expected_checksum << "\n";
    std::cout << "Basic checksum:     " << checksum_basic 
              << (std::abs(checksum_basic - expected_checksum) < 1e-6 ? " ✓" : " ✗") << "\n";
    std::cout << "Adaptive checksum:  " << checksum_adaptive 
              << (std::abs(checksum_adaptive - expected_checksum) < 1e-6 ? " ✓" : " ✗") << "\n";
    std::cout << "Fast checksum:      " << checksum_fast 
              << (std::abs(checksum_fast - expected_checksum) < 1e-6 ? " ✓" : " ✗") << "\n";

    std::cout << "  COMPRESSION STATS\n";
    
    size_t original_size = TEST_SIZE * sizeof(double);
    
    std::cout << "Original size:        " << original_size << " bytes\n";
    std::cout << "Basic compressed:     " << encoder1.sizeInBytes() << " bytes ("
              << (100.0 * encoder1.sizeInBytes() / original_size) << "%)\n";
    std::cout << "Adaptive compressed:  " << encoder2.sizeInBytes() << " bytes ("
              << (100.0 * encoder2.sizeInBytes() / original_size) << "%)\n";
    std::cout << "Fast compressed:      " << encoder3.sizeInBytes() << " bytes ("
              << (100.0 * encoder3.sizeInBytes() / original_size) << "%)\n";


    if(all_pass && errors_basic == 0 && errors_adaptive == 0 && errors_fast == 0)
    {
        std::cout << "ALL TESTS PASSED!\n";
        return true;
    }

    std::cout << "TESTS FAILED!\n";
    return false;
}

int main()
{
    return correctnessTest() ? 0 : 1;
}