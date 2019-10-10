// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <div.hpp>

#include "../utils/random.hpp"
#include <benchmark/benchmark.h>

uint64_t udiv_native(uint64_t x, uint64_t y) noexcept;
uint64_t nop(uint64_t x, uint64_t y) noexcept;
uint64_t soft_div_unr_unrolled(uint64_t x, uint64_t y) noexcept;
uint64_t soft_div_unr(uint64_t x, uint64_t y) noexcept;

using namespace intx;

inline uint64_t udiv_by_reciprocal(uint64_t uu, uint64_t du) noexcept
{
    auto shift = __builtin_clzl(du);
    auto u = uint128{uu} << shift;
    auto d = du << shift;
    auto v = reciprocal_2by1(d);

    return udivrem_2by1(u, d, v).quot;
}


template <decltype(normalize<uint512>) NormalizeFn>
static void div_normalize(benchmark::State& state)
{
    auto u = uint512{{48882153453, 100324254353}, {4343242153453, 1324254353}};
    auto v = uint512{{48882100453, 16666654353}, {4343242156663, 1333354353}};

    for (auto _ : state)
    {
        benchmark::ClobberMemory();
        auto x = NormalizeFn(u, v);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK_TEMPLATE(div_normalize, normalize);

constexpr uint64_t neg(uint64_t x) noexcept
{
    return ~x;
}

inline uint64_t reciprocal_naive(uint64_t d) noexcept
{
    const auto u = uint128{~d, ~uint64_t{0}};
    uint64_t v;

#if _MSC_VER
    v = (u / d).lo;
#else
    uint64_t _;
    asm("divq %4" : "=d"(_), "=a"(v) : "d"(u.hi), "a"(u.lo), "g"(d));
#endif

    return v;
}

template <decltype(reciprocal_2by1) Fn>
static void div_unary(benchmark::State& state)
{
    constexpr auto top_bit = uint64_t{1} << 63;
    auto input = gen_uniform_seq(10);

    for (auto& i : input)
        i |= top_bit;

    benchmark::ClobberMemory();
    uint64_t x = 0;
    for (auto _ : state)
    {
        for (const auto& i : input)
            x ^= Fn(i);
    }
    benchmark::DoNotOptimize(x);
}
BENCHMARK_TEMPLATE(div_unary, neg);
BENCHMARK_TEMPLATE(div_unary, reciprocal_2by1);
BENCHMARK_TEMPLATE(div_unary, reciprocal_naive);

template <uint64_t DivFn(uint64_t, uint64_t)>
static void udiv64(benchmark::State& state)
{
    // Pick random operands. Keep the divisor small, because this is the worst
    // case for most algorithms.
    std::mt19937_64 rng{get_seed()};
    std::uniform_int_distribution<uint64_t> dist_x;
    std::uniform_int_distribution<uint64_t> dist_y(1, 200);

    constexpr size_t size = 1000;
    std::vector<uint64_t> input_x(size);
    std::vector<uint64_t> input_y(size);
    std::vector<uint64_t> output(size);
    for (auto& x : input_x)
        x = dist_x(rng);
    for (auto& y : input_y)
        y = dist_y(rng);

    for (auto _ : state)
    {
        for (size_t i = 0; i < size; ++i)
            output[i] = DivFn(input_x[i], input_y[i]);
        benchmark::DoNotOptimize(output.data());
    }

    if (DivFn == nop)
        return;

    // Check results.
    for (size_t i = 0; i < size; ++i)
    {
        if (output[i] != input_x[i] / input_y[i])
        {
            state.SkipWithError("wrong result");
            break;
        }
    }
}


BENCHMARK_TEMPLATE(udiv64, nop);
BENCHMARK_TEMPLATE(udiv64, udiv_by_reciprocal);
BENCHMARK_TEMPLATE(udiv64, udiv_native);
BENCHMARK_TEMPLATE(udiv64, soft_div_unr);
BENCHMARK_TEMPLATE(udiv64, soft_div_unr_unrolled);
