// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "rlp.h"

namespace evm
{
  namespace rlp
  {
    ByteString to_byte_string(const std::string& s)
    {
      return ByteString{s.begin(), s.end()};
    }

    ByteString encode(const std::string& s)
    {
      return encode(to_byte_string(s));
    }

    uint8_t getMinimumRepresentationLength(size_t l)
    {
      return l <= 0xff ? 1u : (l <= 0xffff ? 2u : (l <= 0xffffffff ? 4u : 8u));
    }

    void writeLengthPrefixed(
      const ByteString& bs, uint8_t offset, ByteString& o_)
    {
      size_t length = bs.size();
      uint8_t lengthOfLength = getMinimumRepresentationLength(length);

      o_.clear();
      o_.reserve(1 + lengthOfLength + length);

      o_.emplace_back(offset + lengthOfLength);
      for (uint8_t i = 0; i < lengthOfLength; ++i)
      {
        o_.emplace_back((length & (0xff << i * 8)) >> i * 8);
      }

      o_.insert(o_.begin() + lengthOfLength + 1, bs.begin(), bs.end());
    }

    ByteString encode(const Item& it)
    {
      if (it.type == Item::Single)
      {
        const auto& v = it.single;
        size_t length = v.size();
        if (length <= 55)
        {
          // "For a single byte whose value is in the [0x00, 0x7f] range, that
          // byte is its own RLP encoding."
          if (length == 1 && v[0] <= 0x7f)
            return v;

          // "Otherwise, if a string is 0-55 bytes long, the RLP encoding
          // consists of a single byte with value 0x80 plus the length of the
          // string followed by the string. The range of the first byte is thus
          // [0x80, 0xb7]."
          ByteString r;
          r.emplace_back(0x80 + length);
          r.insert(r.begin() + 1, v.begin(), v.end());
          return r;
        }

        // "If a string is more than 55 bytes long, the RLP encoding consists of
        // a single byte with value 0xb7 plus the length in bytes of the length
        // of the string in binary form, followed by the length of the string,
        // followed by the string. For example, a length-1024 string would be
        // encoded as \xb9\x04\x00 followed by the string. The range of the
        // first byte is thus [0xb8, 0xbf]."
        ByteString r;
        writeLengthPrefixed(v, 0xb7, r);
        return r;
      }

      const auto& l = it.list;
      ByteString completeNested;
      for (size_t i = 0; i < l.size(); ++i)
      {
        const auto bs = encode(l[i]);
        completeNested.insert(completeNested.end(), bs.begin(), bs.end());
      }

      // "If the total payload of a list (i.e. the combined length of all its
      // items being RLP encoded) is 0-55 bytes long, the RLP encoding consists
      // of a single byte with value 0xc0 plus the length of the list followed
      // by the concatenation of the RLP encodings of the items. The range of
      // the first byte is thus [0xc0, 0xf7]."
      size_t totalLength = completeNested.size();
      if (totalLength <= 55)
      {
        ByteString r;
        r.emplace_back(0xc0 + totalLength);
        r.insert(r.begin() + 1, completeNested.begin(), completeNested.end());
        return r;
      }

      // "If the total payload of a list is more than 55 bytes long, the RLP
      // encoding consists of a single byte with value 0xf7 plus the length in
      // bytes of the length of the payload in binary form, followed by the
      // length of the payload, followed by the concatenation of the RLP
      // encodings of the items. The range of the first byte is thus [0xf8,
      // 0xff]."
      ByteString r;
      writeLengthPrefixed(completeNested, 0xf7, r);
      return r;
    }
  } // namespace rlp
} // namespace evm