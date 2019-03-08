// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../include/util.h"

#include "../include/rlp.h"

#include <iomanip>

using namespace std;

namespace evm
{
  string strip(const string& s)
  {
    return (s.size() >= 2 && s[1] == 'x') ? s.substr(2) : s;
  }

  uint64_t to_uint64(const nlohmann::json& j)
  {
    string s = j;
    return strtoull(&s[0], nullptr, 16);
  }

  vector<uint8_t> to_bytes(const string& _s)
  {
    auto s = evm::strip(_s);

    const size_t byte_len = (s.size() + 1) / 2; // round up
    vector<uint8_t> v(byte_len);

    // Handle odd-length strings
    size_t n = 0;
    if (s.size() % 2 != 0)
    {
      v[0] = static_cast<uint8_t>(strtoul(s.substr(0, 1).c_str(), nullptr, 16));
      ++n;
    }

    auto x = n;
    for (auto i = n; i < byte_len; ++i, x += 2)
    {
      v[i] = static_cast<uint8_t>(strtoul(s.substr(x, 2).c_str(), nullptr, 16));
    }
    return v;
  }

  string to_hex_string(const vector<uint8_t>& bytes)
  {
    stringstream ss;

    ss << "0x" << hex;
    for (int b : bytes)
    {
      ss << setfill('0') << setw(2) << b;
    }
    return ss.str();
  }

  Address generate_address(const Address& sender, uint64_t nonce)
  {
    const auto rlp_encoding = rlp::encode(sender, nonce);

    uint8_t buffer[32u];
    Keccak_256(
      rlp_encoding.data(),
      static_cast<unsigned int>(rlp_encoding.size()),
      buffer);

    return from_big_endian(buffer + 12u, buffer + 32u);
  }
} // namespace evm
