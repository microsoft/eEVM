// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <intx/intx.hpp>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

using uint256_t = intx::uint256;
using uint512_t = intx::uint512;

namespace intx
{
  // ostream operator allows easy printing. This should be contributed directly
  // to intx
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

  // to/from json converters
  template <unsigned N>
  void to_json(nlohmann::json& j, const uint<N>& n)
  {
    std::stringstream ss;
    ss << "0x" << to_string(n, 16);
    j = ss.str();
  }

  template <unsigned N>
  void from_json(const nlohmann::json& j, uint<N>& n)
  {
    if (!j.is_string())
    {
      throw std::runtime_error(
        "intx numbers can only be parsed from hex-string");
    }

    const auto s = j.get<std::string>();
    n = from_string<uint<N>>(s);
  }
}
