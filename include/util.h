// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

extern "C"
{
#include "../3rdparty/keccak/KeccakHash.h"
}
#include "address.h"

#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace evm
{
  /* Workaround for different json assignment issues, e.g.,
  Boost cpp_int or vector in VS 2017:
  https://github.com/nlohmann/json/issues/220 */
  template <typename T>
  void assign_j(T& o, const nlohmann::json& j)
  {
    o = j.get<T>();
  }

  template <typename T, typename U>
  void assign_const(const T& x, U&& y)
  {
    *const_cast<T*>(&x) = y;
  }

  template <typename T>
  void assign_j_const(const T& x, const nlohmann::json& j)
  {
    T t = j;
    assign_const(x, std::move(t));
  }

  inline void keccak_256(
    const unsigned char* input,
    unsigned int inputByteLen,
    unsigned char* output)
  {
    // Ethereum started using Keccak and called it SHA3 before it was finalised.
    // Standard SHA3-256 (the FIPS accepted version) uses padding 0x06, but
    // Ethereum's "Keccak-256" uses padding 0x01.
    // All other constants are copied from Keccak_HashInitialize_SHA3_256 in
    // KeccakHash.h.
    Keccak_HashInstance hi;
    Keccak_HashInitialize(&hi, 1088, 512, 256, 0x01);
    Keccak_HashUpdate(
      &hi, input, inputByteLen * std::numeric_limits<unsigned char>::digits);
    Keccak_HashFinal(&hi, output);
  }

  inline std::array<uint8_t, 32u> keccak_256(
    const unsigned char* begin, size_t byte_len)
  {
    std::array<uint8_t, 32u> h;
    keccak_256(begin, byte_len, h.data());
    return h;
  }

  inline std::array<uint8_t, 32u> keccak_256(
    const std::string& s, size_t skip = 0)
  {
    skip = std::min(skip, s.size());
    return keccak_256((const unsigned char*)s.data() + skip, s.size() - skip);
  }

  std::string strip(const std::string& s);
  std::vector<uint8_t> to_bytes(const std::string& s);

  template <typename Iterator>
  std::string to_hex_string(Iterator begin, Iterator end)
  {
    std::stringstream ss;

    ss << "0x" << std::hex;
    while (begin != end)
    {
      ss << std::setfill('0') << std::setw(2) << (int)*begin;
      begin++;
    }

    return ss.str();
  }

  template <typename T>
  std::string to_hex_string(const T& bytes)
  {
    return to_hex_string(bytes.begin(), bytes.end());
  }

  inline std::string to_checksum_address(const Address& a)
  {
    auto s = to_lower_hex_str(a);

    // Start at index 2 to skip the "0x" prefix
    const auto h = keccak_256(s, 2);

    for (size_t i = 0; i < s.size() - 2; ++i)
    {
      auto& c = s[i + 2];
      if (c >= 'a' && c <= 'f')
      {
        if (h[i / 2] & (i % 2 == 0 ? 0x80 : 0x08))
        {
          c = std::toupper(c);
        }
      }
    }

    return s;
  }

  inline bool is_checksum_address(const std::string& s)
  {
    const auto cs = to_checksum_address(from_hex_str(s));
    return cs == s;
  }

  Address generate_address(const Address& sender, uint64_t nonce);

  uint64_t to_uint64(const nlohmann::json& j);
} // namespace evm
