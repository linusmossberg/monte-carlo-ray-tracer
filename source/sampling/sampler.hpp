#pragma once

#include <array>
#include <random> // Only used for non-determenistic global seed

#include "sobol.hpp"
#include "../common/unrolled-loop.hpp"

#include "sampling.hpp"

// Hash-based Owen-scrambled Sobol sequence generator based on:
// Practical Hash-based Owen Scrambling - Brent Burley
// https://jcgt.org/published/0009/04/01/

struct Sampler
{
    Sampler() = delete;

    // Non-type template dimension-range and loop-unrolling is about 4% faster 
    // for my use, but probably doesn't work well for more Sobol dimensions.
    template<int START_DIM, int NUM_DIMS>
    static auto get()
    {
        std::array<double, NUM_DIMS> res;
        unrolledLoop<int, NUM_DIMS>([&](auto i)
        {
            constexpr int DIM = START_DIM + i;
            res[i] = scramble(Sobol::bitReversedSample<DIM>(shuffled_index), hashCombine(seed, hash(DIM))) * 0x1p-32;
        });
        return res;
    }

    static void initiate(uint32_t start_seed)
    {
        base_seed = hashCombine(global_seed, hash(start_seed));
    }

    static void setIndex(uint32_t index)
    {
        sequence = 0u;
        seed = base_seed;
        bit_reversed_index = Sobol::reverseBits(index);
        shuffled_index = index;
    }

    static void nextSequence()
    {
        seed = hashCombine(base_seed, hash(++sequence));
        shuffled_index = scramble(bit_reversed_index, seed);
    }

private:
    inline thread_local static uint32_t base_seed = 0u, seed = 0u, sequence = 0u,
                                        bit_reversed_index = 0u, shuffled_index = 0u;

    inline static const uint32_t global_seed = std::random_device{}();

    // nested_uniform_scramble, but mostly avoids the first bit-reversal.
    static constexpr uint32_t scramble(uint32_t bit_reversed_x, uint32_t seed)
    {
        // Improved Laine-Karras hash by Nathan Vegdahl
        // https://psychopath.io/post/2021_01_30_building_a_better_lk_hash
        bit_reversed_x ^= bit_reversed_x * 0x3d20adea;
        bit_reversed_x += seed;
        bit_reversed_x *= (seed >> 16) | 1;
        bit_reversed_x ^= bit_reversed_x * 0x05526c56;
        bit_reversed_x ^= bit_reversed_x * 0x53a22864;

        return Sobol::reverseBits(bit_reversed_x);
    }

    // 2-round constants with lowest bias from:
    // https://github.com/skeeto/hash-prospector
    static constexpr uint32_t hash(uint32_t x)
    {
        x ^= x >> 15;
        x *= 0xd168aaad;
        x ^= x >> 15;
        x *= 0xaf723597;
        x ^= x >> 15;
        return x;
    }

    // Boost hash combine
    static constexpr uint32_t hashCombine(uint32_t seed, uint32_t v)
    {
        return seed ^ (v + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
};