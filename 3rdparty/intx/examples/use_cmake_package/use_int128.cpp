// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <intx/int128.hpp>

int main(int argc, char**)
{
    return static_cast<int>(argc / intx::uint128{1, 0});
}
