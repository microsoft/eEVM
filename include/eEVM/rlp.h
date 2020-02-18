// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "bigint.h"
#include "util.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace eevm
{
  namespace rlp
  {
    using ByteString = std::vector<uint8_t>;

    // Helper to detect tuples
    namespace
    {
      template <typename... Ts>
      struct is_tuple : std::false_type
      {};

      template <typename... Ts>
      struct is_tuple<std::tuple<Ts...>> : std::true_type
      {};
    }

    //
    // Encoding
    //

    // Forward declaration to allow recursive calls.
    template <typename... Ts>
    ByteString encode(Ts&&... ts);

    namespace encode_details
    {
      void prefix_multiple_length(size_t total_length, ByteString& bs);

      inline ByteString to_byte_string(const ByteString& bs)
      {
        return bs;
      }

      inline ByteString to_byte_string(const std::string& s)
      {
        return ByteString(s.begin(), s.end());
      }

      template <size_t N>
      ByteString to_byte_string(const std::array<uint8_t, N>& a)
      {
        return ByteString(a.begin(), a.end());
      }

      template <typename T, size_t N>
      ByteString to_byte_string(const std::array<T, N>& a)
      {
        ByteString combined;

        for (const auto& e : a)
        {
          const auto next = encode(e);
          combined.insert(combined.end(), next.begin(), next.end());
        }

        prefix_multiple_length(combined.size(), combined);

        return combined;
      }

      template <typename T>
      ByteString to_byte_string(const std::vector<T>& v)
      {
        ByteString combined;

        for (const auto& e : v)
        {
          const auto next = encode(e);
          combined.insert(combined.end(), next.begin(), next.end());
        }

        prefix_multiple_length(combined.size(), combined);

        return combined;
      }

      inline ByteString to_byte_string(uint64_t n)
      {
        // "positive RLP integers must be represented in big endian binary form
        // with no leading zeroes"
        bool started = false;

        ByteString bs;
        for (size_t byte_index = 0; byte_index < sizeof(n); ++byte_index)
        {
          const uint64_t offset = (7 - byte_index) * 8;
          const uint64_t mask = (uint64_t)0xff << offset;

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

      inline ByteString to_byte_string(const uint256_t& n)
      {
        ByteString bs;

        // Need to special-case 0 to return an empty list, not a list containing
        // a single 0 byte
        if (n != 0)
        {
          // Get big-endian form
          uint8_t arr[32] = {};
          intx::be::store(arr, n);

          // No leading zeroes - count signficant bytes
          const auto n_bytes = intx::count_significant_words<uint8_t>(n);
          bs.resize(n_bytes);

          std::memcpy(bs.data(), arr + 32 - n_bytes, n_bytes);
        }

        return bs;
      }

      inline void prefix_multiple_length(size_t total_length, ByteString& bs)
      {
        // "If the total payload of a list (i.e. the combined length of all its
        // items being RLP encoded) is 0-55 bytes long, the RLP encoding
        // consists of a single byte with value 0xc0 plus the length of the list
        // followed by the concatenation of the RLP encodings of the items. The
        // range of the first byte is thus [0xc0, 0xf7]."
        // NB: This _should_ say '0xc0 plus the length of the concatenation of
        // RLP encoding of the items'
        if (total_length <= 55)
        {
          bs.insert(bs.begin(), 0xc0 + total_length);
          return;
        }

        // "If the total payload of a list is more than 55 bytes long, the RLP
        // encoding consists of a single byte with value 0xf7 plus the length in
        // bytes of the length of the payload in binary form, followed by the
        // length of the payload, followed by the concatenation of the RLP
        // encodings of the items. The range of the first byte is thus [0xf8,
        // 0xff]."
        auto total_length_as_bytes = to_byte_string(total_length);
        const uint8_t length_of_total_length = total_length_as_bytes.size();

        total_length_as_bytes.insert(
          total_length_as_bytes.begin(), 0xf7 + length_of_total_length);
        bs.insert(
          bs.begin(),
          total_length_as_bytes.begin(),
          total_length_as_bytes.end());
      }

      // RLP-encode a single, non-tuple argument. Convert it to a ByteString,
      // prefix with the encoded length.
      template <typename T>
      ByteString encode_single(const T& t)
      {
        auto bytes = to_byte_string(t);
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
        auto length_as_bytes = to_byte_string(length);
        const uint8_t length_of_length = length_as_bytes.size();

        length_as_bytes.insert(
          length_as_bytes.begin(), 0xb7 + length_of_length);
        bytes.insert(
          bytes.begin(), length_as_bytes.begin(), length_as_bytes.end());
        return bytes;
      }

      // RLP-encode a list of arguments. Encode each separately, and return a
      // tuple of the results.
      template <
        typename... Ts,
        typename = std::enable_if_t<!is_tuple<std::decay_t<Ts>...>::value>>
      auto encode_multiple(Ts&&... ts)
      {
        return std::make_tuple(encode(ts)...);
      }

      // RLP-encode a tuple argument. Treat this as a list of multiple of
      // arguments - encode each separately, and return a tuple of the results.
      // This allows nested heterogeneous lists to be represented as tuples.
      template <typename... Ts>
      auto encode_multiple(const std::tuple<Ts...>& tup)
      {
        return std::apply(
          [](auto&&... entry) { return std::make_tuple(encode(entry)...); },
          tup);
      }
    }

    // Main encode function. Either forwards to encode_single, or calls
    // encode_multiple then prepends list-length encoding to concatenated
    // results.
    template <typename... Ts>
    ByteString encode(Ts&&... ts)
    {
      if constexpr (sizeof...(Ts) == 1 && !is_tuple<std::decay_t<Ts>...>::value)
      {
        return encode_details::encode_single(std::forward<Ts>(ts)...);
      }

      const auto nested_terms =
        encode_details::encode_multiple(std::forward<Ts>(ts)...);

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

      encode_details::prefix_multiple_length(total_length, flattened);
      return flattened;
    }

    //
    // Decoding
    //

    class decode_error : public std::logic_error
    {
      using logic_error::logic_error;
    };

    namespace decode_details
    {
      enum class Arity
      {
        Single,
        Multiple,
      };

      struct DataSegment
      {
        Arity arity;
        const uint8_t* data;
        size_t length;
      };

      // Forward declaration to allow recursive calls.
      template <typename... Ts>
      std::tuple<Ts...> decode_impl(const uint8_t*&, size_t&);

      inline std::pair<Arity, size_t> decode_length(
        const uint8_t*& data, size_t& size);

      template <typename T>
      struct from_bytes;

      template <>
      struct from_bytes<size_t>
      {
        size_t operator()(const uint8_t*& data, size_t& size)
        {
          if (size > 8)
          {
            throw decode_error(fmt::format(
              "Trying to decode number: {}  is too many bytes for uint64_t",
              size));
          }

          size_t result = 0;

          while (size > 0)
          {
            result <<= 8u;
            result |= *data;
            data++;
            size--;
          }

          return result;
        }
      };

      template <typename T>
      struct from_bytes
      {
        std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, T>
        operator()(const uint8_t*& data, size_t& size)
        {
          return (T)from_bytes<size_t>{}(data, size);
        }
      };

      template <>
      struct from_bytes<std::string>
      {
        std::string operator()(const uint8_t*& data, size_t& size)
        {
          std::string result(size, '\0');

          for (auto i = 0u; i < size; ++i)
          {
            result[i] = *data++;
          }

          size = 0u;

          return result;
        }
      };

      template <size_t N>
      struct from_bytes<std::array<uint8_t, N>>
      {
        std::array<uint8_t, N> operator()(const uint8_t*& data, size_t& size)
        {
          if (size != N)
          {
            throw decode_error(fmt::format(
              "Trying to decode {} byte array, but given {} bytes to decode",
              N,
              size));
          }

          std::array<uint8_t, N> result;
          std::copy(data, data + size, result.begin());

          data = data + size;
          size = 0u;

          return result;
        }
      };

      template <typename T, size_t N>
      struct from_bytes<std::array<T, N>>
      {
        std::array<T, N> operator()(const uint8_t*& data, size_t& size)
        {
          decode_length(data, size);

          std::array<T, N> result;
          for (auto i = 0u; i < N; ++i)
          {
            result[i] = std::get<0>(decode_impl<T>(data, size));
          }

          size = 0u;

          return result;
        }
      };

      template <typename T>
      struct from_bytes<std::vector<T>>
      {
        std::vector<T> operator()(const uint8_t*& data, size_t& size)
        {
          auto contained_length = decode_length(data, size).second;

          std::vector<T> result;
          while (contained_length > 0)
          {
            result.push_back(
              std::get<0>(decode_impl<T>(data, contained_length)));
          }

          size = 0u;

          return result;
        }
      };

      template <>
      struct from_bytes<ByteString>
      {
        ByteString operator()(const uint8_t*& data, size_t& size)
        {
          ByteString result(size);

          for (auto i = 0u; i < size; ++i)
          {
            result[i] = *data++;
          }

          size = 0u;

          return result;
        }
      };

      template <>
      struct from_bytes<uint256_t>
      {
        uint256_t operator()(const uint8_t*& data, size_t& size)
        {
          uint256_t result = 0u;

          if (size > 0)
          {
            result = from_big_endian(data, size);
          }

          data += size;
          size = 0u;

          return result;
        }
      };

      inline std::pair<Arity, size_t> decode_length(
        const uint8_t*& data, size_t& size)
      {
        if (size == 0)
        {
          throw decode_error("Trying to decode length: got empty data");
        }

        // First byte IS the data
        if (data[0] <= 0x7f)
        {
          return {Arity::Single, 1};
        }

        // First byte is length information - consume it now
        const size_t length = data[0];
        data++;
        size--;

        // Data is a single item, with length encoded in first bytes
        if (length <= 0xb7)
        {
          return {Arity::Single, length - 0x80};
        }

        // Data is a single item, with length encoded in next bytes
        if (length <= 0xbf)
        {
          size_t length_of_length = length - 0xb7;

          if (size < length_of_length)
          {
            throw decode_error(fmt::format(
              "Length of next element should be encoded in {} bytes, but only "
              "{} remain",
              length_of_length,
              size));
          }

          size -= length_of_length;

          // This should advance data and decrement length_of_length to 0
          const size_t content_length =
            from_bytes<size_t>{}(data, length_of_length);
          return {Arity::Single, content_length};
        }

        // Data encodes a list, with total content-length encoded in first byte
        if (length <= 0xf7)
        {
          return {Arity::Multiple, length - 0xc0};
        }

        // Data encodes a list, with total content-length encoded in next bytes
        size_t length_of_length = length - 0xf7;

        if (size < length_of_length)
        {
          throw decode_error(fmt::format(
            "Length of next list should be encoded in {} bytes, but only {} "
            "remain",
            length_of_length,
            size));
        }

        size -= length_of_length;

        const size_t content_length =
          from_bytes<size_t>{}(data, length_of_length);
        return {Arity::Multiple, content_length};
      }

      // General template for decoding a tuple. Type inferred from of third
      // parameter (an unused tag)
      template <typename... Ts>
      std::tuple<Ts...> decode_tuple(
        const uint8_t*& data, size_t& size, std::tuple<Ts...>);

      // Specialisation for decoding empty tuples
      template <>
      inline std::tuple<> decode_tuple(
        const uint8_t*& data, size_t& size, std::tuple<>)
      {
        return std::make_tuple();
      }

      // Specialisation for recursively decoding tuples, ensuring the first item
      // is read before the others
      template <typename T, typename... Ts>
      inline std::tuple<T, Ts...> decode_tuple(
        const uint8_t*& data, size_t& size, std::tuple<T, Ts...>)
      {
        const auto first = decode_impl<T>(data, size);

        return std::tuple_cat(
          first, decode_tuple<Ts...>(data, size, std::tuple<Ts...>{}));
      }

      // Type helper for decoding single item, forwarding to either decode_tuple
      // (to unwrap the types contained in a tuple) or directly to the top-level
      // decode_impl function
      template <typename T>
      auto decode_item(const uint8_t*& data, size_t& size)
      {
        if constexpr (is_tuple<std::decay_t<T>>::value)
        {
          return std::make_tuple(decode_tuple(data, size, T{}));
        }
        else
        {
          return decode_impl<T>(data, size);
        }
      }

      // Type helper for decoding multiple items, decoding the first and then
      // potentially recursing to concatenate the decoded tail
      template <typename T, typename... Ts>
      std::tuple<T, Ts...> decode_multiple(const uint8_t*& data, size_t& size)
      {
        const auto first = decode_item<T>(data, size);

        if constexpr (sizeof...(Ts) == 0)
        {
          return first;
        }
        else
        {
          return std::tuple_cat(first, decode_multiple<Ts...>(data, size));
        }
      }

      // Main decode_impl function. Reads initial length, then either converts a
      // single item from remaining bytes or forwards to decode_multiple
      template <typename... Ts>
      std::tuple<Ts...> decode_impl(const uint8_t*& data, size_t& size)
      {
        auto [arity, contained_length] = decode_length(data, size);

        if constexpr (
          sizeof...(Ts) == 1 && !is_tuple<std::decay_t<Ts>...>::value)
        {
          if (arity != Arity::Single)
          {
            throw decode_error("Expected single item, but data encodes a list");
          }

          size -= contained_length;

          return std::make_tuple(from_bytes<Ts...>{}(data, contained_length));
        }

        if (arity != Arity::Multiple)
        {
          throw decode_error(
            "Expected list item, but data encodes a single item");
        }

        if constexpr (sizeof...(Ts) == 0)
        {
          if (contained_length != 0)
          {
            throw decode_error(fmt::format(
              "Expected empty list, but data contains {} remaining bytes",
              contained_length));
          }

          return std::make_tuple();
        }
        else
        {
          size -= contained_length;

          return decode_multiple<Ts...>(data, contained_length);
        }
      }
    }

    // Core helper. Forwards to decode_impl, ensures entire input has been
    // consumed
    template <typename... Ts>
    std::tuple<Ts...> decode(const uint8_t*& data, size_t& size)
    {
      auto res = decode_details::decode_impl<Ts...>(data, size);

      if (size != 0)
      {
        throw decode_error(fmt::format(
          "Expected to decode entire input, but {} bytes remain", size));
      }

      return res;
    }

    // Helper. Takes ByteString and forwards contained data+size to main
    // decode function
    template <typename... Ts>
    std::tuple<Ts...> decode(const ByteString& bytes)
    {
      const uint8_t* data = bytes.data();
      size_t size = bytes.size();
      return decode<Ts...>(data, size);
    }

    // Helper. Unwraps tuple in case where there is a single top-level decoded
    // value
    template <typename T>
    T decode_single(const ByteString& bytes)
    {
      const uint8_t* data = bytes.data();
      size_t size = bytes.size();
      const std::tuple<T> tup = decode<T>(data, size);
      return std::get<0>(tup);
    }
  }
} // namespace eevm
