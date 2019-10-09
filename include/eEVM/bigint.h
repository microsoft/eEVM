// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <intx/intx.hpp>

using uint256_t = intx::uint256;
using uint512_t = intx::uint512;

namespace intx
{
  template <unsigned N>
  std::ostream& operator<<(std::ostream& o, const uint<N>& n)
  {
    const auto fmt_flags = o.flags();
    const auto basefield = fmt_flags & std::ostream::basefield;
    const auto showbase = fmt_flags & std::ostream::showbase;

    switch (basefield)
    {
      case (std::ostream::hex):
      {
        if (showbase)
        {
          o << "0x";
        }
        o << to_string(n, 16);
        break;
      }

      case (std::ostream::oct):
      {
        if (showbase)
        {
          o << "0";
        }
        o << to_string(n, 8);
        break;
      }

      default:
      {
        o << to_string(n, 10);
        break;
      }
    }
    return o;
  }
}
