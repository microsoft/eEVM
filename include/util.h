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

  inline void Keccak_256(
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

  std::string strip(std::string s);
  std::vector<uint8_t> to_bytes(std::string s);
  std::string to_hex_string(std::vector<uint8_t> bytes);

  Address generate_address(const Address& sender, uint64_t nonce);

  uint64_t to_uint64(const nlohmann::json& j);
} // namespace evm
