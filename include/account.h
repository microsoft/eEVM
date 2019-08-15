// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "address.h"
#include "bigint.h"
#include "storage.h"
#include "util.h"

#include <nlohmann/json.hpp>

namespace evm
{
  using Code = std::vector<uint8_t>;

  struct Account
  {
    Address address = {};
    uint64_t nonce = {};
    uint256_t balance = {};
    Code code = {};

    Account() = default;
    Account(
      const Address& address, const uint256_t& balance, const Code& code) :
      address(address),
      nonce(0),
      balance(balance),
      code(code)
    {}

    Account(
      const Address& address,
      uint64_t nonce,
      const uint256_t& balance,
      const Code& code) :
      address(address),
      nonce(nonce),
      balance(balance),
      code(code)
    {}

    bool has_code() const;
    void set_code(Code&& code_);

    void pay(const uint256_t& amount);
    void pay(Account& r, const uint256_t& amount);

    bool operator==(const Account& that) const;

    friend void to_json(nlohmann::json&, const Account&);
    friend void from_json(const nlohmann::json&, Account&);
  };

  void to_json(nlohmann::json&, const Account&);
  void from_json(const nlohmann::json&, Account&);
} // namespace evm
