// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <exception>

namespace evm
{
  // smart contract runtime execptions
  class Exception : public std::exception
  {
  public:
    enum class Type
    {
      outOfBounds,
      outOfGas,
      outOfFunds,
      illegalInstruction,
      notImplemented
    };
    const Type type;

  private:
    const std::string msg;

  public:
    Exception(Type t, const std::string& m) : type(t), msg(m) {}

    const char* what() const noexcept override
    {
      return msg.c_str();
    }
  };

  /* exceptions of type UnexpectedState should not never be thrown under normal
  conditions. They should be impossible to reach for smart contracts. */
  class UnexpectedState : public std::exception
  {
    const char* const msg;

  public:
    UnexpectedState(const char* msg) : msg(msg) {}
    const char* what() const noexcept override
    {
      return msg;
    }
  };
} // namespace evm
