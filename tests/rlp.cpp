// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/rlp.h"

#include "eEVM/util.h"

#include <doctest/doctest.h>

using namespace eevm;
using namespace std::string_literals;

const auto large_input_decoded = std::make_tuple(
  std::make_tuple("Hello world"s, "Saluton Mondo"s),
  std::make_tuple(
    std::make_tuple(
      std::make_tuple(1u),
      std::make_tuple(2u, 3u),
      std::make_tuple(std::make_tuple(4u))),
    66000u),
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
  "tempor incididunt ut labore et dolore magna aliqua"s);

const std::string large_input_encoded_s =
  "\xf8\xa5\xda\x8bHello world\x8dSaluton "
  "Mondo\xcd\xc8\xc1\x01\xc2\x02\x03\xc2\xc1\x04\x83\x01\x01\xd0\xb8zLorem "
  "ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
  "tempor incididunt ut labore et dolore magna aliqua";

const auto large_input_encoded = std::vector<uint8_t>(
  large_input_encoded_s.begin(), large_input_encoded_s.end());

TEST_CASE("encode" * doctest::test_suite("rlp"))
{
  CHECK(rlp::encode(0x0) == rlp::ByteString{0x80});
  CHECK(rlp::encode(0x1) == rlp::ByteString{0x1});
  CHECK(rlp::encode(0x7f) == rlp::ByteString{0x7f});
  CHECK(rlp::encode(0x80) == rlp::ByteString{0x81, 0x80});

  CHECK(rlp::encode() == rlp::ByteString{0xc0});
  CHECK(rlp::encode("") == rlp::ByteString{0x80});

  CHECK(rlp::encode(0x0, 0x0) == rlp::ByteString{0xc2, 0x80, 0x80});
  CHECK(rlp::encode(0x1, 0x2, 0x3) == rlp::ByteString{0xc3, 0x1, 0x2, 0x3});

  CHECK(rlp::encode("dog") == rlp::ByteString{0x83, 'd', 'o', 'g'});
  CHECK(
    rlp::encode("cat", "dog") ==
    rlp::ByteString{0xc8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g'});

  CHECK(rlp::encode(1024) == rlp::ByteString{0x82, 0x04, 0x00});

  CHECK(rlp::encode(rlp::ByteString{0x0}) == rlp::ByteString{0x0});

  CHECK(
    rlp::encode(rlp::ByteString{0x0, 0x0}) == rlp::ByteString{0x82, 0x0, 0x0});

  CHECK(rlp::encode(std::make_tuple(0x0)) == rlp::ByteString{0xc1, 0x80});
  CHECK(
    rlp::encode(std::make_tuple(0x0, 0x0)) ==
    rlp::ByteString{0xc2, 0x80, 0x80});
  CHECK(
    rlp::encode(std::make_tuple(std::make_tuple(0x0))) ==
    rlp::ByteString{0xc2, 0xc1, 0x80});

  const auto set_0 = std::make_tuple();
  CHECK(rlp::encode(set_0) == rlp::ByteString{0xc0});

  const auto set_1 = std::make_tuple(set_0);
  CHECK(rlp::encode(set_1) == rlp::ByteString{0xc1, 0xc0});

  const auto set_2 = std::make_tuple(set_0, set_1);
  const auto set_3 = std::make_tuple(set_0, set_1, set_2);
  CHECK(
    rlp::encode(set_3) ==
    rlp::ByteString{0xc7, 0xc0, 0xc1, 0xc0, 0xc3, 0xc0, 0xc1, 0xc0});

  CHECK(rlp::encode(large_input_decoded) == large_input_encoded);
}

TEST_CASE("decode" * doctest::test_suite("rlp"))
{
  // NB: Return value of decode is wrapped in a tuple. If requesting a single
  // top-level value, this layer can be popped for you by calling decode_single
  CHECK(rlp::decode<size_t>(rlp::ByteString{0x80}) == std::make_tuple(0x0));
  CHECK(rlp::decode_single<size_t>(rlp::ByteString{0x80}) == 0x0);
  CHECK(rlp::decode_single<size_t>(rlp::ByteString{0x1}) == 0x1);
  CHECK(rlp::decode_single<size_t>(rlp::ByteString{0x7f}) == 0x7f);
  CHECK(rlp::decode_single<size_t>(rlp::ByteString{0x81, 0x80}) == 0x80);
  CHECK_THROWS(rlp::decode_single<size_t>(rlp::ByteString{0x81, 0x80, 0x00}));

  CHECK(rlp::decode<>(rlp::ByteString{0xc0}) == std::make_tuple());
  CHECK(rlp::decode<std::string>(rlp::ByteString{0x80}) == std::make_tuple(""));

  CHECK(
    rlp::decode<size_t, size_t>(rlp::ByteString{0xc2, 0x80, 0x80}) ==
    std::make_tuple(0x0, 0x0));
  CHECK(
    rlp::decode<size_t, size_t, size_t>(rlp::ByteString{0xc3, 0x1, 0x2, 0x3}) ==
    std::make_tuple(0x1, 0x2, 0x3));

  const rlp::ByteString dog{0x83, 'd', 'o', 'g'};
  CHECK(rlp::decode_single<std::string>(dog) == "dog");
  CHECK(rlp::decode<std::string>(dog) == std::make_tuple("dog"));

  const rlp::ByteString cat_dog{0xc8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g'};
  CHECK(
    rlp::decode<std::string, std::string>(cat_dog) ==
    std::make_tuple("cat", "dog"));

  CHECK(rlp::decode_single<size_t>(rlp::ByteString{0x82, 0x04, 0x00}) == 1024);

  CHECK(
    rlp::decode_single<rlp::ByteString>(rlp::ByteString{0x0}) ==
    rlp::ByteString{0x0});

  CHECK(
    rlp::decode_single<rlp::ByteString>(rlp::ByteString{0x82, 0x0, 0x0}) ==
    rlp::ByteString{0x0, 0x0});

  CHECK(
    rlp::decode<std::tuple<size_t>>(rlp::ByteString{0xc1, 0x80}) ==
    std::make_tuple(std::make_tuple(0x0)));
  CHECK(
    rlp::decode<std::tuple<size_t, size_t>>(rlp::ByteString{
      0xc2, 0x80, 0x80}) == std::make_tuple(std::make_tuple(0x0, 0x0)));
  CHECK(
    rlp::decode<std::tuple<std::tuple<size_t>>>(
      rlp::ByteString{0xc2, 0xc1, 0x80}) ==
    std::make_tuple(std::make_tuple(std::make_tuple(0x0))));
  CHECK_THROWS(rlp::decode<std::tuple<std::tuple<size_t>>>(
    rlp::ByteString{0xc2, 0xc1, 0x80, 0x00}));

  CHECK(
    rlp::decode_single<
      std::tuple<size_t, size_t, size_t, size_t, size_t, size_t>>(
      rlp::ByteString{0xc6, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6}) ==
    std::make_tuple(0x1, 0x2, 0x3, 0x4, 0x5, 0x6));

  auto set_0 = std::make_tuple();
  CHECK(rlp::decode_single<decltype(set_0)>(rlp::ByteString{0xc0}) == set_0);

  auto set_1 = std::make_tuple(set_0);
  CHECK(
    rlp::decode_single<decltype(set_1)>(rlp::ByteString{0xc1, 0xc0}) == set_1);

  auto set_2 = std::make_tuple(set_0, set_1);
  CHECK(
    rlp::decode_single<decltype(set_2)>(
      rlp::ByteString{0xc3, 0xc0, 0xc1, 0xc0}) == set_2);

  auto set_3 = std::make_tuple(set_0, set_1, set_2);
  CHECK(
    rlp::decode_single<decltype(set_3)>(rlp::ByteString{
      0xc7, 0xc0, 0xc1, 0xc0, 0xc3, 0xc0, 0xc1, 0xc0}) == set_3);

  CHECK(
    rlp::decode<decltype(large_input_decoded)>(large_input_encoded) ==
    std::make_tuple(large_input_decoded));
}

TEST_CASE("arrays" * doctest::test_suite("rlp"))
{
  {
    std::array<uint8_t, 100> a;
    for (size_t i = 0; i < a.size(); ++i)
    {
      a[i] = i * i;
    }

    const auto encoded = rlp::encode(a);
    CHECK(rlp::decode_single<decltype(a)>(encoded) == a);
  }
}

TEST_CASE("uint256_t" * doctest::test_suite("rlp"))
{
  uint256_t zero_decoded = 0x0;
  auto zero_encoded = rlp::ByteString{0x80};

  uint256_t small_decoded = 1024;
  auto small_encoded = rlp::ByteString{0x82, 0x04, 0x00};

  using namespace intx;
  uint256_t large_decoded = 0x1234567890abcdefdeadbeefcafef00dbaaaad_u256;
  auto large_encoded =
    rlp::ByteString{0x93, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xde,
                    0xad, 0xbe, 0xef, 0xca, 0xfe, 0xf0, 0x0d, 0xba, 0xaa, 0xad};

  SUBCASE("encode")
  {
    CHECK(rlp::encode(zero_decoded) == zero_encoded);
    CHECK(rlp::encode(small_decoded) == small_encoded);
    CHECK(rlp::encode(large_decoded) == large_encoded);
  }

  SUBCASE("decode")
  {
    CHECK(
      rlp::decode_single<decltype(zero_decoded)>(zero_encoded) == zero_decoded);
    CHECK(
      rlp::decode_single<decltype(small_decoded)>(small_encoded) ==
      small_decoded);
    CHECK(
      rlp::decode_single<decltype(large_decoded)>(large_encoded) ==
      large_decoded);
  }
}

TEST_CASE_TEMPLATE(
  "integral" * doctest::test_suite("rlp"),
  T,
  uint8_t,
  uint16_t,
  uint32_t,
  uint64_t)
{
  using TVec = std::vector<T>;
  TVec v{0,
         1,
         std::numeric_limits<T>::max(),
         std::numeric_limits<T>::max() / 2,
         std::numeric_limits<T>::max() / 3};
  for (auto n : v)
  {
    auto encoded = rlp::encode(n);
    CHECK(rlp::decode_single<T>(encoded) == n);
  }
}

TEST_CASE("nested" * doctest::test_suite("rlp"))
{
  {
    using T = std::array<std::string, 3>;
    {
      T empty{};
      const auto encoded = rlp::encode(empty);
      CHECK(rlp::decode_single<decltype(empty)>(encoded) == empty);
    }

    {
      T a;
      a[0] = "Hello";
      a[1] = "Hello world";
      a[2] = "Saluton mondo";
      const auto encoded = rlp::encode(a);
      CHECK(rlp::decode_single<decltype(a)>(encoded) == a);
    }
  }

  {
    using T = std::vector<std::string>;
    {
      T empty{};
      const auto encoded = rlp::encode(empty);
      CHECK(rlp::decode_single<decltype(empty)>(encoded) == empty);
    }

    {
      T v;
      v.push_back("Hello");
      v.push_back("Hello world");
      v.push_back("Saluton mondo");
      const auto encoded = rlp::encode(v);
      CHECK(rlp::decode_single<decltype(v)>(encoded) == v);
    }
  }

  {
    using L0 = std::vector<std::string>;
    using L1 = std::array<L0, 2>;
    using L2 = std::vector<L1>;
    using L3 = std::array<L2, 4>;
    {
      L3 empty{};
      const auto encoded = rlp::encode(empty);
      CHECK(rlp::decode_single<decltype(empty)>(encoded) == empty);
    }

    {
      L3 nest{L2{L1{L0{"a", "b"}, L0{"cd", "efghi", "jkl"}}, L1{}},
              L2{},
              L2{L1{L0{"mnopqr", "s"}}},
              L2{L1{L0{"t"}, L0{"uv"}}, L1{L0{"wx"}, L0{"yz"}}}};
      const auto encoded = rlp::encode(nest);
      CHECK(rlp::decode_single<decltype(nest)>(encoded) == nest);
    }
  }
}

struct UserType
{
  size_t a;
  char b;
  bool c;
  size_t d[3];
};

bool operator==(const UserType& l, const UserType& r)
{
  return l.a == r.a && l.b == r.b && l.c == r.c && l.d[0] == r.d[0] &&
    l.d[1] == r.d[1] && l.d[2] == r.d[2];
}

TEST_CASE("user types" * doctest::test_suite("rlp"))
{
  // User types should be converted to ByteString before being passed to RLP,
  // and will only be decoded as far as ByteStrings

  UserType s{42, '!', true, {11, 1001, 100001}};

  uint8_t* start = (uint8_t*)&s;
  rlp::ByteString bs(start, start + sizeof(s));

  const auto original = std::make_tuple(
    "Other data"s,
    std::make_tuple("Awkward"s, std::make_tuple("Data"s)),
    bs,
    "And something afterwards"s);

  const rlp::ByteString encoded = rlp::encode(
    std::get<0>(original),
    std::get<1>(original),
    std::get<2>(original),
    std::get<3>(original));

  // Decode side currently needs to structure of entire contents.
  // TODO: When decoding to a ByteString, we can ignore the contents
  const auto tup = rlp::decode_single<decltype(original)>(encoded);

  const auto target = std::get<2>(tup);
  const UserType* result = (const UserType*)target.data();

  REQUIRE(s == *result);
}

TEST_CASE("transaction" * doctest::test_suite("rlp"))
{
  using namespace intx;
  uint256_t nonce = 0x5;
  uint256_t gas_price = 0x09184e72a000_u256;
  uint256_t gas_limit = 0x30000_u256;
  uint256_t to = 0xab2fcCB0c5F0499278801CE41F4bcCCA39676f2D_u256;
  uint256_t value = 0x0;
  rlp::ByteString data = {};

  uint256_t v = 0x1c;
  uint256_t r = 0x0;
  uint256_t s = 0x0;

  const auto tx_rlp =
    rlp::encode(nonce, gas_price, gas_limit, to, value, data, v, r, s);

  // Expected result produced from web3.js
  const auto expected = to_bytes(
    "0xe6058609184e72a0008303000094ab2fccb0c5f0499278801ce41f4bccca39676f2d8080"
    "1c8080");

  REQUIRE(tx_rlp == expected);
}
