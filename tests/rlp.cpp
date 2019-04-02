#include "../include/rlp.h"

#include <doctest/doctest.h>

using namespace evm;

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

  const auto long_and_nested = std::make_tuple(
    std::make_tuple("Hello world", "Saluton Mondo"),
    std::make_tuple(
      std::make_tuple(
        std::make_tuple(1),
        std::make_tuple(2, 3),
        std::make_tuple(std::make_tuple(4))),
      66000),
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua");
  const auto expected = rlp::to_byte_string(
    "\xf8\xa5\xda\x8bHello world\x8dSaluton "
    "Mondo\xcd\xc8\xc1\x01\xc2\x02\x03\xc2\xc1\x04\x83\x01\x01\xd0\xb8zLorem "
    "ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua");
  CHECK(rlp::encode(long_and_nested) == expected);
}

TEST_CASE("uint256_t" * doctest::test_suite("rlp"))
{
  const uint256_t a = 1024;
  CHECK(rlp::encode(a) == rlp::ByteString{0x82, 0x04, 0x00});

  using namespace boost::multiprecision::literals;
  const uint256_t b = 0x1234567890abcdefdeadbeefcafef00dbaaaad_cppui;
  CHECK(rlp::encode(b) == rlp::ByteString{0x93, 0x12, 0x34, 0x56, 0x78,
                                          0x90, 0xab, 0xcd, 0xef, 0xde,
                                          0xad, 0xbe, 0xef, 0xca, 0xfe,
                                          0xf0, 0x0d, 0xba, 0xaa, 0xad});
}

TEST_CASE("decode" * doctest::test_suite("rlp"))
{
  // Unimplemented
}