#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <string>
#include "precision_encoder.hpp"
#include "precision_decoder.hpp"

namespace test
{
    struct TestResult
    {
        std::string name;
        bool passed;
        std::string error_msg;
        size_t values_tested;
        size_t errors;
    };  

    class DecoderTester
    {
        public:

            std::vector<TestResult> runTests()
            {
                std::vector<TestResult> results;

                results.push_back(basicDecoding());
                results.push_back(identicalValueDecomp());
                results.push_back(smallChanges());
                results.push_back(largeChanges());
                results.push_back(alternativeValueDecoding());
                results.push_back(gradualDriftDecoding());
                results.push_back(precisionBoundaryDecoding());
                results.push_back(adaptiveDecoderCorrectness());
                results.push_back(fastDecoderCorrectness());
                results.push_back(allDecoders());
                results.push_back(edgeCaseDecoding());
                results.push_back(largeDataDecoding());

                printResultSummary(results);

                return results;
            }

        private:

            TestResult basicDecoding()
            {
                TestResult result;
                result.name = "Basic Decoder Test";
                result.values_tested = 5;
                result.errors = 0;

                try
                {
                    std::vector<double> data = { 100.25, 100.30, 100.35, 100.40, 100.45 };

                    comp::PrecisionEncoder encoder(data.size() * 128);
                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult identicalValueDecomp()
            {
                TestResult result;
                result.name = "Identical Value Decompression Test";
                result.values_tested = 1000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 123.45);

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult smallChanges()
            {
                TestResult result;
                result.name = "Small Changes Decoding Test";
                result.values_tested = 100;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.00);

                    for(auto &val : data)
                    {
                        val += 0.01;
                    }

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult largeChanges()
            {
                TestResult result;
                result.name = "Large Changes Decoding Test";
                result.values_tested = 50;
                result.errors = 0;

                try
                {
                    std::vector<double> data = {
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    1.0, 1000.0, 0.001, 999999.0, 0.0000001,
                    };

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult alternativeValueDecoding()
            {
                TestResult result;
                result.name = "Alt Values Decoding Test";
                result.values_tested = 50;
                result.errors = 0;

                try
                {
                    std::vector<double> data;

                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        data.push_back(100.0);
                        data.push_back(200.0);
                    }

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult gradualDriftDecoding()
            {
                TestResult result;
                result.name = "Gradual Drift Decoding Test";
                result.values_tested = 1000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);
                    
                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        data[i] += 0.001 * i;
                    }

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult precisionBoundaryDecoding()
            {
                TestResult result;
                result.name = "Precision Boundary Decoding Test";
                result.values_tested = 20;
                result.errors = 0;

                try
                {
                    std::vector<double> data = {
                        0.0, 1.0, -1.0,
                        1e-308, 1e308, 
                        std::numeric_limits<double>::min(),
                        std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::epsilon(),
                        3.141592653589793, 2.718281828459045,
                        100.25, -100.25,
                        0.1 + 0.2, 1.0 / 3.0, 1.0 / 7.0,
                        std::sqrt(2.0), std::sqrt(3.0),
                        std::log(2.0), std::exp(1.0), std::sin(1.0)
                    };

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);

                        if(std::abs(decoded - data[i]) > 1e-15 * std::abs(data[i]))
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult adaptiveDecoderCorrectness()
            {
                TestResult result;
                result.name = "Adaptive Decoder Correctness Test";
                result.values_tested = 1000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        if(i % 5 == 0) { data[i] += 0.05; }
                    }

                    comp::AdaptivePrecisionEncoder apEncoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        apEncoder.append(d);
                    }

                    apEncoder.fin();

                    comp::AdaptivePrecisionDecoder apDecoder(apEncoder.get_buffer().front(), apEncoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        apDecoder.next(decoded);
                    
                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }
                
                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult fastDecoderCorrectness()
            {
                TestResult result;
                result.name = "Fast Decoder Correctness Test";
                result.values_tested = 500000;
                result.errors = 0;
                try
                {
                    std::vector<double> data(result.values_tested, 100.0);

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        if(i % 10 == 0) { data[i] += 0.05; }
                    }

                    comp::FastPrecisionEncoder fpEncoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        fpEncoder.fastAppend(d);
                    }

                    fpEncoder.fin();

                    comp::FastPrecisionDecoder fpDecoder(fpEncoder.get_buffer().front(), fpEncoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        fpDecoder.fastNxt(decoded);

                    if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }    

                printResult(result);
                return result;
            }

            TestResult allDecoders()
            {
                TestResult result;
                result.name = "All 3 Decoder Tests";
                result.values_tested = 500;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 50.0);

                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        data[i] += 0.01 * (i % 7);
                    }

                    comp::PrecisionEncoder baseEncoder(data.size() * 128);
                    comp::AdaptivePrecisionEncoder apEncoder(data.size() * 128);
                    comp::FastPrecisionEncoder fpEncoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        baseEncoder.append(d);
                        apEncoder.append(d);
                        fpEncoder.append(d);
                    }

                    baseEncoder.fin();
                    apEncoder.fin();
                    fpEncoder.fin();

                    comp::PrecisionDecoder baseDecoder(baseEncoder.get_buffer().front(), baseEncoder.get_buffer().size());
                    comp::AdaptivePrecisionDecoder apDecoder(apEncoder.get_buffer().front(), apEncoder.get_buffer().size());
                    comp::FastPrecisionDecoder fpDecoder(fpEncoder.get_buffer().front(), fpEncoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double d1{}, d2{}, d3{};
                        baseDecoder.next(d1);
                        apDecoder.next(d2);
                        fpDecoder.fastNxt(d3);
                    
                        if(std::abs(d1 - data[i]) > 1e-10 ||
                           std::abs(d2 - data[i]) > 1e-10 ||
                           std::abs(d3 - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }

                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult edgeCaseDecoding()
            {
                TestResult result;
                result.name = "Edge Case Decoding Test";
                result.values_tested = 10;
                result.errors = 0;

                try
                {
                    std::vector<double> data = {
                    0.0, 0.0, 0.0, -0.0,
                    1.0, 1.0, 1.0,
                    100.0, 100.0, 100.0
                    };

                    comp::PrecisionEncoder encoder(data.size() * 128);
                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }
                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);
                    
                        if(data[i] == 0.0)
                        {
                            if(decoded != 0.0)
                            {
                                result.errors++;
                            }
                        }
                        else if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }
                
                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            TestResult largeDataDecoding()
            {
                TestResult result;
                result.name = "Large Dataset Decoding Test";
                result.values_tested = 10000000; //10M values
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);

                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        if(i % 3 == 0) { data[i] += 0.01; }
                        if(i % 100 == 0) { data[i] += 1.0; }
                    }

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    comp::PrecisionDecoder decoder(encoder.get_buffer().front(), encoder.get_buffer().size());

                    for(size_t i{0}; i < data.size(); ++i)
                    {
                        double decoded{};
                        decoder.next(decoded);
                    
                        if(std::abs(decoded - data[i]) > 1e-10)
                        {
                            result.errors++;
                        }
                    }
                
                    result.passed = (result.errors == 0);
                }
                catch(const std::exception &e)
                {
                    result.passed = false;
                    result.error_msg = std::string("Exception: ") + e.what();
                }

                printResult(result);
                return result;
            }

            void printResult(const TestResult &result)
            {
                std::cout << std::left << std::setw(50) << result.name;
            
                if(result.passed)
                {
                    std::cout << std::right << std::setw(10) << "PASS";
                }
                else
                {
                    std::cout << std::right << std::setw(10) << "FAIL";
                }
            
                std::cout << std::setw(15) << result.values_tested << " values";
            
                if(!result.passed && !result.error_msg.empty())
                {
                    std::cout << "  (" << result.error_msg << ")";
                }
            
                if(result.errors > 0)
                {
                    std::cout << "  [" << result.errors << " errors]";
                }
            
                std::cout << '\n';
            }

            void printResultSummary(const std::vector<TestResult> &results)
            {
                std::cout << "\nDECODER TEST SUMMARY\n";
            
                int passed{0};
                int failed{0};
                size_t total_values{0};
            
                for(const auto &r : results)
                {
                    if(r.passed) { passed++; }
                    else { failed++; }
                    total_values += r.values_tested;
                }
            
                std::cout << "\nTotal Tests: " << results.size() << '\n';
                std::cout << "Passed:  " << passed << '\n';
                std::cout << "Failed: " << failed << '\n';
                std::cout << "Total Values: " << total_values << '\n';
            
                if(failed == 0)
                {
                    std::cout << "\nAll tests passed successfully.\n";
                }
                else
                {
                    std::cout << "\nSome tests failed. Debug.\n";
                }
            } 
        };
}

int main()
{
    test::DecoderTester tester;
    auto results = tester.runTests();

    for(const auto &r : results)
    {
        if(!r.passed)
        {
            return 1;
        }
    }
    
    return 0;
}