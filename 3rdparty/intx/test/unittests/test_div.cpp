// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <div.hpp>
#include <intx/intx.hpp>

#include <gtest/gtest.h>

using namespace intx;

TEST(div, normalize)
{
    uint512 u;
    uint512 v = 1;
    auto na = normalize(u, v);
    EXPECT_EQ(na.shift, 63);
    EXPECT_EQ(na.num_denominator_words, 1);
    EXPECT_EQ(na.num_numerator_words, 0);
    EXPECT_EQ(na.numerator, 0);
    EXPECT_EQ(na.numerator_ex, 0);
    EXPECT_EQ(na.denominator, v << 63);

    u = uint512{1414, 1313};
    v = uint512{12, 1212};
    na = normalize(u, v);
    EXPECT_EQ(na.shift, 60);
    EXPECT_EQ(na.num_denominator_words, 5);
    EXPECT_EQ(na.num_numerator_words, 5);
    EXPECT_EQ(na.numerator, u << 60);
    EXPECT_EQ(na.numerator_ex, 0);
    EXPECT_EQ(na.denominator, v << 60);

    u = uint512{3} << 510;
    v = uint256{0xffffffffffffffff, 1};
    na = normalize(u, v);
    EXPECT_EQ(na.shift, 0);
    EXPECT_EQ(na.num_denominator_words, 3);
    EXPECT_EQ(na.num_numerator_words, 8);
    EXPECT_EQ(na.numerator, u);
    EXPECT_EQ(na.numerator_ex, 0);
    EXPECT_EQ(na.denominator, v);

    u = uint512{7} << 509;
    v = uint256{0x3fffffffffffffff, 1};
    na = normalize(u, v);
    EXPECT_EQ(na.shift, 2);
    EXPECT_EQ(na.num_denominator_words, 3);
    EXPECT_EQ(na.num_numerator_words, 8);
    EXPECT_EQ(na.numerator, u << 2);
    EXPECT_EQ(na.numerator_ex, 3);
    EXPECT_EQ(na.denominator, v << 2);
}

template <typename Int>
struct div_test_case
{
    Int numerator;
    Int denominator;
    Int quotient;
    Int reminder;
};

static div_test_case<uint512> div_test_cases[] = {
    {2, 1, 2, 0},
    {
        0x10000000000000000_u512,
        2,
        0x8000000000000000_u512,
        0,
    },
    {
        0x478392145435897052_u512,
        0x111,
        0x430f89ebadad0baa,
        8,
    },
    {
        0x1e00000000000000000000090000000000000000000000000000000000000000000000000000000000000000000000000000000009000000000000000000_u512,
        0xa,
        0x30000000000000000000000e6666666666666666666666666666666666666666666666666666666666666666666666666666666674ccccccccccccccccc_u512,
        8,
    },
    {
        0x767676767676767676000000767676767676_u512,
        0x2900760076761e00020076760000000076767676000000_u512,
        0,
        0x767676767676767676000000767676767676_u512,
    },
    {
        0x12121212121212121212121212121212_u512,
        0x232323232323232323_u512,
        0x83a83a83a83a83,
        0x171729292929292929_u512,
    },
    {
        0xabc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0abc0_u512,
        0x1c01c01c01c01c01c01c01c01c_u512,
        0x621ed21ed21ed21ed21ed21ed224f40bf40bf40bf40bf40bf40bf46e12de12de12de12de12de12de1900000000000000000_u512,
        0xabc0abc0abc0abc0,
    },
    {
        0xfffff716b61616160b0b0b2b0b0b0becf4bef50a0df4f48b090b2b0bc60a0a00_u512,
        0xfffff716b61616160b0b0b2b0b230b000008010d0a2b00_u512,
        0xffffffffffffffffff_u512,
        0xfffff7169e17030ac1ff082ed51796090b330cd3143500_u512,
    },
    {
        0x50beb1c60141a0000dc2b0b0b0b0b0b410a0a0df4f40b090b2b0bc60a0a00_u512,
        0x2000110000000d0a300e750a000000090a0a_u512,
        0x285f437064cd09ff8bc5b7857d_u512,
        0x1fda1c384d86199e14bb4edfc6693042f11e_u512,
    },
    {
        0x4b00000b41000b0b0b2b0b0b0b0b0b410a0aeff4f40b090b2b0bc60a0a1000_u512,
        0x4b00000b41000b0b0b2b0b0b0b0b0b410a0aeff4f40b0a0a_u512,
        0xffffffffffffff_u512,
        0x4b00000b41000b0b0b2b0b0b0b0b0b400b35fbbafe151a0a_u512,
    },
    {
        0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee_u512,
        7,
        0x22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222_u512,
        0,
    },
    {
        0xf6376770abd3a36b20394c5664afef1194c801c3f05e42566f085ed24d002bb0_u512,
        0xb368d219438b7f3f,
        0x15f53bce87e9fb63c7c3ab03f6c0ba30d3ecf982fa97cdf0a_u512,
        0x4bfd94dbec31523a,
    },
    {
        0x0_u512,
        0x10900000000000000000000000000000000000000000000000000_u512,
        0x0_u512,
        0x0_u512,
    },
    {
        0x77676767676760000000000000001002e000000000000040000000e000000000000007f0000000000000000000000000000000000000000f7000000000000_u512,
        0xfffc000000000000767676240000000000002b05760476000000000000000000767676767600000000000000000000000000000000_u512,
        0x7769450c7b994e65025_u512,
        0x241cb1aa4f67c22ae65c9920bf3bb7ad8280311a887aee8be4054a3e242a5ea9ab35d800f2000000000000000000f7000000000000_u512,
    },
    {
        0xdffffffffffffffffffffffffffffffffff00000000000000000000000000000000000000000001000000000000000000000000008100000000001001_u512,
        0xdffffffffffffffffffffffffffffffffffffffffffff3fffffffffffffffffffffffffffff_u512,
        0xffffffffffffffffffffffffffffffffffedb6db6db6e9_u512,
        0x200000000000000000000000000010000f2492492492ec000000000000080ffedb6db6dc6ea_u512,
    },
    {
        0xff000000000000000000000000000000000000000400000092767600000000000000000000000081000000000000000000000001020000000000eeffffffffff_u512,
        0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffff000000000000000000000005000000000000000000ffffffffff100000000000000000_u512,
        0x0_u512,
        0xff000000000000000000000000000000000000000400000092767600000000000000000000000081000000000000000000000001020000000000eeffffffffff_u512,
    },
    {
        0xfffffffffffffffffffffffffffffffffffffffbffffff6d8989ffffffffffffffffffffffff7efffffffffffffffffffffffefdffffffffff110000000001_u512,
        0xfffffffffffffffffffffffaffffffffffffffffff0000000000f00000000000000000_u512,
        0x1000000000000000000000004fffffffffffffffc00ffff8689890fff_u512,
        0xffffffec09fffda0afa81efafc00ffff868d481fff71de0d8100efffff110000000001_u512,
    },
    {
        0x767676767676000000000076000000000000005600000000000000000000_u512,
        0x767676767676000000000076000000760000_u512,
        0xffffffffffffffffffffffff_u512,
        0x767676007676005600000076000000760000_u512,
    },
    {
        0x8200000000000000000000000000000000000000000000000000000000000000_u512,
        0x8200000000000000fe000004000000ffff000000fffff700_u512,
        0xfffffffffffffffe_u512,
        0x5fffffbffffff01fd00000700000afffe000001ffffee00_u512,
    },
    {
        0xdac7fff9ffd9e1322626262626262600_u512,
        0xd021262626262626_u512,
        0x10d1a094108c5da55_u512,
        0x6f386ccc73c11f62_u512,
    },
};

TEST(div, udivrem_512)
{
    for (auto& t : div_test_cases)
    {
        auto res = udivrem(t.numerator, t.denominator);
        EXPECT_EQ(res.quot, t.quotient);
        EXPECT_EQ(res.rem, t.reminder);
    }
}


static div_test_case<uint256> sdivrem_test_cases[] = {
    {13_u256, 3_u256, 4_u256, 1_u256},
    {-13_u256, 3_u256, -4_u256, -1_u256},
    {13_u256, -3_u256, -4_u256, 1_u256},
    {-13_u256, -3_u256, 4_u256, -1_u256},
    {1_u256 << 255, -1_u256, 1_u256 << 255, 0},
};

TEST(div, sdivrem_256)
{
    for (auto& t : sdivrem_test_cases)
    {
        EXPECT_EQ(t.denominator * t.quotient + t.reminder, t.numerator);

        auto res = sdivrem(t.numerator, t.denominator);
        EXPECT_EQ(res.quot, t.quotient);
        EXPECT_EQ(res.rem, t.reminder);
    }
}

TEST(div, sdivrem_512)
{
    auto n = -13_u512;
    auto d = -3_u512;

    auto res1 = sdivrem(n, d);
    EXPECT_EQ(res1.quot, 4_u512);
    EXPECT_EQ(res1.rem, -1_u512);
}

inline uint64_t reciprocal_naive(uint64_t d) noexcept
{
    const auto u = uint128{~d, ~uint64_t{0}};
    uint64_t v;

#if __x86_64__
    uint64_t _;
    asm("divq %4" : "=d"(_), "=a"(v) : "d"(u.hi), "a"(u.lo), "g"(d));
#else
    v = (u / d).lo;
#endif

    return v;
}

TEST(div, reciprocal)
{
    constexpr auto n = 1000000;

    constexpr auto d_start = uint64_t{1} << 63;
    for (uint64_t d = d_start; d < d_start + n; ++d)
    {
        auto v = reciprocal_2by1(d);
        EXPECT_EQ(v, reciprocal_naive(d)) << d;
    }

    constexpr auto d_end = ~uint64_t{0};
    for (uint64_t d = d_end; d > d_end - n; --d)
    {
        auto v = reciprocal_2by1(d);
        EXPECT_EQ(v, reciprocal_naive(d)) << d;
    }
}
