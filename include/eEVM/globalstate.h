// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "account.h"
#include "block.h"
#include "storage.h"

#include <map>

namespace eevm
{
  /**
   * An account and its storage
   */
  struct AccountState
  {
    Account& acc;
    Storage& st;

    template <
      typename T,
      typename U,
      typename = std::enable_if_t<std::is_base_of<Account, T>::value>,
      typename = std::enable_if_t<std::is_base_of<Storage, U>::value>>
    AccountState(std::pair<T, U>& p) : acc(p.first), st(p.second)
    {}
    AccountState(Account& acc, Storage& st) : acc(acc), st(st) {}
  };

  /**
   * Abstract interface for interacting with EVM world state
   */
  struct GlobalState
  {
    virtual bool exists(const Address& addr) = 0;
    virtual void remove(const Address& addr) = 0;

    /**
     * Creates a new zero-initialized account under the given address if none
     * exists
     */
    virtual AccountState get(const Address& addr) = 0;
    virtual AccountState create(
      const Address& addr, const uint256_t& balance, const Code& code) = 0;

    virtual size_t num_accounts() = 0;

    virtual const Block& get_current_block() = 0;
    virtual uint256_t get_block_hash(uint8_t offset) = 0;
  };
} // namespace eevm
