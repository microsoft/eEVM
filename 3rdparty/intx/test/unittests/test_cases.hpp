// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <intx/intx.hpp>

namespace
{
struct arithmetic_test_case
{
    intx::uint256 x;
    intx::uint256 y;
    intx::uint256 sum;
    intx::uint256 product;
};

arithmetic_test_case arithmetic_test_cases[] = {
    {0, 0, 0, 0},
    {127, 1, 128, 127},
};
}  // namespace
