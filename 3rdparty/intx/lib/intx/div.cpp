// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include "div.hpp"

namespace intx
{
namespace
{
/// Divides arbitrary long unsigned integer by 64-bit unsigned integer (1 word).
/// @param u  The array of a normalized numerator words. It will contain the quotient after
///           execution.
/// @param m  The number of numerator words BEFORE normalization.
/// @param d  The normalized denominator.
/// @return   The remainder.
inline uint64_t udivrem_by1(uint64_t u[], int m, uint64_t d) noexcept
{
    const auto v = reciprocal_2by1(d);

    auto r = u[m];  // Set the top word as remainder.
    u[m] = 0;       // Reset the word being a part of the result quotient.

    for (int j = m - 1; j >= 0; --j)
    {
        const auto x = udivrem_2by1({r, u[j]}, d, v);
        u[j] = x.quot;
        r = x.rem;
    }

    return r;
}

/// Divides arbitrary long unsigned integer by 128-bit unsigned integer (2 words).
/// @param u  The array of a normalized numerator words. It will contain the quotient after
///           execution.
/// @param m  The number of numerator words BEFORE normalization.
/// @param d  The normalized denominator.
/// @return   The remainder.
inline uint128 udivrem_by2(uint64_t u[], int m, uint128 d) noexcept
{
    const auto v = reciprocal_3by2(d);

    auto r = uint128{u[m], u[m - 1]};  // Set the 2 top words as remainder.
    u[m] = u[m - 1] = 0;               // Reset these words being a part of the result quotient.

    for (int j = m - 2; j >= 0; --j)
    {
        const auto x = udivrem_3by2(r.hi, r.lo, u[j], d, v);
        u[j] = x.quot.lo;
        r = x.rem;
    }

    return r;
}

void udivrem_knuth(uint64_t q[], uint64_t un[], int m, const uint64_t vn[], int n) noexcept
{
    const auto divisor = uint128{vn[n - 1], vn[n - 2]};
    const auto reciprocal = reciprocal_2by1(divisor.hi);
    for (int j = m - n; j >= 0; --j)
    {
        const auto u2 = un[j + n];
        const auto u1 = un[j + n - 1];
        const auto u0 = un[j + n - 2];

        uint64_t qhat;
        uint128 rhat;
        const auto dividend = uint128{u2, u1};
        if (dividend.hi >= divisor.hi)  // Will overflow:
        {
            qhat = ~uint64_t{0};
            rhat = dividend - uint128{divisor.hi, 0};
            rhat += divisor.hi;

            // Adjustment.
            // OPT: This is not needed but helps avoiding negative case.
            if (rhat.hi == 0 && umul(qhat, divisor.lo) > uint128{rhat.lo, u0})
                --qhat;
        }
        else
        {
            auto res = udivrem_2by1(dividend, divisor.hi, reciprocal);
            qhat = res.quot;
            rhat = res.rem;

            if (umul(qhat, divisor.lo) > uint128{rhat.lo, u0})
            {
                --qhat;
                rhat += divisor.hi;

                // Adjustment.
                // OPT: This is not needed but helps avoiding negative case.
                if (rhat.hi == 0 && umul(qhat, divisor.lo) > uint128{rhat.lo, u0})
                    --qhat;
            }
        }

        // Multiply and subtract.
        uint64_t borrow = 0;
        for (int i = 0; i < n; ++i)
        {
            const auto p = umul(qhat, vn[i]);
            const auto s = uint128{un[i + j]} - borrow - p.lo;
            un[i + j] = s.lo;
            borrow = p.hi - s.hi;
        }

        un[j + n] = u2 - borrow;
        if (u2 < borrow)  // Too much subtracted, add back.
        {
            --qhat;

            uint64_t carry = 0;
            for (int i = 0; i < n; ++i)
            {
                auto s = uint128{un[i + j]} + vn[i] + carry;
                un[i + j] = s.lo;
                carry = s.hi;
            }
            un[j + n] += carry;

            // TODO: Consider this alternative implementation:
            // bool k = false;
            // for (int i = 0; i < n; ++i) {
            //     auto limit = std::min(un[j+i],vn[i]);
            //     un[i + j] += vn[i] + k;
            //     k = un[i + j] < limit || (k && un[i + j] == limit);
            // }
            // un[j+n] += k;
        }

        q[j] = qhat;  // Store quotient digit.
    }
}

}  // namespace

template <unsigned N>
div_result<uint<N>> udivrem(const uint<N>& u, const uint<N>& v) noexcept
{
    auto na = normalize(u, v);

    if (na.num_denominator_words > na.num_numerator_words)
        return {0, u};

    if (na.num_denominator_words == 1)
    {
        auto r = udivrem_by1(
            as_words(na.numerator), na.num_numerator_words, as_words(na.denominator)[0]);
        return {na.numerator, r >> na.shift};
    }

    if (na.num_denominator_words == 2)
    {
        auto d = as_words(na.denominator);
        auto r = udivrem_by2(as_words(na.numerator), na.num_numerator_words, {d[1], d[0]});
        return {na.numerator, r >> na.shift};
    }

    const auto n = na.num_denominator_words;
    const auto m = na.num_numerator_words;

    auto un = as_words(na.numerator);  // Will be modified.

    uint<N> q;
    uint<N> r;
    auto rw = as_words(r);

    udivrem_knuth(as_words(q), &un[0], m, as_words(na.denominator), n);

    for (int i = 0; i < n; ++i)
        rw[i] = na.shift ? (un[i] >> na.shift) | (un[i + 1] << (64 - na.shift)) : un[i];

    return {q, r};
}

template div_result<uint256> udivrem(const uint256& u, const uint256& v) noexcept;
template div_result<uint512> udivrem(const uint512& u, const uint512& v) noexcept;

}  // namespace intx
