// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/simple/simpleglobalstate.h"

namespace eevm
{
  bool SimpleGlobalState::exists(const Address& addr)
  {
    return accounts.find(addr) != accounts.end();
  }

  void SimpleGlobalState::remove(const Address& addr)
  {
    accounts.erase(addr);
  }

  AccountState SimpleGlobalState::get(const Address& addr)
  {
    const auto acc = accounts.find(addr);
    if (acc != accounts.cend())
      return acc->second;

    return create(addr, 0, {});
  }

  AccountState SimpleGlobalState::create(
    const Address& addr, const uint256_t& balance, const Code& code)
  {
    insert({SimpleAccount(addr, balance, code), {}});

    return get(addr);
  }

  size_t SimpleGlobalState::num_accounts()
  {
    return accounts.size();
  }

  const Block& SimpleGlobalState::get_current_block()
  {
    return currentBlock;
  }

  uint256_t SimpleGlobalState::get_block_hash(uint8_t offset)
  {
    return 0u;
  }

  void SimpleGlobalState::insert(const StateEntry& p)
  {
    const auto ib = accounts.insert(std::make_pair(p.first.get_address(), p));

    assert(ib.second);
  }

  bool operator==(const SimpleGlobalState& l, const SimpleGlobalState& r)
  {
    return (l.accounts == r.accounts) && (l.currentBlock == r.currentBlock);
  }

  void to_json(nlohmann::json& j, const SimpleGlobalState& s)
  {
    j["block"] = s.currentBlock;
    auto o = nlohmann::json::array();
    for (const auto& p : s.accounts)
    {
      o.push_back({to_hex_str(p.first), p.second});
    }
    j["accounts"] = o;
  }

  void from_json(const nlohmann::json& j, SimpleGlobalState& a)
  {
    if (j.find("block") != j.end())
      assign_j(a.currentBlock, j["block"]);
    auto acc = j["accounts"].items();
    for (const auto& it : acc)
    {
      Address addr = it.value()[0];
      std::pair<Account, SimpleStorage> p = it.value()[1];
      a.accounts.insert(make_pair(addr, p));
    }
  }
} // namespace eevm
