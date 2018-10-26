// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "bigint.h"
#include "util.h"

namespace evm
{
  /* A block descriptor for compatibility with regular Ethereum.
  In particular, this is used to parse cpp-ethereum test cases.*/
  struct Block
  {
    uint64_t number = 0, difficulty = 0, gas_limit = 0, timestamp = 0;
    uint256_t coinbase;
  };

  inline void from_json(const nlohmann::json& j, Block& b)
  {
    b.number = to_uint64(j["currentNumber"]);
    b.difficulty = to_uint64(j["currentDifficulty"]);
    b.gas_limit = to_uint64(j["currentGasLimit"]);
    b.timestamp = to_uint64(j["currentTimestamp"]);
    b.coinbase = from_hex_str(j["currentCoinbase"]);
  }
} // namespace evm
