// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "../include/storage.h"

#include <map>

namespace evm
{
  /**
   * Simple std::map-backed implementation of Storage
   */
  class SimpleStorage : public Storage
  {
    std::map<uint256_t, uint256_t> s;

  public:
    SimpleStorage() = default;
    SimpleStorage(const nlohmann::json& j);

    void store(uint256_t key, uint256_t value) override;
    uint256_t load(const uint256_t& key) override;
    bool exists(const uint256_t& key) override;
    bool remove(const uint256_t& key) override;
    bool operator==(const Storage& that) const override;

    bool operator==(const SimpleStorage& that) const;

    friend void to_json(nlohmann::json&, const SimpleStorage&);
    friend void from_json(const nlohmann::json&, SimpleStorage&);
  };
} // namespace evm
