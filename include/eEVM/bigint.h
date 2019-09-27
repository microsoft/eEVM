// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <fmt/format_header_only.h>
#include <intx/intx.hpp>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using uint256_t = intx::uint256;
using uint512_t = intx::uint512;

inline uint256_t from_hex_str(const std::string& s)
{
  return intx::from_string<uint256_t>(s);
}

inline auto to_hex_str(const uint256_t& v)
{
  return intx::hex(v);
}

inline auto to_lower_hex_str(const uint256_t& v)
{
  auto s = to_hex_str(v);
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

inline auto from_big_endian(const uint8_t* begin, size_t size = 32u)
{
  if (size == 32)
  {
    return intx::be::unsafe::load<uint256_t>(begin);
  }
  else
  {
    // TODO: Find out how common this path is, make it the caller's
    // responsibility
    uint8_t tmp[32] = {};
    const auto offset = 32 - size;
    memcpy(tmp + offset, begin, size);

    return intx::be::load<uint256_t>(tmp);
  }
}

inline void to_big_endian(const uint256_t& v, uint8_t* out)
{
  // TODO: Is this cast safe?
  // uint8_t(&arr)[32] = *static_cast<uint8_t(*)[32]>(static_cast<void*>(out));
  intx::be::unsafe::store(out, v);
}

inline int get_sign(uint256_t v)
{
  return (v >> 255) ? -1 : 1;
}

inline auto power(uint256_t b, uint64_t e)
{
  return intx::exp(b, uint256_t(e));
}

namespace intx
{
  inline void to_json(nlohmann::json& j, const uint256& v)
  {
    j = to_lower_hex_str(v);
  }

  inline void from_json(const nlohmann::json& j, uint256& v)
  {
    v = from_hex_str(j);
  }
} // namespace intx
