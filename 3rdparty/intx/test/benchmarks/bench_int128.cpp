// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <intx/int128.hpp>

#include "../utils/gmp.hpp"
#include "../utils/random.hpp"
#include <benchmark/benchmark.h>

using namespace intx;

uint128 div_gcc(uint128 x, uint128 y) noexcept;

inline uint128 div_uint128(uint128 x, uint128 y) noexcept
{
    return x / y;
}

inline uint128 div_gmp(uint128 x, uint128 y) noexcept
{
    return gmp::udivrem(x, y).quot;
}

template <decltype(div_gcc) DivFn>
static void udiv128(benchmark::State& state)
{
    uint128 x = {0x537e3fbc5318dbc0, 0xe7e47d96b32ef2d5};
    uint128 y = {0x395df916dfd1b5e, 0x38ae7c47ce8a620f};

    for (auto _ : state)
    {
        benchmark::ClobberMemory();
        auto q = DivFn(x, y);
        benchmark::DoNotOptimize(q);
    }
}
BENCHMARK_TEMPLATE(udiv128, div_gcc);
BENCHMARK_TEMPLATE(udiv128, div_uint128);
BENCHMARK_TEMPLATE(udiv128, div_gmp);


template <decltype(div_gcc) DivFn>
static void udiv128_worst_shift(benchmark::State& state)
{
    uint128 x = {0xee657725ff64cd48, 0xb8fe188a09dc4f78};
    uint128 y = 3;

    for (auto _ : state)
    {
        benchmark::ClobberMemory();
        auto q = DivFn(x, y);
        benchmark::DoNotOptimize(q);
    }
}
BENCHMARK_TEMPLATE(udiv128_worst_shift, div_gcc);
BENCHMARK_TEMPLATE(udiv128_worst_shift, div_uint128);
BENCHMARK_TEMPLATE(udiv128_worst_shift, div_gmp);


template <decltype(div_gcc) DivFn>
static void udiv128_single_long(benchmark::State& state)
{
    uint128 x = {0x0e657725ff64cd48, 0xb8fe188a09dc4f78};
    uint128 y = 0xe7e47d96b32ef2d5;

    for (auto _ : state)
    {
        benchmark::ClobberMemory();
        auto q = DivFn(x, y);
        benchmark::DoNotOptimize(q);
    }
}
BENCHMARK_TEMPLATE(udiv128_single_long, div_gcc);
BENCHMARK_TEMPLATE(udiv128_single_long, div_uint128);
BENCHMARK_TEMPLATE(udiv128_single_long, div_gmp);


template <decltype(div_gcc) DivFn>
static void udiv128_single_long_shift(benchmark::State& state)
{
    uint128 x = {0x0e657725ff64cd48, 0xb8fe188a09dc4f78};
    uint128 y = 0x77e47d96b32ef2d5;

    for (auto _ : state)
    {
        benchmark::ClobberMemory();
        auto q = DivFn(x, y);
        benchmark::DoNotOptimize(q);
    }
}
BENCHMARK_TEMPLATE(udiv128_single_long_shift, div_gcc);
BENCHMARK_TEMPLATE(udiv128_single_long_shift, div_uint128);
BENCHMARK_TEMPLATE(udiv128_single_long_shift, div_gmp);


template <typename RetT, RetT (*MulFn)(uint64_t, uint64_t)>
static void umul128(benchmark::State& state)
{
    auto inputs = gen_uniform_seq(1000);
    benchmark::ClobberMemory();

    for (auto _ : state)
    {
        uint64_t alo = 0;
        uint64_t ahi = 0;
        for (size_t i = 0; i < inputs.size() - 1; ++i)
        {
            auto p = MulFn(inputs[i], inputs[i + 1]);
            alo ^= p.lo;
            ahi ^= p.hi;
        }
        benchmark::DoNotOptimize(alo);
        benchmark::DoNotOptimize(ahi);
    }
}
BENCHMARK_TEMPLATE(umul128, intx::uint128, intx::constexpr_umul);
BENCHMARK_TEMPLATE(umul128, intx::uint128, intx::umul);
