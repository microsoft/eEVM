// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../include/transaction.h"

#include "../include/util.h"

namespace evm
{
  bool LogEntry::operator==(const LogEntry& that) const
  {
    return address == that.address && data == that.data &&
      topics == that.topics;
  }

  void to_json(nlohmann::json& j, const LogEntry& log)
  {
    j["address"] = log.address;
    j["data"] = to_hex_string(log.data);
    j["topics"] = log.topics;
  }

  void from_json(const nlohmann::json& j, LogEntry& log)
  {
    assign_j_const(log.address, j["address"]);
    assign_j_const(log.data, to_bytes(j["data"]));
    assign_j_const(log.topics, j["topics"]);
  }
} // namespace evm
