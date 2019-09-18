// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <intx/int128.hpp>

#include "../utils/random.hpp"
#include <gtest/gtest.h>

using namespace intx;

namespace
{
struct arith_test_case
{
    uint128 x;
    uint128 y;
    uint128 sum;
    uint128 difference;
    uint128 product;
};

constexpr arith_test_case arith_test_cases[] = {
    {0, 0, 0, 0, 0},
    {0, 1, 1, 0xffffffffffffffffffffffffffffffff_u128, 0},
    {1, 0, 1, 1, 0},
    {1, 1, 2, 0, 1},
    {1, 0xffffffffffffffff, {1, 0}, 0xffffffffffffffff0000000000000002_u128, 0xffffffffffffffff},
    {0xffffffffffffffff, 1, {1, 0}, 0xfffffffffffffffe, 0xffffffffffffffff},
    {0xffffffffffffffff, 0xffffffffffffffff, 0x1fffffffffffffffe_u128, 0,
        0xfffffffffffffffe0000000000000001_u128},
    {0x8000000000000000, 0x8000000000000000, {1, 0}, 0, 0x40000000000000000000000000000000_u128},
    {0x18000000000000000_u128, 0x8000000000000000, {2, 0}, {1, 0},
        0xc0000000000000000000000000000000_u128},
    {0x8000000000000000, 0x18000000000000000_u128, {2, 0}, 0xffffffffffffffff0000000000000000_u128,
        0xc0000000000000000000000000000000_u128},
    {{1, 0}, 0xffffffffffffffff, 0x1ffffffffffffffff_u128, 1, {0xffffffffffffffff, 0}},
    {{1, 0}, {1, 0}, {2, 0}, 0, 0},
};


struct div_test_case
{
    uint128 x;
    uint128 y;
    uint128 quotient;
    uint128 reminder;
};

constexpr div_test_case div_test_cases[] = {
    {{0x8000000000000000, 1}, {0x8000000000000000, 1}, 1, 0},
    {{0x8000000000000000, 1}, {0x8000000000000001, 1}, 0, {0x8000000000000000, 1}},
    {{0x8000000000000001, 1}, {0x8000000000000000, 1}, 1, {1, 0}},
    {{0x8000000000000000, 2}, {0x8000000000000000, 1}, 1, 1},
    {{0x8000000000000000, 1}, {0x8000000000000000, 2}, 0, {0x8000000000000000, 1}},
    {{1, 5}, 7, 0x2492492492492493, 0},
    {{1, 5}, {2, 7}, 0, {1, 5}},
    {{0xffffffffffffffff, 0xffffffffffffffff}, {1, 0xffffffffffffffff}, 0x8000000000000000,
        0x7fffffffffffffff},
    {{0xee657725ff64cd48, 0xb8fe188a09dc4f78}, 3, {0x4f7727b7552199c2, 0xe854b2d8adf41a7d}, 1},
    {{0xbaf3f54fc23ec50a, 0x8db107aae7021a11}, {1, 0xa8d309c2d1c0a3ab}, 0x70a8814961f0fe6e,
        0x8c95107768881c97},
    {{0x9af3f54fc23ec50a, 0x8db107aae7021a11}, {0xb5967a16d599854c, 0xa8d309c2d1c0a3ab}, 0,
        {0x9af3f54fc23ec50a, 0x8db107aae7021a11}},
    {{0x395df916dfd1b5e, 0xe7e47d96b32ef2d5}, {0x537e3fbc5318dbc0, 0x38ae7c47ce8a620f}, 0,
        {0x395df916dfd1b5e, 0xe7e47d96b32ef2d5}},
    {0x6dfed7df267e7ed0, {0xb022a1a70b9cfbdf, 0xea31f3f6afbe6882}, 0, 0x6dfed7df267e7ed0},
    {{0x62cbd6bebd625e29, 0}, {0x525661ea1ecad583, 0x39cb5e652a3a0656}, 1,
        {0x107574d49e9788a5, 0xc634a19ad5c5f9aa}},
    {{0x657725ff64cd486d, 0xb8fe188a09dc4f78}, 0xb92974ae3bfad96a, 0x8c488dc0d0453a78,
        0x41bb80845d7261c8},
    {{0x657725ff64cd48, 0xb8fe188a09dc4f78}, 0xb92974ae3bfad96a, 0x8c488dc0d0453a,
        0xa25244a3b04d7b74},
    {{0x657725ff64cd486d, 0xb8fe188a09dc4f78}, 0xb92974ae3bfad9, {0x8c, 0x488dc0d0453ac8a9},
        0x955de0d6202e37},
    {{0xb1440f0ef70d4ef1, 0x2457e03b7d2cf0ac}, {0x5834d9e467cf238b, 0}, 2,
        {0xda5b46276f07db, 0x2457e03b7d2cf0ac}},
    {{0xb1440f0ef70d4ef1, 0x2457e03b7d2cf0ac}, {0x2000, 0}, 0x58a207877b86a,
        {0xef1, 0x2457e03b7d2cf0ac}},
    {0, {0xbb6376e43a291fef, 0xfee012e52194af52}, 0, 0},
    {0xdb7bf0efd05668d4, 0x510734f5eaa31a26, 2, 0x396d8703fb103488},
    {0xba8221b60d12e7c8, {0x7dfb4ff3ec1e7777, 0}, 0, 0xba8221b60d12e7c8},
    {{0x6558ce6e35a99381, 0}, 0xa08b35664c6dd38e, 0xa19b159fa722dbc7, 0x6a1e234ee7ca129e},
    {{0xac163d152c4f2345, 0}, {0x1b2e2e4a4c2227ff, 0}, 6, {0x90127576382334b, 0}},
    {0, 0x2f7c95d0092581f6, 0, 0},
    {0, {0x7c0ee5320345187, 0}, 0, 0},
    {{1, 0xe7e47d96b32ef2d5}, {0x537e3fbc5318dbc0, 0}, 0, {1, 0xe7e47d96b32ef2d5}},
    {{0x657725ff64cd486d, 0xb8fe188a09dc4f78}, 0x2000000000000000, {3, 0x2bb92ffb266a436d},
        0x18fe188a09dc4f78},
    {{0x9af3f54fc23ec50a, 0x8db107aae7021a11}, 1, {0x9af3f54fc23ec50a, 0x8db107aae7021a11}, 0},
    {{0x1313131313131313, 0x20}, {0x1313131313131313, 0x1313131313134013}, 0,
        {0x1313131313131313, 0x20}},
    {{0xffffffffffffffff, 0xff00000000002100}, {0xffff, 0xffffffffffffffff}, 0xffffffffffff,
        {0xffff, 0xff010000000020ff}},
    {{0x6904e619043deb6a, 0x4ee02431db62d7dd}, {0x8d86ba8220cd85d, 0xc92328ed07f1628}, 0xb,
        {0x7b845df8db09f6a, 0xc497f80ee5ece425}},
    {{0xf78a73117fbdc259, 0xc1626df64c827943}, {0x80053a19fc39bd5, 0x3aa770769c7e0f16}, 0x1e,
        {0x780a620c6d17f5c, 0xe1c3400ff5bcb4af}},
    {{0xe8e0eae8e8e8e8e5, 0xfffc000800000009}, 0x800091000e8e8, {0x1d1b, 0xfc6365e50bec6eb0},
        0x16a8bed6c3089},
    {{0xe8e0eae8e8e8e8e5, 0xfffc000000000000}, 0x800090000e8e8, {0x1d1b, 0xfc9d9d9c540368c6},
        0xcf3ac5f59c90},
    {{0xffffffffffffffff, 0xff3f060e0e0e7a0a}, {0x10, 0x401353ff}, 0xfffffffffbfecac,
        0xf4f0fb98361f6b6},
    {{0xffffffffffffffff, 0xf000000000000000}, {1, 0x40000000}, 0xffffffffc0000000, 0},
    {{0xffffffffffffffff, 0xf000000000000000}, {1, 0x80000000}, 0xffffffff80000000,
        0x3000000000000000},
    {{0xf0f0f0f0f0f0f0f, 0xf0f0f0f0f0f8f01}, {0xf0f0f0f0f0f0f0f, 0xf0f0f0f0f0f0f0f}, 1, 0x7ff2},
    {{0xdac7fff9ffd9e132, 0x2626262626262600}, 0xd021262626262626, {1, 0xd1a094108c5da55},
        0x6f386ccc73c11f62},
    {{0x100000000000004, 0xff00000000a20000}, 0x100000000000000, {1, 0x4ff}, 0xa20000},
};
}  // namespace

void static_test_comparison()
{
    constexpr uint128 zero;
    constexpr uint128 zer0 = 0;
    constexpr uint128 one = 1;

    static_assert(zero == 0, "");
    static_assert(zero != 1, "");
    static_assert(one > 0, "");
    static_assert(one >= 0, "");
    static_assert(zero >= 0, "");
    static_assert(zero < 1, "");
    static_assert(zero <= 1, "");
    static_assert(zero <= 0, "");

    static_assert(zero == zer0, "");
    static_assert(zero != one, "");
    static_assert(one > zero, "");
    static_assert(one >= zero, "");
    static_assert(zero >= zer0, "");
    static_assert(zero < one, "");
    static_assert(zero <= one, "");
    static_assert(zero <= zer0, "");

    constexpr auto zero_one = uint128{0, 1};
    constexpr auto one_zero = uint128{1, 0};

    static_assert(!(zero_one == one_zero), "");
    static_assert(zero_one != one_zero, "");
    static_assert(zero_one < one_zero, "");
    static_assert(zero_one <= one_zero, "");
    static_assert(!(zero_one > one_zero), "");
    static_assert(!(zero_one >= one_zero), "");
}

void static_test_bitwise_operators()
{
    constexpr uint128 x{0x5555555555555555, 0x5555555555555555};
    constexpr uint128 y{0xaaaaaaaaaaaaaaaa, 0xaaaaaaaaaaaaaaaa};
    constexpr uint128 one = 1;
    constexpr uint128 zero = 0;

    static_assert((x | one) == x, "");
    static_assert((y | one) == uint128{0xaaaaaaaaaaaaaaaa, 0xaaaaaaaaaaaaaaab}, "");

    static_assert((x & one) == one, "");
    static_assert((y & one) == zero, "");

    static_assert((x ^ zero) == x, "");
    static_assert((x ^ one) == uint128{0x5555555555555555, 0x5555555555555554}, "");

    static_assert(~x == y, "");
}

void static_test_explicit_conversion_to_bool()
{
    static_assert(uint128{1, 0}, "");
    static_assert(uint128{0, 1}, "");
    static_assert(uint128{1, 1}, "");
    static_assert(!uint128{0, 0}, "");
    static_assert(!static_cast<bool>(uint128{0, 0}), "");
}

#ifndef _MSC_VER
// FIXME: Investigate "integer constant overflow" issue on MSVC.
void static_test_arith()
{
    constexpr auto a = uint128{0x8000000000000000};
    constexpr auto s = a + a;
    static_assert(s == uint128{1, 0}, "");
    static_assert(s - a == a, "");
    static_assert(s - 0 == s, "");
    static_assert(s + 0 == s, "");
    static_assert(-uint128(1) == uint128{0xffffffffffffffff, 0xffffffffffffffff}, "");
    static_assert(0 - uint128(2) == uint128{0xffffffffffffffff, 0xfffffffffffffffe}, "");
    static_assert(uint128(13) - 17 == uint128{0xffffffffffffffff, 0xfffffffffffffffc}, "");

    static_assert(-a == (~a + 1), "");
    static_assert(+a == a, "");
}
#endif

void static_test_numeric_limits()
{
    static_assert(!std::numeric_limits<uint128>::is_signed, "");
    static_assert(std::numeric_limits<uint128>::is_integer, "");
    static_assert(std::numeric_limits<uint128>::is_exact, "");
    static_assert(std::numeric_limits<uint128>::radix == 2, "");

    static_assert(std::numeric_limits<uint128>::digits10 == 38, "");
    static_assert(std::numeric_limits<uint128>::min() == 0, "");
    static_assert(std::numeric_limits<uint128>::max() == uint128{0} - 1, "");
}

TEST(int128, add)
{
    for (auto& t : arith_test_cases)
    {
        EXPECT_EQ(t.x + t.y, t.sum);
        EXPECT_EQ(t.y + t.x, t.sum);
    }
}

TEST(int128, sub)
{
    for (auto& t : arith_test_cases)
    {
        EXPECT_EQ(t.x - t.y, t.difference);
    }
}

TEST(int128, mul)
{
    for (auto& t : arith_test_cases)
    {
        EXPECT_EQ(t.x * t.y, t.product);
        EXPECT_EQ(t.y * t.x, t.product);
    }
}

TEST(int128, increment)
{
    constexpr auto IO = uint128{1, 0};
    constexpr auto Of = uint128{~uint64_t{0}};

    auto a = Of;
    EXPECT_EQ(++a, IO);
    EXPECT_EQ(a, IO);

    auto b = Of;
    EXPECT_EQ(b++, Of);
    EXPECT_EQ(b, IO);

    auto c = IO;
    EXPECT_EQ(--c, Of);
    EXPECT_EQ(c, Of);

    auto d = IO;
    EXPECT_EQ(d--, IO);
    EXPECT_EQ(d, Of);
}

TEST(int128, shl)
{
    constexpr uint128 x = 1;
    for (unsigned s = 0; s < 127; ++s)
        EXPECT_EQ(clz(x << s), 127 - s);

    static_assert((x << 128) == 0, "");
    static_assert((uint128(3) << 63) == uint128(1, uint64_t(1) << 63), "");
}

TEST(int128, shr)
{
    constexpr uint128 x = uint128(1) << 127;
    for (unsigned s = 0; s < 127; ++s)
        EXPECT_EQ(clz(x >> s), s);

    static_assert((x >> 128) == 0, "");
    static_assert((uint128(3, 0) >> 1) == uint128(1, uint64_t(1) << 63), "");
}

TEST(int128, div)
{
    int index = 0;
    for (auto& t : div_test_cases)
    {
        auto q = t.x / t.y;
        auto r = t.x % t.y;

        EXPECT_EQ(q, t.quotient) << index;
        EXPECT_EQ(r, t.reminder) << index;

        auto res = udivrem(t.x, t.y);
        EXPECT_EQ(res.quot, q) << index;
        EXPECT_EQ(res.rem, r) << index;

        index++;
    }
}

#ifdef __SIZEOF_INT128__
#pragma GCC diagnostic ignored "-Wpedantic"
using uint128_ty = unsigned __int128;

TEST(int128, arith_random_args)
{
    int c = 1000000;

    lcg<uint128> dist{get_seed()};

    while (c-- > 0)
    {
        auto x = dist();
        auto y = dist();

        auto s = x + y;
        auto d = x - y;
        auto p = x * y;
        auto q = x / y;
        auto r = x % y;

        auto expected_s = uint128{uint128_ty{x} + uint128_ty{y}};
        auto expected_d = uint128{uint128_ty{x} - uint128_ty{y}};
        auto expected_p = uint128{uint128_ty{x} * uint128_ty{y}};
        auto expected_q = uint128{uint128_ty{x} / uint128_ty{y}};
        auto expected_r = uint128{uint128_ty{x} % uint128_ty{y}};

        EXPECT_EQ(s, expected_s) << c;
        EXPECT_EQ(d, expected_d) << c;
        EXPECT_EQ(p, expected_p) << c;
        EXPECT_EQ(q, expected_q) << c;
        EXPECT_EQ(r, expected_r) << c;
    }
}
#endif

TEST(int128, literals)
{
    auto a = 340282366920938463463374607431768211455_u128;
    EXPECT_EQ(a, (uint128{0xffffffffffffffff, 0xffffffffffffffff}));

    EXPECT_THROW(340282366920938463463374607431768211456_u128, std::overflow_error);
    EXPECT_THROW(3402823669209384634633746074317682114550_u128, std::overflow_error);

    a = 0xffffffffffffffffffffffffffffffff_u128;
    EXPECT_EQ(a, (uint128{0xffffffffffffffff, 0xffffffffffffffff}));

    EXPECT_THROW(0x100000000000000000000000000000000_u128, std::overflow_error);

    // Binary literals 0xb... are not supported yet.
    EXPECT_THROW(operator""_u128("0b1"), std::invalid_argument);

    EXPECT_THROW(operator""_u128("123x456"), std::invalid_argument);
    EXPECT_THROW(operator""_u128("0xabcxdef"), std::invalid_argument);

    EXPECT_EQ(0xaBc123eFd_u128, 0xAbC123EfD_u128);
}

TEST(int128, to_string)
{
    EXPECT_EQ(to_string(uint128{33}, 33), "10");
    EXPECT_EQ(hex(uint128{7 * 16 + 1}), "71");
}

TEST(int128, umul_random)
{
    const auto inputs = gen_uniform_seq(10000);

    for (size_t i = 1; i < inputs.size(); ++i)
    {
        auto x = inputs[i - 1];
        auto y = inputs[i];

        auto generic = intx::constexpr_umul(x, y);
        auto best = intx::umul(x, y);

        EXPECT_EQ(generic.hi, best.hi) << x << " x " << y;
        EXPECT_EQ(generic.lo, best.lo) << x << " x " << y;
    }
}

TEST(int128, clz)
{
    EXPECT_EQ(clz(intx::uint128{0}), 128);
    for (unsigned i = 0; i < intx::uint128::num_bits; ++i)
    {
        const auto input = (intx::uint128{1} << (intx::uint128::num_bits - 1)) >> i;
        EXPECT_EQ(clz(input), i);
    }
}
