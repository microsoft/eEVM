// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "bigint.h"
#include "util.h"

namespace eevm
{
  /**
   * An Ethereum block descriptor; in particular, this is used to parse
   * cpp-ethereum test cases.
   */
  struct Block
  {
    uint64_t number = 0, difficulty = 0, gas_limit = 0, timestamp = 0;
    uint256_t coinbase;
  };

  inline bool operator==(const Block& l, const Block& r)
  {
    return l.coinbase == r.coinbase && l.number == r.number &&
      l.difficulty == r.difficulty && l.gas_limit == r.gas_limit &&
      l.timestamp == r.timestamp;
  }

  inline void from_json(const nlohmann::json& j, Block& b)
  {
    b.number = to_uint64(j["currentNumber"]);
    b.difficulty = to_uint64(j["currentDifficulty"]);
    b.gas_limit = to_uint64(j["currentGasLimit"]);
    b.timestamp = to_uint64(j["currentTimestamp"]);
    b.coinbase = to_uint256(j["currentCoinbase"]);
  }

  inline void to_json(nlohmann::json& j, const Block& b)
  {
    j["currentNumber"] = to_hex_string(b.number);
    j["currentDifficulty"] = to_hex_string(b.difficulty);
    j["currentGasLimit"] = to_hex_string(b.gas_limit);
    j["currentTimestamp"] = to_hex_string(b.timestamp);
    j["currentCoinbase"] = to_hex_string(b.coinbase);
  }
} // namespace eevm
