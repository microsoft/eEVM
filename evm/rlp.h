// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace evm
{
  namespace rlp
  {
    using ByteString = std::vector<uint8_t>;

    // TODO: This could be std::variant
    struct Item
    {
      using TList = std::vector<Item>;

      enum
      {
        Single,
        List
      } type;

      union
      {
        ByteString single;
        TList list;
      };

      Item() : type(Single), single() {}
      Item(const ByteString& i) : type(Single), single(i) {}
      Item(const TList& l) : type(List), list(l) {}
      Item(const Item& it) : type(it.type), single()
      {
        if (type == Single)
          single = it.single;
        else
          list = it.list;
      }
      Item(std::initializer_list<Item> l) : type(List), list(l) {}

      Item& operator=(const Item& other)
      {
        type = other.type;
        if (type == Single)
          single = other.single;
        else
          list = other.list;
        return *this;
      }

      bool operator==(const Item& other) const
      {
        if (type != other.type)
        {
          return false;
        }

        if (type == Single)
          return single == other.single;
        else
          return list == other.list;
      }

      bool operator!=(const Item& other) const
      {
        return !(*this == other);
      }

      ~Item()
      {
        if (type == Single)
          single.~ByteString();
        else
          list.~TList();
      }
    };

    ByteString to_byte_string(const std::string& s);

    ByteString encode(const std::string& s);
    ByteString encode(const Item& it);

  } // namespace rlp
} // namespace evm
