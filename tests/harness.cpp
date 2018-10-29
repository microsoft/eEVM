// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../evm/rlp.h"
#include "../evm/simpleglobalstate.h"
#include "../evm/simplestorage.h"
#include "../include/disassembler.h"
#include "../include/opcode.h"
#include "../include/processor.h"
#include "../include/util.h"

#include <doctest/doctest.h>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;
using namespace evm;
using namespace nlohmann;

pair<Account, SimpleStorage> parseAccount(json::const_iterator& it)
{
  auto j = it.value();
  return {{from_hex_str(it.key()),
           to_uint64(j["nonce"]),
           j["balance"].get<uint256_t>(),
           to_bytes(j["code"])},
          j["storage"]};
}

void run_test_case(
  const string& fileName,
  const set<string>& skip,
  const bool checkLogs,
  const bool disasm = true)
{
  auto test_dir = getenv("TEST_DIR");
  REQUIRE(test_dir != nullptr);
  string testPath = fileName;
  if (test_dir)
  {
    testPath = string(test_dir) + "/" + fileName;
  }

#ifdef RECORD_TRACE
  const string delim(15, '-');
  cout << "Test file " << testPath << "\n" << delim << endl;
#endif

  ifstream fs(testPath);
  {
    INFO("Looking for " << testPath);
    REQUIRE(fs.good());
  }

  const auto cases = json::parse(fs);
  int i = 0;
  for (decltype(auto) it = cases.cbegin(); it != cases.cend(); it++, i++)
  {
    if (skip.find(it.key()) != skip.end())
      continue;

    auto subcase = it.key();
    INFO("SUBCASE: " << subcase);
    {
      const auto& j = it.value();
      Address callee = j["exec"]["address"];
      Address caller = j["exec"]["caller"];
      const auto c = to_bytes(j["exec"]["code"]);
      const auto inp = to_bytes(j["exec"]["data"]);
      uint64_t value = to_uint64(j["exec"]["value"]);

#ifdef RECORD_TRACE
      cout << "Case " << it.key() << " (#" << std::dec << i << ")" << endl;
      if (disasm)
      {
        try
        {
          cout << "Disassembly:\n" << Disassembler::dis(c);
        }
        catch (exception)
        {
          cout << "Failed to disassemble.\n";
          continue;
        }
        cout << delim << endl;
      }
#endif

      Block b = j["env"];
      SimpleGlobalState gs(b);
      NullLogHandler ignore;
      Transaction tx(
        j["exec"]["origin"],
        ignore,
        to_uint64(j["exec"]["value"]),
        to_uint64(j["exec"]["gasPrice"]),
        to_uint64(j["exec"]["gas"]));

      // parse accounts
      for (decltype(auto) it = j["pre"].cbegin(); it != j["pre"].cend(); it++)
        gs.insert(parseAccount(it));

      CHECK(gs.exists(callee));

      Processor p(gs);
      Trace* tr(nullptr);

#ifdef RECORD_TRACE
      tr = new Trace();
#endif

      const auto e = p.run(tx, caller, gs.get(callee), inp, value, tr);

#ifdef RECORD_TRACE
      cout << "Trace:\n" << *tr << delim << endl;
      delete tr;
#endif

      // was the run supposed to fail?
      const auto post = j.find("post");
      if (post == j.end())
      {
        CHECK((e.er == ExitReason::threw || e.er == ExitReason::halted));
        continue;
      }

      uint64_t postAccounts = 0;
      for (auto it = post.value().cbegin(); it != post.value().cend(); it++)
      {
        const auto expected = parseAccount(it);
        const auto actual = gs.get(expected.first.address);
        CHECK(actual.acc == expected.first);

        decltype(auto) st = dynamic_cast<SimpleStorage&>(actual.st);
        CHECK(st == expected.second);
      }

      // the != 0 check exists because there exists failure scenarios where post
      // is not mentioned when an exception is raised but that does not mean
      // systemState is lost
      if (postAccounts != 0)
        CHECK(postAccounts == gs.num_accounts());

      // does the test case specify output?
      if (j.find("out") != j.end())
        CHECK(e.output == to_bytes(j.at("out")));

      // TODO: Calculate hash of RLP of produced log entries, to compare against
      // the value specified in the test case
    }
  }
}

TEST_CASE("vmBitwiseLogicOperationTest" * doctest::test_suite("logic"))
{
  run_test_case("vmBitwiseLogicOperationTest.json", {}, true);
}

TEST_CASE("vmEnvironmentalInfoTest" * doctest::test_suite("logic"))
{
  auto skip = set<string>{
    // memory index > 2^64
    "calldatacopy_DataIndexTooHigh",
    "calldatacopy_DataIndexTooHigh2",
    "calldatacopy_DataIndexTooHigh2_return",
    "calldatacopy_DataIndexTooHigh_return",
    "calldataloadSizeTooHigh",
    "calldataload_BigOffset",
    "codecopy_DataIndexTooHigh",
  };

  run_test_case("vmEnvironmentalInfoTest.json", skip, true);
}

TEST_CASE("vmTests" * doctest::test_suite("logic"))
{
  auto skip = set<string>{};

  run_test_case("vmTests.json", skip, true);
}

// These pass, but are kept hidden by default because they're slow
TEST_CASE(
  "vmPerformanceTest" * doctest::test_suite(".performance") * doctest::skip())
{
  auto skip = set<string>{// Missing post
                          "ackermann33",

                          // Empty output
                          "loop-exp-16b-100k",
                          "loop-exp-32b-100k",

                          // Incorrect output
                          "loop-exp-8b-100k"};

  run_test_case("vmPerformanceTest.json", skip, false);
}

TEST_CASE("vmSystemOperationsTest" * doctest::test_suite("logic"))
{
  auto skip = set<string>{};

  run_test_case("vmSystemOperationsTest.json", skip, false);
}

// Passes, but slow, so disabled by default
TEST_CASE(
  "vmInputLimitsLight" * doctest::test_suite(".input") * doctest::skip())
{
  auto skip = set<string>{
    // copies code to offset > 2^64
    "012fd315e355bad0d1bdce9a44863f3c909bfdf9909779c431c9e0fdf9ae339f",
    "01923ee9def56e347452847fd9be4577f8b663097620664ba24317f67a73122a",
    "01a5cf9db140969b2a2410361164fc41c64c070805b82116d217240d4e304f6f",
    "01d740c2964a008fc6998e2d0cf2df984c8451369737426ad5640a129be6c5dd",

    // logs data with size > 2^64
    "01854150aba4ddc54c4ac0a61e21b838cb53017d0fa83faf8e146233337cb1fb",
    "0322751b60db071ea7c6885f6f3eaf0b83af83856ba5a72e3a87404cc171fac3"};

  run_test_case("vmInputLimitsLight.json", skip, true);
}

TEST_CASE("vmArithmeticTest" * doctest::test_suite("logic"))
{
  auto skip = set<string>{
    // exponent wider than 64 bits
    "exp1",

    // exponents too large ~(256 ^ (256 ^ n))
    "expPowerOf256Of256_4",
    "expPowerOf256Of256_5",
    "expPowerOf256Of256_6",
    "expPowerOf256Of256_7",
    "expPowerOf256Of256_8",
    "expPowerOf256Of256_9",
    "expPowerOf256Of256_10",
    "expPowerOf256Of256_11",
    "expPowerOf256Of256_12",
    "expPowerOf256Of256_13",
    "expPowerOf256Of256_14",
    "expPowerOf256Of256_15",
    "expPowerOf256Of256_16",
    "expPowerOf256Of256_17",
    "expPowerOf256Of256_18",
    "expPowerOf256Of256_19",
    "expPowerOf256Of256_20",
    "expPowerOf256Of256_21",
    "expPowerOf256Of256_22",
    "expPowerOf256Of256_23",
    "expPowerOf256Of256_24",
    "expPowerOf256Of256_25",
    "expPowerOf256Of256_26",
    "expPowerOf256Of256_27",
    "expPowerOf256Of256_28",
    "expPowerOf256Of256_29",
    "expPowerOf256Of256_30",
    "expPowerOf256Of256_31",
    "expPowerOf256Of256_32",
    "expPowerOf256Of256_33",

    // filler and code say store[2] = result, but it's not in the expected post
    "expXY"};

  run_test_case("vmArithmeticTest.json", skip, true);
}

// Passes, but not testing anything interesting because we're currently ignoring
// the logs
TEST_CASE("vmLogTest" * doctest::test_suite("env"))
{
  auto skip = set<string>{
    // mem access > 2^64
    "log0_logMemStartTooHigh",
    "log0_logMemsizeTooHigh",
    "log1_logMemStartTooHigh",
    "log1_logMemsizeTooHigh",
    "log2_logMemStartTooHigh",
    "log2_logMemsizeTooHigh",
    "log3_logMemStartTooHigh",
    "log3_logMemsizeTooHigh",
    "log4_logMemStartTooHigh",
    "log4_logMemsizeTooHigh",
  };

  run_test_case("vmLogTest.json", skip, true);
}

TEST_CASE("vmPushDupSwapTest" * doctest::test_suite("logic"))
{
  auto skip = set<string>{
    // clearly valid program, but no post defined
    "push33",
  };

  run_test_case("vmPushDupSwapTest.json", skip, true);
}

TEST_CASE("vmIOandFlowOperationsTest" * doctest::test_suite("env"))
{
  auto skip = set<string>{
    // infinite loop, expects to run out of gas
    "BlockNumberDynamicJump0_foreverOutOfGas",
    "DynamicJump0_foreverOutOfGas",
    "JDfromStorageDynamicJump0_foreverOutOfGas",
    "jump0_foreverOutOfGas",

    // missing gas
    "gas0",
    "gas1",

    // seem like correct program, why no post here?
    "return1",
  };

  run_test_case("vmIOandFlowOperationsTest.json", skip, true);
}

TEST_CASE("vmBlockInfoTest" * doctest::test_suite("env"))
{
  auto skip = set<string>{"gaslimit"};

  run_test_case("vmBlockInfoTest.json", skip, true);
}

TEST_CASE("vmRandomTest" * doctest::test_suite("rand"))
{
  auto skip = set<string>{};

  run_test_case("vmRandomTest.json", skip, true);
}

TEST_CASE("vmSha3Test" * doctest::test_suite("sha"))
{
  auto skip = set<string>{};

  run_test_case("vmSha3Test.json", skip, true);
}
