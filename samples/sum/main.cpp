// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/opcode.h"
#include "eEVM/processor.h"
#include "eEVM/simple/simpleglobalstate.h"
#include "eEVM/util.h"

#include <fmt/format_header_only.h>
#include <iostream>

int usage(const char* bin_name)
{
  std::cout << fmt::format("Usage: {} [-v] hex_a hex_b", bin_name) << std::endl;
  std::cout
    << "Prints sum of arguments (hex string representation of 256-bit uints)"
    << std::endl;
  return 1;
}

void push_uint256(std::vector<uint8_t>& code, const uint256_t& n)
{
  // Append opcode
  code.push_back(eevm::Opcode::PUSH32);

  // Resize code array
  const size_t pre_size = code.size();
  code.resize(pre_size + 32);

  // Serialize number into code array
  eevm::to_big_endian(n, code.data() + pre_size);
}

std::vector<uint8_t> create_a_plus_b_bytecode(
  const uint256_t& a, const uint256_t& b)
{
  std::vector<uint8_t> code;
  constexpr uint8_t mdest = 0x0; //< Memory start address for result
  constexpr uint8_t rsize = 0x20; //< Size of result

  // Push args and ADD
  push_uint256(code, a);
  push_uint256(code, b);
  code.push_back(eevm::Opcode::ADD);

  // Store result
  code.push_back(eevm::Opcode::PUSH1);
  code.push_back(mdest);
  code.push_back(eevm::Opcode::MSTORE);

  // Return
  code.push_back(eevm::Opcode::PUSH1);
  code.push_back(rsize);
  code.push_back(eevm::Opcode::PUSH1);
  code.push_back(mdest);
  code.push_back(eevm::Opcode::RETURN);

  return code;
}

int main(int argc, char** argv)
{
  // Validate args, read verbose option
  bool verbose = false;
  size_t first_arg = 1;
  if (argc < 3 || argc > 4)
  {
    return usage(argv[0]);
  }

  if (argc == 4)
  {
    if (strcmp(argv[1], "-v") != 0)
    {
      return usage(argv[0]);
    }

    verbose = true;
    first_arg = 2;
  }

  srand(time(nullptr));

  // Parse args
  const uint256_t arg_a = eevm::to_uint256(argv[first_arg]);
  const uint256_t arg_b = eevm::to_uint256(argv[first_arg + 1]);

  if (verbose)
  {
    std::cout << fmt::format(
                   "Calculating {} + {}",
                   eevm::to_lower_hex_string(arg_a),
                   eevm::to_lower_hex_string(arg_b))
              << std::endl;
  }

  // Invent a random address to use as sender
  std::vector<uint8_t> raw_address(20);
  std::generate(
    raw_address.begin(), raw_address.end(), []() { return rand(); });
  const eevm::Address sender =
    eevm::from_big_endian(raw_address.data(), raw_address.size());

  // Generate a target address for the summing contract (this COULD be random,
  // but here we use the scheme for Contract Creation specified in the Yellow
  // Paper)
  const eevm::Address to = eevm::generate_address(sender, 0);

  // Create summing bytecode
  const eevm::Code code = create_a_plus_b_bytecode(arg_a, arg_b);

  // Construct global state
  eevm::SimpleGlobalState gs;

  // Populate the global state with the constructed contract
  const eevm::AccountState contract = gs.create(to, 0, code);

  if (verbose)
  {
    std::cout << fmt::format(
                   "Address {} contains the following bytecode:\n {}",
                   eevm::to_checksum_address(to),
                   eevm::to_hex_string(contract.acc.get_code()))
              << std::endl;
  }

  // Construct a transaction object
  eevm::NullLogHandler ignore; //< Ignore any logs produced by this transaction
  eevm::Transaction tx(sender, ignore);

  // Construct processor
  eevm::Processor p(gs);

  if (verbose)
  {
    std::cout << fmt::format(
                   "Executing a transaction from {} to {}",
                   eevm::to_checksum_address(sender),
                   eevm::to_checksum_address(to))
              << std::endl;
  }

  // Run transaction
  eevm::Trace tr;
  const eevm::ExecResult e = p.run(
    tx,
    sender,
    contract,
    {}, //< No input - the arguments are hard-coded in the contract
    0, //< No gas value
    &tr //< Record execution trace
  );

  if (e.er != eevm::ExitReason::returned)
  {
    std::cout << fmt::format("Unexpected return code: {}", (size_t)e.er)
              << std::endl;
    return 2;
  }

  if (verbose)
  {
    std::cout << fmt::format(
                   "Execution completed, and returned a result of {} bytes",
                   e.output.size())
              << std::endl;
  }

  const uint256_t result =
    eevm::from_big_endian(e.output.data(), e.output.size());

  std::cout << fmt::format(
                 "{} + {} = {}",
                 eevm::to_lower_hex_string(arg_a),
                 eevm::to_lower_hex_string(arg_b),
                 eevm::to_lower_hex_string(result))
            << std::endl;

  return 0;
}
