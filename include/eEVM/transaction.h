// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "address.h"

#include <nlohmann/json.hpp>
#include <vector>

namespace eevm
{
  namespace log
  {
    using Data = std::vector<uint8_t>;
    using Topic = uint256_t;
  }

  struct LogEntry
  {
    Address address;
    log::Data data;
    std::vector<log::Topic> topics;

    bool operator==(const LogEntry& that) const;

    friend void to_json(nlohmann::json&, const LogEntry&);
    friend void from_json(const nlohmann::json&, LogEntry&);
  };

  void to_json(nlohmann::json&, const LogEntry&);
  void from_json(const nlohmann::json&, LogEntry&);

  struct LogHandler
  {
    virtual ~LogHandler() = default;
    virtual void handle(LogEntry&&) = 0;
  };

  struct NullLogHandler : public LogHandler
  {
    virtual void handle(LogEntry&&) override {}
  };

  struct VectorLogHandler : public LogHandler
  {
    std::vector<LogEntry> logs;

    virtual ~VectorLogHandler() = default;
    virtual void handle(LogEntry&& e) override
    {
      logs.emplace_back(e);
    }
  };

  /**
   * Ethereum transaction
   */
  struct Transaction
  {
    const Address origin;
    const uint64_t value;
    const uint64_t gas_price;
    const uint64_t gas_limit;

    LogHandler& log_handler;
    std::vector<Address> selfdestruct_list;

    Transaction(
      const Address origin,
      LogHandler& lh,
      uint64_t value = 0,
      uint64_t gas_price = 0,
      uint64_t gas_limit = 0) :
      origin(origin),
      value(value),
      gas_price(gas_price),
      gas_limit(gas_limit),
      log_handler(lh)
    {}
  };
} // namespace eevm
