#pragma once

#include <array>

namespace Sobol
{
    constexpr uint32_t reverseBits(uint32_t x)
    {
        x = (((x & 0xaaaaaaaau) >> 1) | ((x & 0x55555555u) << 1));
        x = (((x & 0xccccccccu) >> 2) | ((x & 0x33333333u) << 2));
        x = (((x & 0xf0f0f0f0u) >> 4) | ((x & 0x0f0f0f0fu) << 4));
        x = (((x & 0xff00ff00u) >> 8) | ((x & 0x00ff00ffu) << 8));
        return ((x >> 16) | (x << 16));
    }

    // Based on code and data from: https://web.maths.unsw.edu.au/~fkuo/sobol/
    // The bits are also reversed at compile-time to optimize Owen-scrambling.
    constexpr auto generateBitReversedDirections()
    {
        // First 6 dimensions (2D-7D) from "new-joe-kuo-6.21201", since I only use 7.
        constexpr uint32_t s[] = { 1, 2, 3, 3, 4, 4 };
        constexpr uint32_t a[] = { 0, 1, 1, 2, 1, 4 };
        constexpr uint32_t m[][s[std::size(s) - 1]] =
        {
            { 1 },
            { 1, 3 },
            { 1, 3, 1 },
            { 1, 1, 1 },
            { 1, 1, 3, 3 },
            { 1, 3, 5, 13 }
        };

        auto V = std::array<std::array<uint32_t, 32>, std::size(s)>();
        for (uint32_t dim = 0; dim < V.size(); dim++)
        {
            for (uint32_t bit = 0; bit < s[dim]; bit++)
            {
                V[dim][bit] = m[dim][bit] << (32 - (bit + 1));
            }
            for (uint32_t bit = s[dim]; bit < 32; bit++)
            {
                V[dim][bit] = V[dim][bit - s[dim]] ^ (V[dim][bit - s[dim]] >> s[dim]);
                for (uint32_t k = 1; k < s[dim]; k++)
                {
                    V[dim][bit] ^= (((a[dim] >> (s[dim] - 1 - k)) & 1) * V[dim][bit - k]);
                }
            }
            for (uint32_t bit = 0; bit < 32; bit++)
            {
                V[dim][bit] = reverseBits(V[dim][bit]);
            }
        }
        return V;
    }

    constexpr auto BIT_REVERSED_DIRECTIONS = generateBitReversedDirections();

    template<int DIM>
    constexpr uint32_t bitReversedSample(uint32_t index)
    {
        static_assert(DIM >= 0 && DIM <= BIT_REVERSED_DIRECTIONS.size(), "Dimension is not included.");

        // First Sobol dimension is bit-reversed index.
        if constexpr (DIM == 0) return index;

        uint32_t x = 0u;
        for (int bit = 0; index; index >>= 1u, bit++)
        {
            x ^= (index & 1u) * BIT_REVERSED_DIRECTIONS[DIM - 1][bit];
        }
        return x;
    }
}