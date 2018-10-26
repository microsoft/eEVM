// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../include/account.h"

#include "../include/exception.h"

#include <ostream>
#include <type_traits>

namespace evm
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
    j["address"] = a.address;
    j["balance"] = a.balance;
    j["nonce"] = a.nonce;
    j["code"] = a.code;
  }

  void from_json(const nlohmann::json& j, Account& a)
  {
    assign_j(a.address, j["address"]);
    assign_j(a.balance, j["balance"]);
    assign_j(a.nonce, j["nonce"]);
    assign_j(a.code, j["code"]);
  }
} // namespace evm