// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <intx/int128.hpp>

#include <gtest/gtest.h>

using namespace intx;

static_assert(clz_generic(uint32_t{0}) == 32, "");
static_assert(clz_generic(uint32_t{1}) == 31, "");
static_assert(clz_generic(uint32_t{3}) == 30, "");
static_assert(clz_generic(uint32_t{9}) == 28, "");

static_assert(clz_generic(uint64_t{0}) == 64, "");
static_assert(clz_generic(uint64_t{1}) == 63, "");
static_assert(clz_generic(uint64_t{3}) == 62, "");
static_assert(clz_generic(uint64_t{9}) == 60, "");


TEST(builtins, clz64_single_one)
{
    for (unsigned i = 0; i <= 63; ++i)
    {
        const auto input = (uint64_t{1} << 63) >> i;
        EXPECT_EQ(clz(input), i);
        EXPECT_EQ(clz_generic(input), i);
    }
}

TEST(builtins, clz64_two_ones)
{
    for (unsigned i = 0; i <= 63; ++i)
    {
        const auto input = ((uint64_t{1} << 63) >> i) | 1;
        EXPECT_EQ(clz(input), i);
        EXPECT_EQ(clz_generic(input), i);
    }
}

TEST(builtins, clz32_single_one)
{
    for (unsigned i = 0; i <= 31; ++i)
    {
        const auto input = (uint32_t{1} << 31) >> i;
        EXPECT_EQ(clz(input), i);
        EXPECT_EQ(clz_generic(input), i);
    }
}

TEST(builtins, clz32_two_ones)
{
    for (unsigned i = 0; i <= 31; ++i)
    {
        const auto input = ((uint32_t{1} << 31) >> i) | 1;
        EXPECT_EQ(clz(input), i);
        EXPECT_EQ(clz_generic(input), i);
    }
}

TEST(builtins, clz_zero)
{
    EXPECT_EQ(clz(uint32_t{0}), 32);
    EXPECT_EQ(clz_generic(uint32_t{0}), 32);
    EXPECT_EQ(clz(uint64_t{0}), 64);
    EXPECT_EQ(clz_generic(uint64_t{0}), 64);
}
