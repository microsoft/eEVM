// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../evm/simpleglobalstate.h"
#include "../include/disassembler.h"
#include "../include/opcode.h"
#include "../include/processor.h"
#include "../include/rlp.h"
#include "../include/util.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <vector>

using namespace std;
using namespace evm;

TEST_CASE("util" * doctest::test_suite("util"))
{
  SUBCASE("to_bytes")
  {
    REQUIRE(to_bytes("") == vector<uint8_t>{});
    REQUIRE(to_bytes("0x") == vector<uint8_t>{});
    REQUIRE(to_bytes("0x0") == vector<uint8_t>{0x0});
    REQUIRE(to_bytes("0x00") == vector<uint8_t>{0x0});
    REQUIRE(to_bytes("0x000") == vector<uint8_t>{0x0, 0x0});
    REQUIRE(to_bytes("0x0000") == vector<uint8_t>{0x0, 0x0});
    REQUIRE(to_bytes("0xa") == vector<uint8_t>{0x0a});
    REQUIRE(to_bytes("0xab") == vector<uint8_t>{0xab});
    REQUIRE(to_bytes("0xabc") == vector<uint8_t>{0xa, 0xbc});
    REQUIRE(to_bytes("0xabcd") == vector<uint8_t>{0xab, 0xcd});
  }
}

TEST_CASE("byteExport" * doctest::test_suite("primitive"))
{
  std::array<uint8_t, 32> raw;

  SUBCASE("empty")
  {
    uint256_t n = 0x0;
    to_big_endian(n, raw.begin());
    for (size_t i = 0; i < 32; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.begin(), raw.end());
    REQUIRE(m == n);
  }

  SUBCASE("0xf")
  {
    uint256_t n = 0xf;
    to_big_endian(n, raw.begin());
    REQUIRE(raw[31] == 0xf);
    for (size_t i = 0; i < 31; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.begin(), raw.end());
    REQUIRE(m == n);
  }

  SUBCASE("0xff")
  {
    uint256_t n = 0xff;
    to_big_endian(n, raw.begin());
    REQUIRE(raw[31] == 0xff);
    for (size_t i = 0; i < 31; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.begin(), raw.end());
    REQUIRE(m == n);
  }

  SUBCASE("0xfff")
  {
    uint256_t n = 0xfff;
    to_big_endian(n, raw.begin());
    REQUIRE(raw[31] == 0xff);
    REQUIRE(raw[30] == 0xf);
    for (size_t i = 0; i < 30; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.begin(), raw.end());
    REQUIRE(m == n);
  }

  SUBCASE("0xab0cd01002340560000078")
  {
    uint256_t n = from_hex_str("0xab0cd01002340560000078");
    to_big_endian(n, raw.begin());
    REQUIRE(raw[31] == 0x78);
    REQUIRE(raw[30] == 0x00);
    REQUIRE(raw[29] == 0x00);
    REQUIRE(raw[28] == 0x60);
    REQUIRE(raw[27] == 0x05);
    REQUIRE(raw[26] == 0x34);
    REQUIRE(raw[25] == 0x02);
    REQUIRE(raw[24] == 0x10);
    REQUIRE(raw[23] == 0xd0);
    REQUIRE(raw[22] == 0x0c);
    REQUIRE(raw[21] == 0xab);
    for (size_t i = 0; i < 21; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.begin(), raw.end());
    REQUIRE(m == n);
  }

  SUBCASE("fullsize")
  {
    uint256_t n = from_hex_str(
      "0xa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf");
    to_big_endian(n, raw.begin());
    for (size_t i = 0; i < 32; ++i)
    {
      REQUIRE(raw[i] == 0xa0 + i);
    }

    uint256_t m = from_big_endian(raw.begin(), raw.end());
    REQUIRE(m == n);
  }
}

template <typename A, typename B>
auto PRINT_CHECK(A&& a, B&& b)
{
  if (a != b)
  {
    std::cout << "FAILED: " << std::endl;
    for (auto i : a)
    {
      std::cout << " " << std::hex << (int)i;
    }
    std::cout << std::endl;

    for (auto i : b)
    {
      std::cout << " " << std::hex << (int)i;
    }
    std::cout << std::endl;
  }
  else
  {
    std::cout << "SUCCESS" << std::endl;
  }
}

TEST_CASE("rlp" * doctest::test_suite("rlp"))
{
  SUBCASE("encode")
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
      rlp::encode(rlp::ByteString{0x0, 0x0}) ==
      rlp::ByteString{0x82, 0x0, 0x0});

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

  SUBCASE("decode")
  {
    // Unimplemented
  }
}

TEST_CASE("addressGeneration" * doctest::test_suite("rlp"))
{
  Address sender = from_hex_str("0x6ac7ea33f8831ea9dcc53393aaa88b25a785dbf0");

  Address nonce0 = generate_address(sender, 0u);
  CHECK(nonce0 == from_hex_str("0xcd234a471b72ba2f1ccf0a70fcaba648a5eecd8d"));

  Address nonce1 = generate_address(sender, 1u);
  CHECK(nonce1 == from_hex_str("0x343c43a37d37dff08ae8c4a11544c718abb4fcf8"));

  Address nonce2 = generate_address(sender, 2u);
  CHECK(nonce2 == from_hex_str("0xf778b86fa74e846c4f0a1fbd1335fe81c00a0c91"));

  Address nonce3 = generate_address(sender, 3u);
  CHECK(nonce3 == from_hex_str("0xfffd933a0bc612844eaf0c6fe3e5b8e9b6c1d19c"));
}

TEST_CASE("vmExecution" * doctest::test_suite("vm"))
{
  // harness.cpp runs more thorough tests from standard test cases. This is
  // the simplest possible test of the API, independent of json parsing/test
  // formats

  SimpleGlobalState gs;
  NullLogHandler ignore;
  Address from(0x100);
  Address to(0x101);
  Transaction tx(from, ignore);

  Processor p(gs);

  SUBCASE("nop")
  {
    Trace tr;
    const auto e = p.run(tx, from, gs.get(to), {}, 0, &tr);

    CHECK(e.er == ExitReason::halted);
    CHECK(e.output.empty());
    CHECK(tr.events.empty());
  }

  SUBCASE("add")
  {
    Trace tr;

    constexpr uint8_t a = 0xed;
    constexpr uint8_t b = 0xfe;
    constexpr uint8_t mdest = 0x0;
    constexpr uint8_t rsize = 0x20;

    const std::vector<uint8_t> code = {Opcode::PUSH1,
                                       a,
                                       Opcode::PUSH1,
                                       b,
                                       Opcode::ADD,
                                       Opcode::PUSH1,
                                       mdest,
                                       Opcode::MSTORE,
                                       Opcode::PUSH1,
                                       rsize,
                                       Opcode::PUSH1,
                                       mdest,
                                       Opcode::RETURN};

    gs.create(to, {}, code);

    const auto e = p.run(tx, from, gs.get(to), {}, 0, &tr);

    CHECK(e.er == ExitReason::returned);
    CHECK(e.output.size() == rsize);

    const uint256_t result = from_big_endian(e.output.begin(), e.output.end());
    CHECK(result == a + b);

    // Confirm each opcode produced a TraceEvent, in order
    auto it = code.begin();
    for (const auto& event : tr.events)
    {
      it = std::find(it, code.end(), event.op);
      CHECK(it != code.end());
    }
    CHECK(std::next(it) == code.end());
  }
}
