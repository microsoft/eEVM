// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <intx/intx.hpp>

#include <gtest/gtest.h>

using namespace intx;

static_assert(uint128{2} + uint128{2} == 4, "");
static_assert(uint256{2} + uint256{2} == 4, "");
static_assert(uint512{2} + uint512{2} == 4, "");

static_assert(uint128{2} - uint128{1} == 1, "");
#ifndef _MSC_VER
// FIXME: Investigate "integer constant overflow" issue on MSVC.
static_assert(uint256{2} - uint256{1} == 1, "");
static_assert(uint512{2} - uint512{1} == 1, "");
#endif

static_assert(constexpr_mul(uint128{2}, uint128{2}) == 4, "");
static_assert(constexpr_mul(uint256{2}, uint256{2}) == 4, "");
static_assert(constexpr_mul(uint512{2}, uint512{2}) == 4, "");

static_assert(0_u256 == 0, "");
static_assert(-1_u256 == ~0_u256, "");
static_assert(
    115792089237316195423570985008687907853269984665640564039457584007913129639935_u256 == ~0_u256,
    "");
static_assert(
    0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff_u256 == ~0_u256, "");

static_assert(clz(uint128{0}) == 128, "");
static_assert(clz(uint128{1}) == 127, "");
static_assert(clz(uint256{0}) == 256, "");
static_assert(clz(uint256{1}) == 255, "");
static_assert(clz(uint512{0}) == 512, "");
static_assert(clz(uint512{1}) == 511, "");

TEST(uint256, div)
{
    uint256 a = 10001;
    uint256 b = 10;

    EXPECT_EQ(a / b, 1000);
}


template <typename T>
class uint_api : public testing::Test
{
};

using types = testing::Types<uint128, uint256, uint512>;
TYPED_TEST_CASE(uint_api, types);

TYPED_TEST(uint_api, constructor)
{
    auto i = int{-1};
    auto x = TypeParam{i};
    TypeParam y = i;
    auto z = TypeParam(i);

    EXPECT_EQ(x, y);
    EXPECT_EQ(x, z);
}

TYPED_TEST(uint_api, arithmetic)
{
    auto a = int{};
    auto b = uint64_t{};
    auto c = uint128{};
    auto d = uint256{};

    auto x = TypeParam{};

    EXPECT_TRUE(x + x == 0);
    EXPECT_TRUE(x + a == 0);
    EXPECT_TRUE(x + b == 0);
    EXPECT_TRUE(x + c == 0);
    EXPECT_TRUE(x + d == 0);

    EXPECT_TRUE(a + x == 0);
    EXPECT_TRUE(b + x == 0);
    EXPECT_TRUE(c + x == 0);
    EXPECT_TRUE(d + x == 0);

    EXPECT_TRUE(x - x == 0);
    EXPECT_TRUE(x - a == 0);
    EXPECT_TRUE(x - b == 0);
    EXPECT_TRUE(x - c == 0);
    EXPECT_TRUE(x - d == 0);

    EXPECT_TRUE(a - x == 0);
    EXPECT_TRUE(b - x == 0);
    EXPECT_TRUE(c - x == 0);
    EXPECT_TRUE(d - x == 0);

    EXPECT_TRUE(x * x == 0);
    EXPECT_TRUE(x * a == 0);
    EXPECT_TRUE(x * b == 0);
    EXPECT_TRUE(x * c == 0);
    EXPECT_TRUE(x * d == 0);

    EXPECT_TRUE(a * x == 0);
    EXPECT_TRUE(b * x == 0);
    EXPECT_TRUE(c * x == 0);
    EXPECT_TRUE(d * x == 0);
}


TYPED_TEST(uint_api, division)
{
    auto a = int{1};
    auto b = uint64_t{1};
    auto c = uint128{1};
    auto d = uint256{1};

    auto x = TypeParam{1};

    EXPECT_TRUE(x / x == 1);
    EXPECT_TRUE(x / a == 1);
    EXPECT_TRUE(x / b == 1);
    EXPECT_TRUE(x / c == 1);
    EXPECT_TRUE(x / d == 1);

    EXPECT_TRUE(a / x == 1);
    EXPECT_TRUE(b / x == 1);
    EXPECT_TRUE(c / x == 1);
    EXPECT_TRUE(d / x == 1);

    EXPECT_TRUE(x % x == 0);
    EXPECT_TRUE(x % a == 0);
    EXPECT_TRUE(x % b == 0);
    EXPECT_TRUE(x % c == 0);
    EXPECT_TRUE(x % d == 0);

    EXPECT_TRUE(a % x == 0);
    EXPECT_TRUE(b % x == 0);
    EXPECT_TRUE(c % x == 0);
    EXPECT_TRUE(d % x == 0);
}


TYPED_TEST(uint_api, bitwise)
{
    auto a = int{};
    auto b = uint64_t{};
    auto c = uint128{};
    auto d = uint256{};

    auto x = TypeParam{};

    EXPECT_TRUE((x | x) == 0);
    EXPECT_TRUE((x | a) == 0);
    EXPECT_TRUE((x | b) == 0);
    EXPECT_TRUE((x | c) == 0);
    EXPECT_TRUE((x | d) == 0);

    EXPECT_TRUE((a | x) == 0);
    EXPECT_TRUE((b | x) == 0);
    EXPECT_TRUE((c | x) == 0);
    EXPECT_TRUE((d | x) == 0);

    EXPECT_TRUE((x & x) == 0);
    EXPECT_TRUE((x & a) == 0);
    EXPECT_TRUE((x & b) == 0);
    EXPECT_TRUE((x & c) == 0);
    EXPECT_TRUE((x & d) == 0);

    EXPECT_TRUE((a & x) == 0);
    EXPECT_TRUE((b & x) == 0);
    EXPECT_TRUE((c & x) == 0);
    EXPECT_TRUE((d & x) == 0);

    EXPECT_TRUE((x ^ x) == 0);
    EXPECT_TRUE((x ^ a) == 0);
    EXPECT_TRUE((x ^ b) == 0);
    EXPECT_TRUE((x ^ c) == 0);
    EXPECT_TRUE((x ^ d) == 0);

    EXPECT_TRUE((a ^ x) == 0);
    EXPECT_TRUE((b ^ x) == 0);
    EXPECT_TRUE((c ^ x) == 0);
    EXPECT_TRUE((d ^ x) == 0);
}

TYPED_TEST(uint_api, comparison)
{
    auto a = int{3};
    auto b = uint64_t{5};
    auto c = uint128{7};
    auto d = uint256{11};

    auto x = TypeParam{};

    EXPECT_FALSE(x == a);
    EXPECT_FALSE(a == x);
    EXPECT_FALSE(x == b);
    EXPECT_FALSE(b == x);
    EXPECT_FALSE(x == c);
    EXPECT_FALSE(c == x);
    EXPECT_FALSE(x == d);
    EXPECT_FALSE(d == x);

    EXPECT_FALSE(x != x);
    EXPECT_TRUE(x != a);
    EXPECT_TRUE(a != x);
    EXPECT_TRUE(x != b);
    EXPECT_TRUE(b != x);
    EXPECT_TRUE(x != c);
    EXPECT_TRUE(c != x);
    EXPECT_TRUE(x != d);
    EXPECT_TRUE(d != x);

    EXPECT_FALSE(x < x);
    EXPECT_TRUE(x < a);
    EXPECT_TRUE(x < b);
    EXPECT_TRUE(x < c);
    EXPECT_TRUE(x < d);
    EXPECT_FALSE(a < x);
    EXPECT_FALSE(b < x);
    EXPECT_FALSE(c < x);
    EXPECT_FALSE(d < x);

    EXPECT_FALSE(x > x);
    EXPECT_FALSE(x > a);
    EXPECT_FALSE(x > b);
    EXPECT_FALSE(x > c);
    EXPECT_FALSE(x > d);
    EXPECT_TRUE(a > x);
    EXPECT_TRUE(b > x);
    EXPECT_TRUE(c > x);
    EXPECT_TRUE(d > x);

    EXPECT_TRUE(x <= x);
    EXPECT_TRUE(x <= a);
    EXPECT_TRUE(x <= b);
    EXPECT_TRUE(x <= c);
    EXPECT_TRUE(x <= d);
    EXPECT_FALSE(a <= x);
    EXPECT_FALSE(b <= x);
    EXPECT_FALSE(c <= x);
    EXPECT_FALSE(d <= x);

    EXPECT_TRUE(x >= x);
    EXPECT_FALSE(x >= a);
    EXPECT_FALSE(x >= b);
    EXPECT_FALSE(x >= c);
    EXPECT_FALSE(x >= d);
    EXPECT_TRUE(a >= x);
    EXPECT_TRUE(b >= x);
    EXPECT_TRUE(c >= x);
    EXPECT_TRUE(d >= x);
}

TYPED_TEST(uint_api, arithmetic_op_assignment)
{
    auto x = TypeParam{};

    EXPECT_EQ(x += 11, 11);
    EXPECT_EQ(x -= 4, 7);
    EXPECT_EQ(x *= 2, 14);
    EXPECT_EQ(x %= 8, 6);
    EXPECT_EQ(x /= 3, 2);

    EXPECT_EQ(x += x, 4);
    EXPECT_EQ(x -= x, 0);
    EXPECT_EQ(x += uint128{3}, 3);
    EXPECT_EQ(x *= 3u, 9);
    EXPECT_EQ(x /= x, 1);
    EXPECT_EQ(x %= x, 0);
}

TYPED_TEST(uint_api, bitwise_op_assignment)
{
    auto x = TypeParam{};

    EXPECT_EQ(x |= 0b1011, 0b1011);
    EXPECT_EQ(x &= 0b0110, 0b0010);
    EXPECT_EQ(x ^= 0b1110, 0b1100);
    EXPECT_EQ(x >>= 2, 0b0011);
    EXPECT_EQ(x <<= 1, 0b0110);

    x = 0;
    EXPECT_EQ(x |= uint128{0b1011}, 0b1011);
    EXPECT_EQ(x &= uint128{0b0110}, 0b0010);
    EXPECT_EQ(x ^= uint128{0b1110}, 0b1100);
    EXPECT_EQ(x >>= uint128{2}, 0b0011);
    EXPECT_EQ(x <<= uint128{1}, 0b0110);

    EXPECT_EQ(x |= x, x);
    EXPECT_EQ(x &= x, x);
    EXPECT_EQ(x ^= x, 0);
    EXPECT_EQ(x |= 1, 1);
    EXPECT_EQ(x <<= x, 2);
    EXPECT_EQ(x >>= x, 0);
}
