// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <experimental/div.hpp>
#include <intx/intx.hpp>

#include <cassert>
#include <cstring>
#include <tuple>

#pragma GCC diagnostic ignored "-Wsign-conversion"


#if 0
#include <iostream>
#define DEBUG(X) X
inline std::ostream& dbgs() { return std::cerr; }
#else
#define DEBUG(X) \
    do           \
    {            \
    } while (false)
#endif

namespace intx
{
inline div_result<uint64_t> udivrem_long(uint128 x, uint64_t y) noexcept
{
    auto shift = clz(y);
    auto yn = y << shift;
    auto xn = x << shift;
    auto v = reciprocal_2by1(yn);
    auto res = udivrem_2by1(xn, yn, v);
    return {res.quot, res.rem >> shift};
}

inline std::tuple<uint32_t, uint32_t> udivrem_long_asm(uint64_t u, uint32_t v)
{
#if (__GNUC__ || __clang__) && __x86_64__
    // RDX:RAX by r/m64 : RAX <- Quotient, RDX <- Remainder.
    uint32_t q, r;
    const auto uh = static_cast<uint32_t>(u >> 32);
    const auto ul = static_cast<uint32_t>(u);
    asm("divl %4" : "=d"(r), "=a"(q) : "d"(uh), "a"(ul), "g"(v));
#else
    const auto q = uint32_t(u / v);
    const auto r = uint32_t(u % v);
#endif
    return {q, r};
}

// The fastest for 256 / 32.
static void udivrem_1_stable(uint32_t* q, uint32_t* r, const uint32_t* u, uint32_t v, int m)
{
    // Load the divisor once. The enabled more optimization because compiler
    // knows that divisor remains unchanged when storing to q[j].
    uint32_t remainder = 0;

    // TODO: Use fixed m, i.e. m = 8 for uint256.
    for (int j = m - 1; j >= 0; --j)
    {
        uint64_t dividend = join(remainder, u[j]);
        // This cannot overflow because the high part of the devidend is the
        // remainder of the previous division so smaller than v.
        std::tie(q[j], remainder) = udivrem_long_asm(dividend, v);
    }
    *r = remainder;
}

div_result<uint256> udivrem_1(uint256 x, uint64_t y)
{
    uint64_t r = 0;
    uint256 q = 0;

    uint64_t u[4];
    std::memcpy(u, &x, sizeof(x));

    auto qt = (uint64_t*)&q;

    for (int j = 4 - 1; j >= 0; --j)
    {
        auto res = udivrem_long({r, u[j]}, y);
        qt[j] = res.quot;
        r = res.rem;
    }
    return {q, r};
}

div_result<uint512> udivrem_1(uint512 x, uint64_t y)
{
    uint64_t r = 0;
    uint512 q = 0;

    uint64_t u[8];
    std::memcpy(u, &x, sizeof(x));

    auto qt = (uint64_t*)&q;

    for (int j = 8 - 1; j >= 0; --j)
    {
        auto res = udivrem_long({r, u[j]}, y);
        qt[j] = res.quot;
        r = res.rem;
    }
    return {q, r};
}


/// Implementation of Knuth's Algorithm D (Division of nonnegative integers)
/// from "Art of Computer Programming, Volume 2", section 4.3.1, p. 272. The
/// variables here have the same names as in the algorithm. Comments explain
/// the algorithm and any deviation from it.
static void KnuthDiv(uint32_t* u, uint32_t* v, uint32_t* q, uint32_t* r, unsigned m, unsigned n)
{
    assert(u && "Must provide dividend");
    assert(v && "Must provide divisor");
    assert(q && "Must provide quotient");
    assert(u != v && u != q && v != q && "Must use different memory");
    assert(n > 1 && "n must be > 1");

    // b denotes the base of the number system. In our case b is 2^32.
    const uint64_t b = uint64_t(1) << 32;

    // The DEBUG macros here tend to be spam in the debug output if you're not
    // debugging this code. Disable them unless KNUTH_DEBUG is defined.

    DEBUG(dbgs() << "KnuthLL: m=" << m << " n=" << n << '\n');
    DEBUG(dbgs() << "KnuthLL: original:");
    DEBUG(for (int i = m + n; i >= 0; i--) dbgs() << " " << u[i]);
    DEBUG(dbgs() << " by");
    DEBUG(for (int i = n; i > 0; i--) dbgs() << " " << v[i - 1]);
    DEBUG(dbgs() << '\n');
    // D1. [Normalize.] Set d = b / (v[n-1] + 1) and multiply all the digits of
    // u and v by d. Note that we have taken Knuth's advice here to use a power
    // of 2 value for d such that d * v[n-1] >= b/2 (b is the base). A power of
    // 2 allows us to shift instead of multiply and it is easy to determine the
    // shift amount from the leading zeros.  We are basically normalizing the u
    // and v so that its high bits are shifted to the top of v's range without
    // overflow. Note that this can require an extra word in u so that u must
    // be of length m+n+1.
    unsigned shift = clz(v[n - 1]);
    uint32_t v_carry = 0;
    uint32_t u_carry = 0;
    if (shift)
    {
        for (unsigned i = 0; i < m + n; ++i)
        {
            uint32_t u_tmp = u[i] >> (32 - shift);
            u[i] = (u[i] << shift) | u_carry;
            u_carry = u_tmp;
        }
        for (unsigned i = 0; i < n; ++i)
        {
            uint32_t v_tmp = v[i] >> (32 - shift);
            v[i] = (v[i] << shift) | v_carry;
            v_carry = v_tmp;
        }
    }
    u[m + n] = u_carry;

    DEBUG(dbgs() << "KnuthLL:   normal:");
    DEBUG(for (int i = m + n; i >= 0; i--) dbgs() << " " << u[i]);
    DEBUG(dbgs() << " by");
    DEBUG(for (int i = n; i > 0; i--) dbgs() << " " << v[i - 1]);
    DEBUG(dbgs() << '\n');

    // D2. [Initialize j.]  Set j to m. This is the loop counter over the places.
    int j = m;
    do
    {
        DEBUG(dbgs() << "KnuthLL: quotient digit #" << j << '\n');
        // D3. [Calculate q'.].
        //     Set qp = (u[j+n]*b + u[j+n-1]) / v[n-1]. (qp=qprime=q')
        //     Set rp = (u[j+n]*b + u[j+n-1]) % v[n-1]. (rp=rprime=r')
        // Now test if qp == b or qp*v[n-2] > b*rp + u[j+n-2]; if so, decrease
        // qp by 1, increase rp by v[n-1], and repeat this test if rp < b. The test
        // on v[n-2] determines at high speed most of the cases in which the trial
        // value qp is one too large, and it eliminates all cases where qp is two
        // too large.
        uint64_t dividend = intx::join(u[j + n], u[j + n - 1]);
        DEBUG(dbgs() << "KnuthLL: dividend == " << dividend << '\n');

        uint64_t qp, rp;
        if (u[j + n] >= v[n - 1])
        {
            // Overflow:
            qp = b;
            rp = dividend - qp * v[n - 1];
        }
        else
        {
            qp = dividend / v[n - 1];
            rp = dividend % v[n - 1];
        }


        if (qp == b || qp * v[n - 2] > b * rp + u[j + n - 2])
        {
            qp--;
            rp += v[n - 1];
            if (rp < b && (qp == b || qp * v[n - 2] > b * rp + u[j + n - 2]))
            {
                qp--;
            }
        }
        DEBUG(dbgs() << "KnuthLL: qp == " << qp << ", rp == " << rp << '\n');

        // D4. [Multiply and subtract.] Replace (u[j+n]u[j+n-1]...u[j]) with
        // (u[j+n]u[j+n-1]..u[j]) - qp * (v[n-1]...v[1]v[0]). This computation
        // consists of a simple multiplication by a one-place number, combined with
        // a subtraction.
        // The digits (u[j+n]...u[j]) should be kept positive; if the result of
        // this step is actually negative, (u[j+n]...u[j]) should be left as the
        // true value plus b**(n+1), namely as the b's complement of
        // the true value, and a "borrow" to the left should be remembered.
        int64_t borrow = 0;
        for (unsigned i = 0; i < n; ++i)
        {
            uint64_t p = qp * v[i];
            uint64_t subres = uint64_t(u[j + i]) - borrow - intx::lo_half(p);
            u[j + i] = intx::lo_half(subres);
            borrow = intx::hi_half(p) - intx::hi_half(subres);
            DEBUG(dbgs() << "KnuthLL: u[" << (j + i) << "] = " << u[j + i]
                         << ", borrow = " << borrow << '\n');
        }
        bool isNeg = u[j + n] < borrow;
        u[j + n] -= intx::lo_half(static_cast<uint64_t>(borrow));

        DEBUG(dbgs() << "KnuthLL: after subtraction:");
        DEBUG(for (int i = m + n; i >= 0; i--) dbgs() << " " << u[i]);
        DEBUG(dbgs() << '\n');

        // D5. [Test remainder.] Set q[j] = qp. If the result of step D4 was
        // negative, go to step D6; otherwise go on to step D7.
        q[j] = intx::lo_half(qp);
        if (isNeg)
        {
            // D6. [Add back]. The probability that this step is necessary is very
            // small, on the order of only 2/b. Make sure that test data accounts for
            // this possibility. Decrease q[j] by 1
            q[j]--;
            // and add (0v[n-1]...v[1]v[0]) to (u[j+n]u[j+n-1]...u[j+1]u[j]).
            // A carry will occur to the left of u[j+n], and it should be ignored
            // since it cancels with the borrow that occurred in D4.
            bool carry = false;
            for (unsigned i = 0; i < n; i++)
            {
                uint32_t limit = std::min(u[j + i], v[i]);
                DEBUG(dbgs() << "KnuthLL: u[" << (j + i) << "] = " << u[j + i] << " + " << v[i]
                             << " + " << (int)carry << '\n');
                u[j + i] += v[i] + carry;
                carry = u[j + i] < limit || (carry && u[j + i] == limit);
            }
            u[j + n] += carry;
        }
        DEBUG(dbgs() << "KnuthLL: after correction:");
        DEBUG(for (int i = m + n; i >= 0; i--) dbgs() << " " << u[i]);
        DEBUG(dbgs() << "\nKnuthLL: digit result = " << q[j] << '\n');

        // D7. [Loop on j.]  Decrease j by one. Now if j >= 0, go back to D3.
    } while (--j >= 0);

    DEBUG(dbgs() << "KnuthLL: quotient:");
    DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << q[i]);
    DEBUG(dbgs() << '\n');

    // D8. [Unnormalize]. Now q[...] is the desired quotient, and the desired
    // remainder may be obtained by dividing u[...] by d. If r is non-null we
    // compute the remainder (urem uses this).
    if (r)
    {
        // The value d is expressed by the "shift" value above since we avoided
        // multiplication by d by using a shift left. So, all we have to do is
        // shift right here.
        if (shift)
        {
            uint32_t carry = 0;
            DEBUG(dbgs() << "KnuthLL: remainder:");
            for (int i = n - 1; i >= 0; i--)
            {
                r[i] = (u[i] >> shift) | carry;
                carry = u[i] << (32 - shift);
                DEBUG(dbgs() << " " << r[i]);
            }
        }
        else
        {
            for (int i = n - 1; i >= 0; i--)
            {
                r[i] = u[i];
                DEBUG(dbgs() << " " << r[i]);
            }
        }
        DEBUG(dbgs() << '\n');
    }
    DEBUG(dbgs() << '\n');
}


static int divmnu(unsigned q[], unsigned r[], const unsigned u[], const unsigned v[], int m, int n)
{
    const uint64_t b = uint64_t(1) << 32;  // Number base (32 bits).
    unsigned *un, *vn;                     // Normalized form of u, v.
    if (m < n || n <= 0 || v[n - 1] == 0)
        return 1;  // Return if invalid param.

    if (n == 1)
    {
        udivrem_1_stable(q, r, u, v[0], m);
        return 0;
    }

    DEBUG(dbgs() << "KnuthHD: m=" << m << " n=" << n << '\n');
    DEBUG(dbgs() << "KnuthHD: original:");
    DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << u[i]);
    DEBUG(dbgs() << " by");
    DEBUG(for (int i = n; i > 0; i--) dbgs() << " " << v[i - 1]);
    DEBUG(dbgs() << '\n');

    // Normalize by shifting v left just enough so that
    // its high-order bit is on, and shift u left the
    // same amount. We may have to append a high-order
    // digit on the dividend; we do that unconditionally.
    unsigned shift = intx::clz(v[n - 1]);
    vn = static_cast<uint32_t*>(alloca(n * sizeof(uint32_t)));
    // shift == 0, we would get shift by 32 => UB. Consider using uint64.
    for (auto i = n - 1; i > 0; i--)
        vn[i] = shift != 0 ? (v[i] << shift) | (v[i - 1] >> (32 - shift)) : v[i];
    vn[0] = v[0] << shift;

    un = static_cast<uint32_t*>(alloca((m + 1) * sizeof(uint32_t)));
    un[m] = shift != 0 ? u[m - 1] >> (32 - shift) : 0;
    for (auto i = m - 1; i > 0; i--)
        un[i] = shift != 0 ? (u[i] << shift) | (u[i - 1] >> (32 - shift)) : u[i];
    un[0] = u[0] << shift;

    DEBUG(dbgs() << "KnuthHD:   normal:");
    DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << un[i]);
    DEBUG(dbgs() << " by");
    DEBUG(for (int i = n; i > 0; i--) dbgs() << " " << vn[i - 1]);
    DEBUG(dbgs() << '\n');

    for (auto j = m - n; j >= 0; j--)  // Main loop.
    {
        DEBUG(dbgs() << "KnuthHD: quotient digit #" << j << '\n');

        auto dividend = un[j + n] * b + un[j + n - 1];
        DEBUG(dbgs() << "KnuthHD: dividend == " << dividend << '\n');

        // Compute estimate qhat of q[j].
        uint64_t qhat = dividend / vn[n - 1];         // Estimated quotient digit.
        uint64_t rhat = dividend - qhat * vn[n - 1];  // A remainder.
    again:
        if (qhat >= b || qhat * vn[n - 2] > b * rhat + un[j + n - 2])
        {
            qhat--;
            rhat += vn[n - 1];
            if (rhat < b)
                goto again;
        }

        DEBUG(dbgs() << "KnuthHD: qp == " << qhat << ", rp == " << rhat << '\n');

        // Multiply and subtract.
        int64_t borrow = 0;
        for (int i = 0; i < n; i++)
        {
            uint64_t p = qhat * vn[i];
            uint64_t t = uint64_t(un[i + j]) - borrow - static_cast<unsigned>(p);
            un[i + j] = static_cast<unsigned>(t);
            borrow = unsigned(p >> 32) - unsigned(t >> 32);

            DEBUG(dbgs() << "KnuthHD: u[" << (j + i) << "] = " << u[j + i]
                         << ", borrow = " << borrow << '\n');
        }
        int64_t t = un[j + n] - borrow;
        un[j + n] = static_cast<unsigned>(t);

        DEBUG(dbgs() << "KnuthHD: after subtraction:");
        DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << un[i]);
        DEBUG(dbgs() << '\n');

        q[j] = static_cast<unsigned>(qhat);  // Store quotient digit.
        if (t < 0)
        {            // If we subtracted too
            q[j]--;  // much, add back.
            uint64_t carry = 0;
            for (int i = 0; i < n; i++)
            {
                // TODO: Consider using bool carry. See LLVM version.
                uint64_t t1 = uint64_t(un[i + j]) + vn[i] + carry;
                un[i + j] = static_cast<unsigned>(t1);
                carry = t1 >> 32;
            }
            un[j + n] = static_cast<unsigned>(un[j + n] + carry);
        }

        DEBUG(dbgs() << "KnuthHD: after correction:");
        DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << un[i]);
        DEBUG(dbgs() << "\nKnuthHD: digit result = " << q[j] << '\n');
    }  // End j.
       // If the caller wants the remainder, unnormalize
       // it and pass it back.
    if (r)
    {
        for (auto i = 0; i < n; i++)
            r[i] = shift != 0 ? (un[i] >> shift) | (un[i + 1] << (32 - shift)) : un[i];
    }
    return 0;
}

static void udiv_knuth_internal_base(
    unsigned q[], unsigned r[], const unsigned u[], const unsigned v[], int m, int n)
{
    if (n == 1)
        return udivrem_1_stable(q, r, u, v[0], m);

    DEBUG(dbgs() << "KnuthHD: m=" << m << " n=" << n << '\n');
    DEBUG(dbgs() << "KnuthHD: original:");
    DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << u[i]);
    DEBUG(dbgs() << " by");
    DEBUG(for (int i = n; i > 0; i--) dbgs() << " " << v[i - 1]);
    DEBUG(dbgs() << '\n');

    // Normalize by shifting the divisor v left so that its highest bit is on,
    // and shift the dividend u left the same amount.
    auto vn = static_cast<uint32_t*>(alloca(n * sizeof(uint32_t)));
    auto un = static_cast<uint32_t*>(alloca((m + 1) * sizeof(uint32_t)));

    unsigned shift = clz(v[n - 1]);

    for (int i = n - 1; i > 0; i--)
        vn[i] = shift != 0 ? (v[i] << shift) | (v[i - 1] >> (32 - shift)) : v[i];
    vn[0] = v[0] << shift;

    un[m] = shift != 0 ? u[m - 1] >> (32 - shift) : 0;
    for (int i = m - 1; i > 0; i--)
        un[i] = shift != 0 ? (u[i] << shift) | (u[i - 1] >> (32 - shift)) : u[i];
    un[0] = u[0] << shift;


    DEBUG(dbgs() << "KnuthHD:   normal:");
    DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << un[i]);
    DEBUG(dbgs() << " by");
    DEBUG(for (int i = n; i > 0; i--) dbgs() << " " << vn[i - 1]);
    DEBUG(dbgs() << '\n');

    const uint64_t b = uint64_t(1) << 32;  // Number base (32 bits).
    for (int j = m - n; j >= 0; j--)       // Main loop.
    {
        DEBUG(dbgs() << "KnuthHD: quotient digit #" << j << '\n');

        auto dividend = un[j + n] * b + un[j + n - 1];
        DEBUG(dbgs() << "KnuthHD: dividend == " << dividend << '\n');

        // Compute estimate qhat of q[j].
        uint64_t qhat = dividend / vn[n - 1];         // Estimated quotient digit.
        uint64_t rhat = dividend - qhat * vn[n - 1];  // A remainder.
    again:
        if (qhat >= b || qhat * vn[n - 2] > b * rhat + un[j + n - 2])
        {
            qhat--;
            rhat += vn[n - 1];
            if (rhat < b)
                goto again;
        }

        DEBUG(dbgs() << "KnuthHD: qp == " << qhat << ", rp == " << rhat << '\n');

        // Multiply and subtract.
        int64_t borrow = 0;
        for (int i = 0; i < n; i++)
        {
            uint64_t p = qhat * vn[i];
            uint64_t t = uint64_t(un[i + j]) - borrow - static_cast<unsigned>(p);
            un[i + j] = static_cast<unsigned>(t);
            borrow = unsigned(p >> 32) - unsigned(t >> 32);

            DEBUG(dbgs() << "KnuthHD: u[" << (j + i) << "] = " << u[j + i]
                         << ", borrow = " << borrow << '\n');
        }
        int64_t t = un[j + n] - borrow;
        un[j + n] = static_cast<unsigned>(t);

        DEBUG(dbgs() << "KnuthHD: after subtraction:");
        DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << un[i]);
        DEBUG(dbgs() << '\n');

        q[j] = static_cast<unsigned>(qhat);  // Store quotient digit.
        if (t < 0)
        {            // If we subtracted too
            q[j]--;  // much, add back.
            uint64_t carry = 0;
            for (int i = 0; i < n; i++)
            {
                // TODO: Consider using bool carry. See LLVM version.
                uint64_t t1 = uint64_t(un[i + j]) + vn[i] + carry;
                un[i + j] = static_cast<unsigned>(t1);
                carry = t1 >> 32;
            }
            un[j + n] = static_cast<unsigned>(un[j + n] + carry);
        }

        DEBUG(dbgs() << "KnuthHD: after correction:");
        DEBUG(for (int i = m; i >= 0; i--) dbgs() << " " << un[i]);
        DEBUG(dbgs() << "\nKnuthHD: digit result = " << q[j] << '\n');
    }  // End j.
       // If the caller wants the remainder, unnormalize
       // it and pass it back.
    if (r)
    {
        for (int i = 0; i < n; i++)
            r[i] = shift != 0 ? (un[i] >> shift) | (un[i + 1] << (32 - shift)) : un[i];
    }
}

static void udiv_knuth_internal(
    unsigned q[], unsigned r[], const unsigned u[], const unsigned v[], int m, int n)
{
    // Normalize by shifting the divisor v left so that its highest bit is on,
    // and shift the dividend u left the same amount.
    auto vn = static_cast<uint32_t*>(alloca(n * sizeof(uint32_t)));
    auto un = static_cast<uint32_t*>(alloca((m + 1) * sizeof(uint32_t)));

    unsigned shift = clz(v[n - 1]);
    unsigned lshift = num_bits(v[0]) - shift;

    for (int i = n - 1; i > 0; i--)
        vn[i] = shift ? (v[i] << shift) | (v[i - 1] >> lshift) : v[i];
    vn[0] = v[0] << shift;

    un[m] = shift != 0 ? u[m - 1] >> lshift : 0;
    for (int i = m - 1; i > 0; i--)
        un[i] = shift ? (u[i] << shift) | (u[i - 1] >> lshift) : u[i];
    un[0] = u[0] << shift;

    //    DEBUG(dbgs() << std::hex << "un: ");
    //    DEBUG(for (int i = m; i >= 0; i--) dbgs() << un[i]);
    //    DEBUG(dbgs() << "\n");

    DEBUG(dbgs() << std::hex << "vn: ");
    DEBUG(for (int i = n - 1; i >= 0; i--) dbgs() << vn[i] << " ");
    DEBUG(dbgs() << "\n");

    //    uint32_t v_carry = 0;
    //    uint32_t u_carry = 0;
    //    if (shift)
    //    {
    //        for (int i = 0; i < m; ++i)
    //        {
    //            uint32_t u_tmp = u[i] >> lshift;
    //            un[i] = (u[i] << shift) | u_carry;
    //            u_carry = u_tmp;
    //        }
    //        for (int i = 0; i < n; ++i)
    //        {
    //            uint32_t v_tmp = v[i] >> lshift;
    //            vn[i] = (v[i] << shift) | v_carry;
    //            v_carry = v_tmp;
    //        }
    //    }
    //    else
    //    {
    //        std::copy_n(u, m, un);
    //        std::copy_n(v, n, vn);
    //    }
    //    un[m] = u_carry;


    //    std::copy_n(u, m, un);
    //    std::copy_n(v, n, vn);
    //    un[m] = 0;  // The dividend may get additional high-order digit.
    //
    //
    //    if (shift)
    //    {
    //        uint32_t u_carry = 0;
    //        for (int i = 0; i < (m + 1); ++i)
    //        {
    //            uint32_t u_tmp = un[i] >> lshift;
    //            un[i] = (un[i] << shift) | u_carry;
    //            u_carry = u_tmp;
    //        }
    //        uint32_t v_carry = 0;
    //        for (int i = 0; i < n; ++i)
    //        {
    //            uint32_t v_tmp = vn[i] >> lshift;
    //            vn[i] = (vn[i] << shift) | v_carry;
    //            v_carry = v_tmp;
    //        }
    //    }


    const uint64_t base = uint64_t(1) << 32;  // Number base (32 bits).
    for (int j = m - n; j >= 0; j--)          // Main loop.
    {
        uint64_t qhat, rhat;
        uint32_t divisor = vn[n - 1];
        uint64_t dividend = join(un[j + n], un[j + n - 1]);
        if (hi_half(dividend) >= divisor)  // Will overflow:
        {
            qhat = base;
            rhat = dividend - qhat * divisor;
        }
        else
        {
            std::tie(qhat, rhat) = udivrem_long_asm(dividend, divisor);
        }

        uint32_t next_divisor = vn[n - 2];
        uint64_t pd = join(lo_half(rhat), un[j + n - 2]);
        if (qhat == base || qhat * next_divisor > pd)
        {
            qhat--;
            rhat += divisor;
            pd = join(lo_half(rhat), un[j + n - 2]);
            if (rhat < base && (qhat == base || qhat * next_divisor > pd))
                qhat--;
        }

        // Multiply and subtract.
        int64_t borrow = 0;
        for (int i = 0; i < n; i++)
        {
            uint64_t p = qhat * vn[i];
            int64_t t = int64_t(un[i + j]) - borrow - lo_half(p);
            uint64_t s = static_cast<uint64_t>(t);
            un[i + j] = lo_half(s);
            borrow = hi_half(p) - hi_half(s);
        }
        DEBUG(dbgs() << "borrow: " << (int)borrow << "\n");
        int64_t t = un[j + n] - borrow;
        un[j + n] = static_cast<uint32_t>(t);

        q[j] = lo_half(qhat);  // Store quotient digit.
        DEBUG(dbgs() << std::hex << "q[" << j << "]: " << q[j] << "\n");

        if (t < 0)
        {            // If we subtracted too
            --q[j];  // much, add back.
            uint64_t carry = 0;
            for (int i = 0; i < n; ++i)
            {
                // TODO: Consider using bool carry. See LLVM version.
                uint64_t u_tmp = uint64_t(un[i + j]) + uint64_t(vn[i]) + carry;
                un[i + j] = lo_half(u_tmp);
                carry = hi_half(u_tmp);
            }
            un[j + n] = lo_half(uint64_t(un[j + n]) + carry);
        }
    }

    for (int i = 0; i < n; ++i)
        r[i] = shift ? (un[i] >> shift) | (un[i + 1] << lshift) : un[i];
}


static void udiv_knuth_internal_64(
    uint64_t q[], uint64_t r[], const uint64_t u[], const uint64_t v[], int m, int n)
{
    // Normalize by shifting the divisor v left so that its highest bit is on,
    // and shift the dividend u left the same amount.
    auto vn = static_cast<uint64_t*>(alloca(n * sizeof(uint64_t)));
    auto un = static_cast<uint64_t*>(alloca((m + 1) * sizeof(uint64_t)));

    unsigned shift = clz(v[n - 1]);
    unsigned lshift = num_bits(v[0]) - shift;

    for (int i = n - 1; i > 0; i--)
        vn[i] = shift ? (v[i] << shift) | (v[i - 1] >> lshift) : v[i];
    vn[0] = v[0] << shift;

    un[m] = shift != 0 ? u[m - 1] >> lshift : 0;
    for (int i = m - 1; i > 0; i--)
        un[i] = shift ? (u[i] << shift) | (u[i - 1] >> lshift) : u[i];
    un[0] = u[0] << shift;

    //    DEBUG(dbgs() << std::hex << "un: ");
    //    DEBUG(for (int i = m; i >= 0; i--) dbgs() << un[i]);
    //    DEBUG(dbgs() << "\n");

    DEBUG(dbgs() << std::hex << "vn: ");
    DEBUG(for (int i = n - 1; i >= 0; i--) dbgs() << vn[i] << " ");
    DEBUG(dbgs() << "\n");


    constexpr uint128 base = uint128(1) << 64;  // Number base (32 bits).
    for (int j = m - n; j >= 0; j--)            // Main loop.
    {
        uint128 qhat, rhat;
        uint64_t divisor = vn[n - 1];
        uint128 dividend = join(un[j + n], un[j + n - 1]);
        if (hi_half(dividend) >= divisor)  // Will overflow:
        {
            qhat = base;
            rhat = dividend - qhat * divisor;
        }
        else
        {
            auto res = udivrem_long(dividend, divisor);
            qhat = res.quot;
            rhat = res.rem;
        }

        uint64_t next_divisor = vn[n - 2];
        uint128 pd = join(lo_half(rhat), un[j + n - 2]);
        if (qhat == base || qhat * next_divisor > pd)
        {
            --qhat;
            rhat += divisor;
            pd = join(lo_half(rhat), un[j + n - 2]);
            if (rhat < base && (qhat == base || qhat * next_divisor > pd))
                --qhat;
        }

        // Multiply and subtract.
        uint128 borrow = 0;
        for (int i = 0; i < n; i++)
        {
            uint128 p = qhat * vn[i];
            uint128 t = uint128{un[i + j]} - borrow - lo_half(p);
            un[i + j] = lo_half(t);
            borrow = hi_half(p) - hi_half(t);
        }
        DEBUG(dbgs() << "borrow: " << (int)borrow << "\n");
        auto t = un[j + n] - borrow;
        un[j + n] = static_cast<uint64_t>(t);

        q[j] = lo_half(qhat);  // Store quotient digit.
        DEBUG(dbgs() << std::hex << "q[" << j << "]: " << q[j] << "\n");

        if (t < 0)
        {            // If we subtracted too
            --q[j];  // much, add back.
            uint128 carry = 0;
            for (int i = 0; i < n; ++i)
            {
                // TODO: Consider using bool carry. See LLVM version.
                uint128 u_tmp = uint128(un[i + j]) + uint128(vn[i]) + carry;
                un[i + j] = lo_half(u_tmp);
                carry = hi_half(u_tmp);
            }
            un[j + n] = lo_half(uint128(un[j + n]) + carry);
        }
    }

    for (int i = 0; i < n; ++i)
        r[i] = shift ? (un[i] >> shift) | (un[i + 1] << lshift) : un[i];
}

div_result<uint256> udiv_qr_knuth_64(const uint256& x, const uint256& y)
{
    if (x < y)
        return {0, x};

    const unsigned n = count_significant_words<uint64_t>(y);

    if (n == 1)
        return udivrem_1(x, static_cast<uint64_t>(y));

    // Skip dividend's leading zero limbs.
    const unsigned m = count_significant_words<uint64_t>(x);

    if (n > m)
        return {0, x};

    uint256 q, r;
    auto p_x = (uint64_t*)&x;
    auto p_y = (uint64_t*)&y;
    auto p_q = (uint64_t*)&q;
    auto p_r = (uint64_t*)&r;
    udiv_knuth_internal_64(p_q, p_r, p_x, p_y, m, n);

    return {q, r};
}

div_result<uint256> udiv_qr_knuth_opt(const uint256& x, const uint256& y)
{
    if (x < y)
        return {0, x};

    const unsigned n = count_significant_words<uint32_t>(y);

    if (n <= 2)
        return udivrem_1(x, static_cast<uint64_t>(y));

    // Skip dividend's leading zero limbs.
    const unsigned m = 8 - (clz(x) / (4 * 8));

    if (n > m)
        return {0, x};

    uint256 q, r;
    auto p_x = (uint32_t*)&x;
    auto p_y = (uint32_t*)&y;
    auto p_q = (uint32_t*)&q;
    auto p_r = (uint32_t*)&r;
    udiv_knuth_internal(p_q, p_r, p_x, p_y, m, n);

    return {q, r};
}

div_result<uint512> udiv_qr_knuth_512_64(const uint512& x, const uint512& y)
{
    if (x < y)
        return {0, x};

    const unsigned n = count_significant_words<uint64_t>(y);

    if (n == 1)
        return udivrem_1(x, static_cast<uint64_t>(y.lo.lo.lo));

    // Skip dividend's leading zero limbs.
    const unsigned m = count_significant_words<uint64_t>(x);

    if (n > m)
        return {0, x};

    uint512 q;
    uint256 r;
    auto p_x = (uint64_t*)&x;
    auto p_y = (uint64_t*)&y;
    auto p_q = (uint64_t*)&q;
    auto p_r = (uint64_t*)&r;
    udiv_knuth_internal_64(p_q, p_r, p_x, p_y, m, n);

    return {q, r};
}

div_result<uint512> udiv_qr_knuth_512(const uint512& x, const uint512& y)
{
    if (x < uint512(y))
        return {0, x};

    const unsigned n = count_significant_words<uint32_t>(y);

    if (n <= 2)
        return udivrem_1(x, static_cast<uint64_t>(y.lo.lo.lo));

    // Skip dividend's leading zero limbs.
    const unsigned m = count_significant_words<uint32_t>(x);

    uint512 q;
    uint256 r;
    auto p_x = (uint32_t*)&x;
    auto p_y = (uint32_t*)&y;
    auto p_q = (uint32_t*)&q;
    auto p_r = (uint32_t*)&r;
    udiv_knuth_internal(p_q, p_r, p_x, p_y, m, n);

    return {q, r};
}

div_result<uint256> udiv_qr_knuth_opt_base(const uint256& x, const uint256& y)
{
    // Skip dividend's leading zero limbs.
    const unsigned m = 8 - (clz(x) / (4 * 8));
    const unsigned n = 8 - (clz(y) / (4 * 8));

    if (n > m)
        return {0, x};

    uint256 q, r;
    auto p_x = (uint32_t*)&x;
    auto p_y = (uint32_t*)&y;
    auto p_q = (uint32_t*)&q;
    auto p_r = (uint32_t*)&r;
    udiv_knuth_internal_base(p_q, p_r, p_x, p_y, m, n);

    return {q, r};
}

div_result<uint256> udiv_qr_knuth_hd_base(const uint256& x, const uint256& y)
{
    // Skip dividend's leading zero limbs.
    const unsigned m = 8 - (clz(x) / (4 * 8));
    const unsigned n = 8 - (clz(y) / (4 * 8));

    if (n > m)
        return {0, x};

    uint256 q, r;
    auto p_x = (uint32_t*)&x;
    auto p_y = (uint32_t*)&y;
    auto p_q = (uint32_t*)&q;
    auto p_r = (uint32_t*)&r;
    divmnu(p_q, p_r, p_x, p_y, m, n);

    return {q, r};
}

div_result<uint256> udiv_qr_knuth_llvm_base(const uint256& u, const uint256& v)
{
    // Skip dividend's leading zero limbs.
    const unsigned u_limbs = 8 - (clz(u) / (4 * 8));
    const unsigned n = 8 - (clz(v) / (4 * 8));

    if (n > u_limbs)
        return {0, u};

    unsigned m = u_limbs - n;

    uint32_t u_data[9];  // u needs one limb more.
    std::memcpy(u_data, &u, sizeof(u));
    uint32_t v_data[8];
    std::memcpy(v_data, &v, sizeof(v));
    uint32_t q[8];
    uint32_t r[8];

    // If we're left with only a single word for the divisor, Knuth doesn't work
    // so we implement the short division algorithm here. This is much simpler
    // and faster because we are certain that we can divide a 64-bit quantity
    // by a 32-bit quantity at hardware speed and short division is simply a
    // series of such operations. This is just like doing short division but we
    // are using base 2^32 instead of base 10.
    if (n == 1)
    {
        udivrem_1_stable(q, r, u_data, v_data[0], u_limbs);
    }
    else
    {
        // Now we're ready to invoke the Knuth classical divide algorithm. In this
        // case n > 1.
        KnuthDiv(u_data, v_data, q, r, m, n);
    }

    uint256 qq, rr;
    std::memcpy(&qq, q, (m + 1) * sizeof(uint32_t));
    std::memcpy(&rr, r, n * sizeof(uint32_t));
    return {qq, rr};
}

}  // namespace intx
