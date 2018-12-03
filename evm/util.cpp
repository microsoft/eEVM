// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../include/util.h"

#include "rlp.h"

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
    // "positive RLP integers must be represented in big endian binary form with
    // no leading zeroes"
    uint8_t buffer[32u];
    to_big_endian(sender, buffer);
    rlp::ByteString s{buffer + 12u, buffer + 32u};

    rlp::ByteString n;
    bool started = false;
    for (size_t byte_index = 0; byte_index < 8; ++byte_index)
    {
      auto offset = (7 - byte_index) * 8;
      static constexpr uint64_t BYTE_MASK = 0xff;
      uint8_t nonce_byte = (nonce & (BYTE_MASK << offset)) >> offset;
      if (started || nonce_byte != 0)
      {
        n.push_back(nonce_byte);
        started = true;
      }
    }

    rlp::Item it = rlp::Item::TList{s, n};

    rlp::ByteString bs = rlp::encode(it);

    Keccak_256(bs.data(), static_cast<unsigned int>(bs.size()), buffer);

    return from_big_endian(buffer + 12u, buffer + 32u);
  }
} // namespace evm
