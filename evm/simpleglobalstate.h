// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "../include/globalstate.h"
#include "simpleaccount.h"
#include "simplestorage.h"

namespace evm
{
  /**
   * Simple std::map-backed implementation of GlobalState
   */
  class SimpleGlobalState : public GlobalState
  {
  public:
    using StateEntry = std::pair<SimpleAccount, SimpleStorage>;

  private:
    Block currentBlock;

    std::map<Address, StateEntry> accounts;

  public:
    SimpleGlobalState() = default;
    explicit SimpleGlobalState(Block b) : currentBlock(std::move(b)) {}

    virtual bool exists(const Address& addr) override;
    virtual void remove(const Address& addr) override;

    AccountState get(const Address& addr) override;
    AccountState create(
      const Address& addr, const uint256_t& balance, const Code& code) override;

    size_t num_accounts() override;

    virtual const Block& get_current_block() override;
    virtual uint256_t get_block_hash(uint8_t offset) override;

    /**
     * For tests which require some initial state, allow manual insertion of
     * pre-constructed accounts
     */
    void insert(const StateEntry& e);
  };
} // namespace evm
