#include <iostream>
#include <cmath>
#include <cassert>
#include <iomanip>
#include <vector>
#include <string>
#include <stdexcept>
#include <numeric>
#include <chrono>
#include "precision_encoder.hpp"

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

    class PrecisionTester
    {
        public:

            std::vector<TestResult> runTests()
            {
                std::vector<TestResult> results;

                //Run all tests
                results.push_back(basicEncoding());
                results.push_back(identicalValueCompression());
                results.push_back(smallChanges());
                results.push_back(largeChanges());
                results.push_back(alternativeValueEncoding());
                results.push_back(gradualDriftEncoding());
                results.push_back(precisionBoundaryEncoding());
                results.push_back(adaptiveVBaseSize());
                results.push_back(fastVBaseSize());
                results.push_back(allSizes());
                results.push_back(edgeCaseEncodings());
                results.push_back(largeDatasetEncoding());

                printResultSummary(results);

                return results;
            }

        private:

            TestResult basicEncoding()
            {
                TestResult result;
                result.name = "Basic Precision Encoder Test.";
                result.values_tested = 5;
                result.errors = 0;

                try
                {
                    std::vector<double> data = { 100.25, 100.30, 100.35, 100.40, 100.45 };

                    comp::PrecisionEncoder encoder(data.size() * 128);

                    for(const auto &val : data)
                    {
                        encoder.append(val);
                    }

                    encoder.fin();

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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

            TestResult identicalValueCompression()
            {
                TestResult result;
                result.name = "Identical Value Compression Test (High Compression).";
                result.values_tested = 10000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 123.45);

                    comp::PrecisionEncoder encoder(data.size() * 64);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    size_t compressed = encoder.sizeInBytes();
                    size_t original_size = data.size() * sizeof(double);
                    double ratio = 100.0 * compressed / original_size;

                    if(ratio > 5.0)
                    {
                        result.error_msg = "Poor compression: " + std::to_string(ratio) + "%";
                        result.errors++;
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
                result.name = "Small Change Test";
                result.values_tested = 100;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);

                    for(auto &val : data)
                    {
                        val += 0.01;
                    }

                    comp::AdaptivePrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }
                    
                    encoder.fin();

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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
                result.name = "Large Change Test";
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

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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

            TestResult alternativeValueEncoding()
            {
                TestResult result;
                result.name = "Alternative Value Encoding Test";
                result.values_tested = 100;
                result.errors = 0;

                try
                {
                    std::vector<double> data;
                    for(int i{0}; i < 50; ++i)
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

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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

            TestResult gradualDriftEncoding()
            {
                TestResult result;
                result.name = "Gradual Drift Encoding Test";
                result.values_tested = 1000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);
                    
                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        data[i] += 0.001 * i;
                    }

                    comp::AdaptivePrecisionEncoder encoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        encoder.append(d);
                    }

                    encoder.fin();

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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

            TestResult precisionBoundaryEncoding()
            {
                TestResult result;
                result.name = "Precision Boundary Encoding Test";
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

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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

            TestResult adaptiveVBaseSize()
            {
                TestResult result;
                result.name = "Adaptive v Base Encoder Compression Size";
                result.values_tested = 1000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);
                    
                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        if(i % 5 == 0)
                        {
                            data[i] += 0.01;
                        }
                    }

                    comp::PrecisionEncoder baseEncoder(data.size() * 128);
                    comp::AdaptivePrecisionEncoder apEncoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        baseEncoder.append(d);
                        apEncoder.append(d);
                    }

                    baseEncoder.fin();
                    apEncoder.fin();

                    if(baseEncoder.sizeInBytes() == 0 || apEncoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
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

            TestResult fastVBaseSize()
            {
                TestResult result;
                result.name = "Base v Fast Encoder Compression Size";
                result.values_tested = 1000;
                result.errors = 0;

                try
                {
                    std::vector<double> data(result.values_tested, 100.0);
                    
                    for(size_t i{0}; i < result.values_tested; ++i)
                    {
                        if(i % 5 == 0)
                        {
                            data[i] += 0.01;
                        }
                    }

                    comp::PrecisionEncoder baseEncoder(data.size() * 128);
                    comp::FastPrecisionEncoder fastEncoder(data.size() * 128);

                    for(const auto &d : data)
                    {
                        baseEncoder.append(d);
                        fastEncoder.fastAppend(d);
                    }

                    baseEncoder.fin();
                    fastEncoder.fin();

                    if(baseEncoder.sizeInBytes() == 0 || fastEncoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                        result.error_msg = "Encoder produced no output.";
                    }

                    if(baseEncoder.sizeInBytes() != fastEncoder.sizeInBytes())
                    {
                        result.errors++;
                        result.error_msg = "Size mismatch.";
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

            TestResult allSizes()
            {
                TestResult result;
                result.name = "All 3 Encoder Sizes";
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

                    if(baseEncoder.sizeInBytes() == 0 ||
                       apEncoder.sizeInBytes() == 0 ||
                       fpEncoder.sizeInBytes() == 0)
                    {
                        result.errors++;
                    }

                    if(baseEncoder.sizeInBytes() != fpEncoder.sizeInBytes())
                    {
                        result.errors++;
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

            TestResult edgeCaseEncodings()
            {
                TestResult result;
                result.name = "Edge Case Encoding Test";
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
                
                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
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

            TestResult largeDatasetEncoding()
            {
                TestResult result;
                result.name = "Large Dataset Encoding Test";
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

                    if(encoder.sizeInBytes() == 0)
                    {
                        result.errors++;
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

            void printResult(const TestResult &results)
            {
                std::cout << std::left << std::setw(50) << results.name;

                if(results.passed)
                {
                    std::cout << std::right << std::setw(10) << " PASS";
                }
                else
                {
                    std::cout << std::right << std::setw(10) << " FAIL";
                }

                std::cout << std::setw(15) << results.values_tested << " values";

                if(!results.passed && !results.error_msg.empty())
                {
                    std::cout << " (" << results.error_msg << ")";
                }

                std::cout << '\n';
            }

            void printResultSummary(const std::vector<TestResult> &results)
            {
                std::cout << "\n ENCODER TEST SUMMARY\n";

                int passed{0};
                int failed{0};

                size_t total_vals{0};

                for(const auto &r : results)
                {
                    if(r.passed) { passed++; }
                    else { failed++; }
                    total_vals += r.values_tested;
                }

                std::cout << "Total Tests: " << results.size() << '\n';
                std::cout << "Passed: " << passed << '\n';
                std::cout << "Failed: " << failed << '\n';
                std::cout << "Total values tested: " << total_vals << '\n';

                if(failed == 0)
                {
                    std::cout << "All tests passed successfully.\n";
                }
                else
                {
                    std::cout << "Some tests failed. Debug.\n";
                }
            }
    };
}

int main()
{
    test::PrecisionTester tester;

    auto start = std::chrono::high_resolution_clock::now();
    auto res = tester.runTests();

    for(const auto &r : res)
    {
        if(!r.passed)
        {
            return 1;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "\nTime taken to complete all tests: " << diff << " microseconds.\n";

    return 0;
}