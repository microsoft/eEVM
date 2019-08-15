// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "../include/account.h"

#include <nlohmann/json.hpp>

namespace evm
{
  /**
   * Simple implementation of Account
   */
  class SimpleAccount : public Account
  {
  private:
    Address address = {};
    uint256_t balance = {};
    Nonce nonce = {};
    Code code = {};

  public:
    SimpleAccount() = default;

    SimpleAccount(const Address& a, const uint256_t& b, const Code& c) :
      address(a),
      balance(b),
      nonce(0),
      code(c)
    {}

    virtual Address get_address() const override;
    void set_address(const Address& a);

    virtual uint256_t get_balance() const override;
    virtual void increment_balance(const uint256_t& amount) override;
    virtual void decrement_balance(const uint256_t& amount) override;
    virtual void pay(Account& r, const uint256_t& amount) override;

    virtual Nonce get_nonce() const override;
    virtual void increment_nonce() override;
    void set_nonce(Nonce n);

    virtual Code get_code() const override;
    virtual bool has_code() override;
    virtual void set_code(Code&& c) override;

    bool operator==(const Account&) const;

    friend void to_json(nlohmann::json&, const SimpleAccount&);
    friend void from_json(const nlohmann::json&, SimpleAccount&);
  };
} // namespace evm
