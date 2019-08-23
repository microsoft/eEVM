// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "eEVM/account.h"

#include <nlohmann/json.hpp>

namespace eevm
{
  /**
   * Simple implementation of Account
   */
  class SimpleAccount : public Account
  {
  private:
    Address address = {};
    uint256_t balance = {};
    Code code = {};
    Nonce nonce = {};

  public:
    SimpleAccount() = default;

    SimpleAccount(const Address& a, const uint256_t& b, const Code& c) :
      address(a),
      balance(b),
      code(c),
      nonce(0)
    {}

    SimpleAccount(
      const Address& a, const uint256_t& b, const Code& c, Nonce n) :
      address(a),
      balance(b),
      code(c),
      nonce(n)
    {}

    virtual Address get_address() const override;
    void set_address(const Address& a);

    virtual uint256_t get_balance() const override;
    virtual void set_balance(const uint256_t& b) override;

    virtual Nonce get_nonce() const override;
    void set_nonce(Nonce n);
    virtual void increment_nonce() override;

    virtual Code get_code() const override;
    virtual void set_code(Code&& c) override;
    virtual bool has_code() override;

    bool operator==(const Account&) const;

    friend void to_json(nlohmann::json&, const SimpleAccount&);
    friend void from_json(const nlohmann::json&, SimpleAccount&);
  };

  void to_json(nlohmann::json&, const SimpleAccount&);
  void from_json(const nlohmann::json&, SimpleAccount&);
} // namespace eevm
