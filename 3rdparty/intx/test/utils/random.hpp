// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

/// @file
/// Helpers for generating random inputs for benchmarks.

#include <algorithm>
#include <random>

using seed_type = std::random_device::result_type;

static seed_type get_seed()
{
    static const auto seed = std::random_device{}();
    return seed;
}

/// Linear congruential generator for any integer type.
template <typename Int>
struct lcg
{
    Int state = 0;

    explicit lcg(seed_type seed) : state(seed)
    {
        // Run for some time to fill the state.
        for (int i = 0; i < 97; ++i)
            (*this)();
    }

    Int operator()()
    {
        return state = static_cast<Int>(Int(0x5851f42d4c957f2d) * state + Int(0x14057b7ef767814f));
    }
};


inline std::vector<uint64_t> gen_uniform_seq(size_t num)
{
    std::mt19937_64 rng{get_seed()};
    std::uniform_int_distribution<uint64_t> dist;
    std::vector<uint64_t> seq;
    std::generate_n(std::back_inserter(seq), num, [&]{ return dist(rng); });
    return seq;
}

