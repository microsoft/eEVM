// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "address.h"
#include "bigint.h"
#include "exception.h"

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
    virtual void set_balance(const uint256_t& b) = 0;

    virtual void pay_to(Account& other, const uint256_t& amount)
    {
      const auto this_balance = get_balance();
      if (amount > this_balance)
      {
        throw Exception(
          Exception::Type::outOfFunds,
          "Insufficient funds to pay " + to_hex_str(amount) + " to " +
            to_hex_str(other.get_address()) + " (from " +
            to_hex_str(get_address()) + ", current balance " +
            to_hex_str(this_balance) + ")");
      }

      const auto other_balance = other.get_balance();
      const auto proposed_balance = other_balance + amount;
      if (proposed_balance < other_balance)
      {
        throw Exception(
          Exception::Type::overflow,
          "Overflow while attempting to pay " + to_hex_str(amount) + " to " +
            to_hex_str(other.get_address()) + " (current balance " +
            to_hex_str(other_balance) + ") from " + to_hex_str(get_address()));
      }

      set_balance(this_balance - amount);
      other.set_balance(proposed_balance);
    }

    virtual Nonce get_nonce() const = 0;
    virtual void increment_nonce() = 0;

    virtual Code get_code() const = 0;
    virtual void set_code(Code&& code) = 0;

    virtual bool has_code()
    {
      return !get_code().empty();
    }
  };
} // namespace evm
