// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "address.h"
#include "bigint.h"

namespace evm
{
  using Code = std::vector<uint8_t>;

  /**
   * Abstract interface for interacting with EVM accounts
   */
  struct Account
  {
    using Nonce = size_t;

    virtual ~Account() {}

    virtual Address get_address() const = 0;

    virtual uint256_t get_balance() const = 0;
    virtual void increment_balance(const uint256_t& amount) = 0;
    virtual void decrement_balance(const uint256_t& amount) = 0;
    virtual void pay(Account& r, const uint256_t& amount) = 0;

    virtual Nonce get_nonce() const = 0; // Returns new nonce after incrementing
    virtual void increment_nonce() = 0;

    virtual Code get_code() const = 0;
    virtual bool has_code() = 0;
    virtual void set_code(Code&& code_) = 0;
  };
} // namespace evm
