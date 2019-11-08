// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/bigint.h"
#include "eEVM/disassembler.h"
#include "eEVM/opcode.h"
#include "eEVM/processor.h"
#include "eEVM/simple/simpleaccount.h"
#include "eEVM/simple/simpleglobalstate.h"
#include "eEVM/util.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

using namespace std;
using namespace eevm;

TEST_CASE(
  "from_json/to_json are mutually inverse" * doctest::test_suite("json"))
{
  constexpr auto env_var = "TEST_DIR";
  auto test_dir = getenv(env_var);
  if (!test_dir)
  {
    throw std::logic_error(
      "Must set path to test cases in " + std::string(env_var) +
      " environment variable");
  }

  SUBCASE("Using default SimpleGlobalState objects")
  {
    SimpleGlobalState s0;
    nlohmann::json j = s0;
    SimpleGlobalState s1 = j.get<SimpleGlobalState>();
    REQUIRE(s1 == s0);
  }

  SUBCASE("Using fully defined SimpleGlobalState JSON object")
  {
    // simpleGlobalStateFull.json is a generated file, so we can safely
    // check for an equality on string-by-string basis
    auto test_path = string(test_dir) + "/simpleGlobalStateFull.json";
    const auto j = nlohmann::json::parse(std::ifstream(test_path));
    SimpleGlobalState s0 = j.get<SimpleGlobalState>();
    nlohmann::json j2 = s0;
    REQUIRE(j == j2);
  }

  SUBCASE("Using default SimpleAccount objects")
  {
    SimpleAccount a1;
    nlohmann::json j = a1;
    SimpleAccount a2 = j.get<SimpleAccount>();
    REQUIRE(a1 == a2);
  }

  SUBCASE("Using non-default values for SimpleAccount")
  {
    SimpleAccount a1(
      to_uint256("0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6"),
      5678,
      {0x00, 0x01, 0x10, 0x11},
      0x66);
    nlohmann::json j = a1;
    SimpleAccount a2 = j;
    REQUIRE(a1 == a2);
  }

  SUBCASE("Using partially defined JSON as a source for SimpleAccount")
  {
    auto test_path = string(test_dir) + "/vmTests.json";
    const auto j = nlohmann::json::parse(std::ifstream(test_path));
    const auto rec = *j["suicide"]["pre"].begin();
    SimpleAccount a1 = rec.get<SimpleAccount>();
    nlohmann::json j2 = a1;
    if (rec.find("balance") != rec.end())
      CHECK(a1.get_balance() == to_uint256(j2["balance"]));
    if (rec.find("code") != rec.end())
      CHECK(a1.get_code() == to_bytes(j2["code"]));
    if (rec.find("nonce") != rec.end())
      CHECK(a1.get_nonce() == to_uint64(j2["nonce"]));
    if (rec.find("address") != rec.end())
      CHECK(a1.get_address() == to_uint256(j2["address"]));
  }

  SUBCASE("Using fully defined JSON as a source for SimpleAccount")
  {
    auto test_path = string(test_dir) + "/accountFull.json";
    const auto j = nlohmann::json::parse(std::ifstream(test_path));
    SimpleAccount a1 = j.get<SimpleAccount>();
    nlohmann::json j2 = a1;
    REQUIRE(j == j2);
  }
}

TEST_CASE("util" * doctest::test_suite("util"))
{
  SUBCASE("to_hex_string")
  {
    using namespace intx;
    REQUIRE(to_hex_string(0) == "0x0");
    REQUIRE(to_hex_string(1) == "0x1");
    REQUIRE(to_hex_string(0xa) == "0xa");
    REQUIRE(to_hex_string(0xff) == "0xff");
    REQUIRE(
      to_hex_string(
        0x1234567890abcdef1a1a2b2b3c3c4d4d5e5e6f6f0011223344556677889900aa_u256) ==
      "0x1234567890abcdef1a1a2b2b3c3c4d4d5e5e6f6f0011223344556677889900aa");

    REQUIRE(to_hex_string_fixed(0, 4) == "0x0000");
    REQUIRE(to_hex_string_fixed(1, 4) == "0x0001");
    REQUIRE(
      to_hex_string_fixed(0xa, 64) ==
      "0x000000000000000000000000000000000000000000000000000000000000000a");
    REQUIRE(
      to_hex_string_fixed(0xff, 64) ==
      "0x00000000000000000000000000000000000000000000000000000000000000ff");
    REQUIRE(
      to_hex_string_fixed(
        0x1234567890abcdef1a1a2b2b3c3c4d4d5e5e6f6f0011223344556677889900aa_u256,
        64) ==
      "0x1234567890abcdef1a1a2b2b3c3c4d4d5e5e6f6f0011223344556677889900aa");
  }

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

  SUBCASE("keccak_256")
  {
    constexpr auto empty_hash =
      "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470";
    constexpr auto hello_world_hash =
      "0xed6c11b0b5b808960df26f5bfc471d04c1995b0ffd2055925ad1be28d6baadfd";
    constexpr auto ello_world_hash =
      "0x06f5a9ffe20e0fda47399119d5f89e6ea5aa7442fdbc973c365ef4ad993cde12";
    constexpr auto world_hash =
      "0x8452c9b9140222b08593a26daa782707297be9f7b3e8281d7b4974769f19afd0";

    {
      INFO("std::string");

      const std::string empty;
      REQUIRE(to_hex_string(keccak_256(empty)) == empty_hash);
      REQUIRE(to_hex_string(keccak_256_skip(5, empty)) == empty_hash);

      const std::string s = "Hello world";
      REQUIRE(to_hex_string(keccak_256(s)) == hello_world_hash);
      REQUIRE(to_hex_string(keccak_256_skip(1, s)) == ello_world_hash);
      REQUIRE(to_hex_string(keccak_256_skip(6, s)) == world_hash);
    }

    {
      INFO("std::vector");

      const std::vector<uint8_t> empty;
      REQUIRE(to_hex_string(keccak_256(empty)) == empty_hash);
      REQUIRE(to_hex_string(keccak_256_skip(5, empty)) == empty_hash);

      const std::vector<uint8_t> v{
        'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
      REQUIRE(to_hex_string(keccak_256(v)) == hello_world_hash);
      REQUIRE(to_hex_string(keccak_256_skip(1, v)) == ello_world_hash);
      REQUIRE(to_hex_string(keccak_256_skip(6, v)) == world_hash);
    }

    {
      INFO("std::array");

      const std::array<uint8_t, 0> empty;
      REQUIRE(to_hex_string(keccak_256(empty)) == empty_hash);
      REQUIRE(to_hex_string(keccak_256_skip(5, empty)) == empty_hash);

      const std::array<uint8_t, 11> a{
        'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
      REQUIRE(to_hex_string(keccak_256(a)) == hello_world_hash);
      REQUIRE(to_hex_string(keccak_256_skip(1, a)) == ello_world_hash);
      REQUIRE(to_hex_string(keccak_256_skip(6, a)) == world_hash);
    }
  }

  SUBCASE("to_checksum_address")
  {
    const Address t0 = to_uint256("0x5aaeb6053f3e94c9b9a09f33669435e7ef1beaed");
    REQUIRE(
      to_checksum_address(t0) == "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed");

    const Address t1 = to_uint256("0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359");
    REQUIRE(
      to_checksum_address(t1) == "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359");

    const Address t2 = to_uint256("0xDBF03B407C01E7CD3CBEA99509D93F8DDDC8C6FB");
    REQUIRE(
      to_checksum_address(t2) == "0xdbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB");

    const Address t3 = to_uint256("0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb");
    REQUIRE(
      to_checksum_address(t3) == "0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb");

    REQUIRE(is_checksum_address("0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed"));
    REQUIRE(is_checksum_address("0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359"));
    REQUIRE(is_checksum_address("0xdbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB"));
    REQUIRE(is_checksum_address("0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb"));
  }
}

TEST_CASE("byteExport" * doctest::test_suite("primitive"))
{
  std::array<uint8_t, 32> raw;

  SUBCASE("empty")
  {
    uint256_t n = 0x0;
    to_big_endian(n, raw.data());
    for (size_t i = 0; i < 32; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.data(), raw.size());
    REQUIRE(m == n);
  }

  SUBCASE("0xf")
  {
    uint256_t n = 0xf;
    to_big_endian(n, raw.data());
    REQUIRE(raw[31] == 0xf);
    for (size_t i = 0; i < 31; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.data(), raw.size());
    REQUIRE(m == n);
  }

  SUBCASE("0xff")
  {
    uint256_t n = 0xff;
    to_big_endian(n, raw.data());
    REQUIRE(raw[31] == 0xff);
    for (size_t i = 0; i < 31; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.data(), raw.size());
    REQUIRE(m == n);
  }

  SUBCASE("0xfff")
  {
    uint256_t n = 0xfff;
    to_big_endian(n, raw.data());
    REQUIRE(raw[31] == 0xff);
    REQUIRE(raw[30] == 0xf);
    for (size_t i = 0; i < 30; ++i)
    {
      REQUIRE(raw[i] == 0);
    }

    uint256_t m = from_big_endian(raw.data(), raw.size());
    REQUIRE(m == n);
  }

  SUBCASE("0xab0cd01002340560000078")
  {
    uint256_t n = to_uint256("0xab0cd01002340560000078");
    to_big_endian(n, raw.data());
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

    uint256_t m = from_big_endian(raw.data(), raw.size());
    REQUIRE(m == n);
  }

  SUBCASE("fullsize")
  {
    uint256_t n = to_uint256(
      "0xa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf");
    to_big_endian(n, raw.data());
    for (size_t i = 0; i < 32; ++i)
    {
      REQUIRE(raw[i] == 0xa0 + i);
    }

    uint256_t m = from_big_endian(raw.data(), raw.size());
    REQUIRE(m == n);
  }
}

TEST_CASE("addressGeneration" * doctest::test_suite("rlp"))
{
  Address sender = to_uint256("0x6ac7ea33f8831ea9dcc53393aaa88b25a785dbf0");

  Address nonce0 = generate_address(sender, 0u);
  CHECK(nonce0 == to_uint256("0xcd234a471b72ba2f1ccf0a70fcaba648a5eecd8d"));

  Address nonce1 = generate_address(sender, 1u);
  CHECK(nonce1 == to_uint256("0x343c43a37d37dff08ae8c4a11544c718abb4fcf8"));

  Address nonce2 = generate_address(sender, 2u);
  CHECK(nonce2 == to_uint256("0xf778b86fa74e846c4f0a1fbd1335fe81c00a0c91"));

  Address nonce3 = generate_address(sender, 3u);
  CHECK(nonce3 == to_uint256("0xfffd933a0bc612844eaf0c6fe3e5b8e9b6c1d19c"));
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

    const uint256_t result = from_big_endian(e.output.data(), e.output.size());
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
