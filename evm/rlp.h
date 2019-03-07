// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
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

  namespace rlp_test
  {
    using ByteString = std::vector<uint8_t>;

    template <typename... Ts>
    ByteString encode(Ts&&... ts);

    inline ByteString to_bytes(const ByteString& bs)
    {
      return bs;
    }

    inline ByteString to_bytes(const std::string& s)
    {
      return ByteString(s.begin(), s.end());
    }

    inline ByteString to_bytes(uint64_t n)
    {
      // "positive RLP integers must be represented in big endian binary form
      // with no leading zeroes"
      bool started = false;

      ByteString bs;
      for (size_t byte_index = 0; byte_index < sizeof(n); ++byte_index)
      {
        const uint64_t offset = (7 - byte_index) * 8;
        const uint64_t mask = 0xff << offset;

        const uint8_t current_byte = (mask & n) >> offset;

        if (started || current_byte != 0)
        {
          bs.push_back(current_byte);
          started = true;
        }
      }

      // NB: If n is 0, this returns an empty list - this is correct.
      return bs;
    }

    template <typename T>
    ByteString encode_single(const T& t)
    {
      auto bytes = to_bytes(t);
      const auto length = bytes.size();

      // "For a single byte whose value is in the [0x00, 0x7f] range, that
      // byte is its own RLP encoding."
      if (length == 1 && bytes[0] <= 0x7f)
      {
        return bytes;
      }

      // "Otherwise, if a string is 0-55 bytes long, the RLP encoding
      // consists of a single byte with value 0x80 plus the length of the
      // string followed by the string. The range of the first byte is thus
      // [0x80, 0xb7]."
      if (length <= 55)
      {
        bytes.insert(bytes.begin(), 0x80 + length);
        return bytes;
      }

      // "If a string is more than 55 bytes long, the RLP encoding consists of
      // a single byte with value 0xb7 plus the length in bytes of the length
      // of the string in binary form, followed by the length of the string,
      // followed by the string. For example, a length-1024 string would be
      // encoded as \xb9\x04\x00 followed by the string. The range of the
      // first byte is thus [0xb8, 0xbf]."
      auto length_as_bytes = to_bytes(length);
      const uint8_t length_of_length = length_as_bytes.size();

      length_as_bytes.insert(length_as_bytes.begin(), 0xb7 + length_of_length);
      bytes.insert(
        bytes.begin(), length_as_bytes.begin(), length_as_bytes.end());
      return bytes;
    }

    namespace
    {
      template <typename... Ts>
      struct is_tuple : std::false_type
      {};

      template <typename... Ts>
      struct is_tuple<std::tuple<Ts...>> : std::true_type
      {};

      template <typename... Ts>
      struct is_tuple<std::tuple<Ts...>&> : std::true_type
      {};
    }

    template <typename... Ts>
    auto encode_multiple(Ts&&... ts)
    {
      return std::tuple_cat(std::make_tuple(encode(ts)...));
    }

    template <typename... Ts>
    auto encode_multiple(std::tuple<Ts...>& tup)
    {
      return std::apply(
        [](auto... entry) {
          return std::tuple_cat(std::make_tuple(encode(entry)...));
        },
        tup);
    }

    template <typename... Ts>
    ByteString encode(Ts&&... ts)
    {
      if constexpr (sizeof...(Ts) == 1 && !is_tuple<Ts...>::value)
      {
        return encode_single(std::forward<Ts...>(ts)...);
      }

      const auto nested_terms = encode_multiple(ts...);

      // Get total length by summing size of each term in tuple
      const auto total_length = std::apply(
        [](auto&&... term) { return (term.size() + ... + 0); }, nested_terms);

      ByteString flattened;

      // Get concatenation by inserting each term's encoding
      std::apply(
        [&flattened](auto&&... term) {
          ((flattened.insert(flattened.end(), term.begin(), term.end()), ...));
        },
        nested_terms);

      // "If the total payload of a list (i.e. the combined length of all its
      // items being RLP encoded) is 0-55 bytes long, the RLP encoding
      // consists of a single byte with value 0xc0 plus the length of the list
      // followed by the concatenation of the RLP encodings of the items. The
      // range of the first byte is thus [0xc0, 0xf7]."
      // NB: This _should_ say '0xc0 plus the length of the concatenation of
      // RLP encoding of the items'
      if (total_length <= 55)
      {
        flattened.insert(flattened.begin(), 0xc0 + total_length);
        return flattened;
      }

      // "If the total payload of a list is more than 55 bytes long, the RLP
      // encoding consists of a single byte with value 0xf7 plus the length in
      // bytes of the length of the payload in binary form, followed by the
      // length of the payload, followed by the concatenation of the RLP
      // encodings of the items. The range of the first byte is thus [0xf8,
      // 0xff]."
      auto total_length_as_bytes = to_bytes(total_length);
      const uint8_t length_of_total_length = total_length_as_bytes.size();

      total_length_as_bytes.insert(
        total_length_as_bytes.begin(), 0xf7 + length_of_total_length);
      flattened.insert(
        flattened.begin(),
        total_length_as_bytes.begin(),
        total_length_as_bytes.end());
      return flattened;
    }
  }
} // namespace evm
