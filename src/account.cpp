// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/account.h"

#include "eEVM/exception.h"

#include <ostream>
#include <type_traits>

namespace eevm
{
  bool Account::has_code() const
  {
    return code.empty();
  }

  void Account::set_code(Code&& code_)
  {
    // Only set code when it hasn't been set yet
    if (has_code())
      return;

    code = code_;
  }

  void Account::pay(const uint256_t& amount)
  {
    if (amount > balance)
      throw Exception(
        Exception::Type::outOfFunds,
        "Insufficient funds to pay (" + to_hex_str(amount) + " > " +
          to_hex_str(balance) + ")");

    balance -= amount;
  }

  void Account::pay(Account& r, const uint256_t& amount)
  {
    pay(amount);
    r.balance += amount;
  }

  bool Account::operator==(const Account& that) const
  {
    return address == that.address && nonce == that.nonce &&
      balance == that.balance && code == that.code;
  }

  inline std::ostream& operator<<(std::ostream& os, const Account& a)
  {
    os << nlohmann::json(a).dump(2);
    return os;
  }

  void to_json(nlohmann::json& j, const Account& a)
  {
    j["address"] = address_to_hex_string(a.address);
    j["balance"] = a.balance;
    j["nonce"] = to_hex_string(a.nonce);
    j["code"] = to_hex_string(a.code);
  }

  void from_json(const nlohmann::json& j, Account& a)
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
} // namespace eevm
