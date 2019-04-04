#include "../include/rlp.h"

#include <doctest/doctest.h>

using namespace evm;
using namespace std::string_literals;

const auto large_input_decoded = std::make_tuple(
  std::make_tuple("Hello world"s, "Saluton Mondo"s),
  std::make_tuple(
    std::make_tuple(
      std::make_tuple(1),
      std::make_tuple(2, 3),
      std::make_tuple(std::make_tuple(4))),
    66000),
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
  "tempor incididunt ut labore et dolore magna aliqua"s);
const auto large_input_encoded = rlp::to_byte_string(
  "\xf8\xa5\xda\x8bHello world\x8dSaluton "
  "Mondo\xcd\xc8\xc1\x01\xc2\x02\x03\xc2\xc1\x04\x83\x01\x01\xd0\xb8zLorem "
  "ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
  "tempor incididunt ut labore et dolore magna aliqua");

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

TEST_CASE("uint256_t" * doctest::test_suite("rlp"))
{
  uint256_t small_decoded = 1024;
  auto small_encoded = rlp::ByteString{0x82, 0x04, 0x00};

  using namespace boost::multiprecision::literals;
  uint256_t large_decoded = 0x1234567890abcdefdeadbeefcafef00dbaaaad_cppui;
  auto large_encoded =
    rlp::ByteString{0x93, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xde,
                    0xad, 0xbe, 0xef, 0xca, 0xfe, 0xf0, 0x0d, 0xba, 0xaa, 0xad};

  SUBCASE("encode")
  {
    CHECK(rlp::encode(small_decoded) == small_encoded);
    CHECK(rlp::encode(large_decoded) == large_encoded);
  }

  SUBCASE("decode")
  {
    CHECK(
      rlp::decode_single<decltype(small_decoded)>(small_encoded) ==
      small_decoded);
    CHECK(
      rlp::decode_single<decltype(large_decoded)>(large_encoded) ==
      large_decoded);
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