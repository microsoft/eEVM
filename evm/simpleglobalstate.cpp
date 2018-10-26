// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "simpleglobalstate.h"

namespace evm
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
    const Address& addr, uint256_t balance, Code code)
  {
    const auto acc = accounts.emplace(
      addr, std::make_pair(Account(addr, balance, code), SimpleStorage()));
    assert(acc.second);

    return acc.first->second;
  }

  size_t SimpleGlobalState::num_accounts()
  {
    return accounts.size();
  }

  const Block& SimpleGlobalState::get_current_block()
  {
    return currentBlock;
  }

  uint256_t SimpleGlobalState::get_block_hash(uint64_t idx)
  {
    return 0u;
  }

  void SimpleGlobalState::insert(std::pair<Account, SimpleStorage> p)
  {
    accounts.insert(std::make_pair(p.first.address, p));
  }
} // namespace evm