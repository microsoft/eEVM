// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/simple/simpleaccount.h"

#include "eEVM/util.h"

namespace eevm
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

  void SimpleAccount::set_balance(const uint256_t& b)
  {
    balance = b;
  }

  Account::Nonce SimpleAccount::get_nonce() const
  {
    return nonce;
  }

  void SimpleAccount::set_nonce(Nonce n)
  {
    nonce = n;
  }

  void SimpleAccount::increment_nonce()
  {
    ++nonce;
  }

  Code SimpleAccount::get_code() const
  {
    return code;
  }

  void SimpleAccount::set_code(Code&& c)
  {
    code = c;
  }

  bool SimpleAccount::has_code()
  {
    return !get_code().empty();
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
    j["balance"] = to_hex_string(a.balance);
    j["nonce"] = to_hex_string(a.nonce);
    j["code"] = to_hex_string(a.code);
  }

  void from_json(const nlohmann::json& j, SimpleAccount& a)
  {
    if (j.find("address") != j.end())
    {
      a.address = to_uint256(j["address"]);
    }

    if (j.find("balance") != j.end())
    {
      a.balance = to_uint256(j["balance"]);
    }

    if (j.find("nonce") != j.end())
    {
      a.nonce = to_uint64(j["nonce"]);
    }

    if (j.find("code") != j.end())
    {
      a.code = to_bytes(j["code"]);
    }
  }
} // namespace eevm
