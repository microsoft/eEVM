// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "simplestorage.h"

#include "../include/util.h"

#include <ostream>

namespace eevm
{
  SimpleStorage::SimpleStorage(const nlohmann::json& j)
  {
    for (auto it = j.cbegin(); it != j.cend(); it++)
      s.emplace(
        std::piecewise_construct,
        /* key */ std::forward_as_tuple(from_hex_str(it.key())),
        /* value */ std::forward_as_tuple(from_hex_str(it.value())));
  }

  void SimpleStorage::store(const uint256_t& key, const uint256_t& value)
  {
    s[key] = value;
  }

  uint256_t SimpleStorage::load(const uint256_t& key)
  {
    auto e = s.find(key);
    if (e == s.end())
      return 0;
    return e->second;
  }

  bool SimpleStorage::exists(const uint256_t& key)
  {
    return s.find(key) != s.end();
  }

  bool SimpleStorage::remove(const uint256_t& key)
  {
    auto e = s.find(key);
    if (e == s.end())
      return false;
    s.erase(e);
    return true;
  }

  bool SimpleStorage::operator==(const SimpleStorage& that) const
  {
    return s == that.s;
  }

  void to_json(nlohmann::json& j, const SimpleStorage& s)
  {
    for (const auto& p : s.s)
      j[to_hex_str(p.first)] = p.second;
  }

  void from_json(const nlohmann::json& j, SimpleStorage& s)
  {
    for (decltype(auto) it = j.cbegin(); it != j.cend(); it++)
      s.s.emplace(from_hex_str(it.key()), from_hex_str(it.value()));
  }

  inline std::ostream& operator<<(std::ostream& os, const SimpleStorage& s)
  {
    os << nlohmann::json(s).dump(2);
    return os;
  }
} // namespace eevm
