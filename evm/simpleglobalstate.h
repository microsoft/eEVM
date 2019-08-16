// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "../include/globalstate.h"
#include "simplestorage.h"

namespace evm
{
  /**
   * Simple std::map-backed implementation of GlobalState
   */
  class SimpleGlobalState : public GlobalState
  {
  private:
    Block currentBlock;

  protected:
    std::map<uint256_t, std::pair<Account, SimpleStorage>> accounts;

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
    void insert(std::pair<Account, SimpleStorage> p);

    friend void to_json(nlohmann::json&, const SimpleGlobalState&);
    friend void from_json(const nlohmann::json&, SimpleGlobalState&);
    friend bool operator== (const SimpleGlobalState&, const SimpleGlobalState&);
  };

  void to_json(nlohmann::json&, const SimpleGlobalState&);
  void from_json(const nlohmann::json&, SimpleGlobalState&);
  bool operator== (const SimpleGlobalState&, const SimpleGlobalState&);
} // namespace evm
