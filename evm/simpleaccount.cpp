// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "simpleaccount.h"

#include "../include/exception.h"
#include "../include/util.h"

namespace evm
{
  Address SimpleAccount::get_address() const
  {
    return address;
  }

  void SimpleAccount::set_address(const Address& a)
  {
    address = a;
  }

  uint256_t SimpleAccount::get_balance() const
  {
    return balance;
  }

  void SimpleAccount::increment_balance(const uint256_t& amount)
  {
    const auto new_balance = balance + amount;

    if (new_balance < balance)
    {
      // This overflow is not a type of evm::Exception, throw as runtime_error
      throw std::runtime_error(
        "Overflow while incrementing balance (" + to_hex_str(balance) + " + " +
        to_hex_str(amount) + ")");
    }

    balance = new_balance;
  }

  void SimpleAccount::decrement_balance(const uint256_t& amount)
  {
    if (amount > balance)
    {
      throw Exception(
        Exception::Type::outOfFunds,
        "Insufficient funds to pay (" + to_hex_str(amount) + " > " +
          to_hex_str(balance) + ")");
    }

    balance -= amount;
  }

  void SimpleAccount::pay_to(Account& r, const uint256_t& amount)
  {
    decrement_balance(amount);
    r.increment_balance(amount);
  }

  Account::Nonce SimpleAccount::get_nonce() const
  {
    return nonce;
  }

  void SimpleAccount::increment_nonce()
  {
    ++nonce;
  }

  void SimpleAccount::set_nonce(Nonce n)
  {
    nonce = n;
  }

  Code SimpleAccount::get_code() const
  {
    return code;
  }

  bool SimpleAccount::has_code()
  {
    return !get_code().empty();
  }

  void SimpleAccount::set_code(Code&& c)
  {
    code = c;
  }

  bool SimpleAccount::operator==(const Account& a) const
  {
    return get_address() == a.get_address() &&
      get_balance() == a.get_balance() && get_nonce() == a.get_nonce() &&
      get_code() == a.get_code();
  }

  void to_json(nlohmann::json& j, const SimpleAccount& a)
  {
    j["address"] = address_to_hex_string(a.address);
    j["balance"] = a.balance;
    j["nonce"] = to_hex_string(a.nonce);
    j["code"] = to_hex_string(a.code);
  }

  void from_json(const nlohmann::json& j, SimpleAccount& a)
  {
    if (j.find("address") != j.end())
      assign_j(a.address, j["address"]);

    if (j.find("balance") != j.end())
      assign_j(a.balance, j["balance"]);

    if (j.find("nonce") != j.end())
      assign_j(a.nonce, to_uint64(j["nonce"]));

    if (j.find("code") != j.end())
      assign_j(a.code, to_bytes(j["code"]));
  }
} // namespace evm
