// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

extern "C"
{
#include "../3rdparty/keccak/KeccakHash.h"
}
#include "address.h"

#include <iomanip>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace eevm
{
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

  using KeccakHash = std::array<uint8_t, 32u>;

  inline KeccakHash keccak_256(const uint8_t* begin, size_t byte_len)
  {
    KeccakHash h;
    keccak_256(begin, byte_len, h.data());
    return h;
  }

  inline KeccakHash keccak_256(const std::string& s)
  {
    return keccak_256((const uint8_t*)s.data(), s.size());
  }

  inline KeccakHash keccak_256(const std::vector<uint8_t>& v)
  {
    return keccak_256(v.data(), v.size());
  }

  template <size_t N>
  inline KeccakHash keccak_256(const std::array<uint8_t, N>& a)
  {
    return keccak_256(a.data(), N);
  }

  template <typename T>
  inline KeccakHash keccak_256_skip(size_t skip, const T& t)
  {
    skip = std::min(skip, t.size());
    return keccak_256((const uint8_t*)t.data() + skip, t.size() - skip);
  }

  std::string strip(const std::string& s);
  std::vector<uint8_t> to_bytes(const std::string& s);

  template <typename Iterator>
  std::string to_hex_string(Iterator begin, Iterator end)
  {
    return fmt::format("0x{:02x}", fmt::join(begin, end, ""));
  }

  template <typename T>
  std::string to_hex_string(const T& bytes)
  {
    return to_hex_string(bytes.begin(), bytes.end());
  }

  inline std::string to_hex_string(uint64_t v)
  {
    return fmt::format("0x{:x}", v);
  }

  inline auto address_to_hex_string(const Address& v)
  {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(40) << std::setfill('0')
       << to_hex_str(v).substr(2);
    auto s = ss.str();
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
  }

  inline std::string to_checksum_address(const Address& a)
  {
    auto s = to_lower_hex_str(a);

    // Start at index 2 to skip the "0x" prefix
    const auto h = keccak_256_skip(2, s);

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
  uint64_t to_uint64(const std::string& s);
} // namespace eevm
