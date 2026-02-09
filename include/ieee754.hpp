#pragma once

#include <cstdint>
#include <bit>
#include <iostream>
#include <iomanip>

/*
This header breaks the floating point number into its raw bit array.
We use std::bit_cast because casting a double * or an uint64_t * results in UB. (aliasing violation)
*/

namespace comp
{
    //Constants - used to create masks and isolate parts of the double
    constexpr uint64_t SIGN_MASK = 0x8000000000000000ULL;
    constexpr uint64_t EXP_MASK = 0x7FF0000000000000ULL;
    constexpr uint64_t MANT_MASK = 0x000FFFFFFFFFFFFFULL;

    //This function compiles to 0 instructions
    [[nodiscard]] inline uint64_t to_uint64(double v) noexcept
    {
        return std::bit_cast<uint64_t>(v);
    }
    
    [[nodiscard]] inline double to_double(uint64_t v) noexcept
    {
        return std::bit_cast<double>(v);
    }

    //Bitwise introspection helper
    /*
    Returns:
        - Sign: 1
        - Exponent: 11
        - Mantissa: 52
    */
   struct DoubleParts
   {
        bool sign;
        uint16_t exponent;
        uint64_t mantissa;
   };

   /*
   The function below breaks down the double into it's bitwise parts
   as defined in struct DoubleParts

   Reserved purely for debugging.
   */

   inline DoubleParts breakParts(double v)
   {
        uint64_t u = to_uint64(v);

        return {
            (u & SIGN_MASK) != 0,
            static_cast<uint16_t>((u & EXP_MASK) >> 52),
            (u & MANT_MASK)
        };
   }

   //Hardware accelerated leading zeros
   //Req for calculating how many bits we can throw away

   [[nodiscard]] inline int count_lzs(uint64_t val) noexcept
   {
        return std::countl_zero(val); //Zero check not needed, std::countl_zero(0) already returns 64.
   }

   [[nodiscard]] inline int count_tzs(uint64_t val) noexcept
   {
        if(val == 0)
        {
            return 64;
        }

        return std::countr_zero(val);
   }

   //Debug util
   //To help visualize the bit pattern

   inline void print_bits(double v)
   {
        uint64_t u = to_uint64(v);

        std::cout << "Val: " << std::setw(10) << v << " | Hex: 0x"
        << std::hex << std::setw(16) << std::setfill('0') << u << std::dec <<
        " | Bits: ";

        for(int i{63}; i >= 0; --i)
        {
            if(i == 62)
            {
                std::cout << " "; // Separate the sign bit from the exponent bits
            }
            if(i == 51)
            {
                std::cout << " "; // Separate the exponent bits from the mantissa bits
            }
            std::cout << ((u >> i) & 1);
        }
        std::cout << '\n';
   }
}