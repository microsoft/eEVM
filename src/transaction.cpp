// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/transaction.h"

#include "eEVM/util.h"

namespace eevm
{
  bool LogEntry::operator==(const LogEntry& that) const
  {
    return address == that.address && data == that.data &&
      topics == that.topics;
  }

  void to_json(nlohmann::json& j, const LogEntry& log)
  {
    j["address"] = to_checksum_address(log.address);
    j["data"] = to_hex_string(log.data);

    auto topics_array = nlohmann::json::array();
    for (const auto& topic : log.topics)
    {
      topics_array.push_back(to_hex_string_fixed(topic));
    }
    j["topics"] = topics_array;

    // Fill in all specified fields for compliance, so this can be parsed by
    // standard tools
    j["logIndex"] = "0x0";
    j["blockNumber"] = "0x0";
    j["blockHash"] = "0x0";
    j["transactionHash"] = "0x0";
    j["transactionIndex"] = "0x0";
  }

  void from_json(const nlohmann::json& j, LogEntry& log)
  {
    log.address = to_uint256(j["address"]);
    log.data = to_bytes(j["data"]);
    for (const auto& topic : j["topics"])
    {
      log.topics.push_back(to_uint256(topic));
    }
  }
} // namespace eevm
