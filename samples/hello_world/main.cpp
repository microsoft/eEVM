// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "evm/simpleglobalstate.h"
#include "include/opcode.h"
#include "include/processor.h"

#include <iostream>

std::vector<uint8_t> create_hello_world_bytecode()
{
  std::string hw("Hello world!");

  std::vector<uint8_t> code;
  constexpr uint8_t mdest = 0x0;
  const uint8_t rsize = hw.size() + 1;

  // Store each byte in evm memory
  uint8_t mcurrent = mdest;
  for (const char& c : hw)
  {
    code.push_back(evm::Opcode::PUSH1);
    code.push_back(c);
    code.push_back(evm::Opcode::PUSH1);
    code.push_back(mcurrent++);
    code.push_back(evm::Opcode::MSTORE8);
  }

  // Return
  code.push_back(evm::Opcode::PUSH1);
  code.push_back(rsize);
  code.push_back(evm::Opcode::PUSH1);
  code.push_back(mdest);
  code.push_back(evm::Opcode::RETURN);

  return code;
}

int main(int argc, char** argv)
{
  // Create random addresses for sender and contract
  std::vector<uint8_t> raw_address(160);
  std::generate(
    raw_address.begin(), raw_address.end(), []() { return rand(); });
  const evm::Address sender =
    from_big_endian(raw_address.begin(), raw_address.end());

  std::generate(
    raw_address.begin(), raw_address.end(), []() { return rand(); });
  const evm::Address to =
    from_big_endian(raw_address.begin(), raw_address.end());

  // Create global state
  evm::SimpleGlobalState gs;

  // Create code
  const evm::Code code = create_hello_world_bytecode();

  // Deploy contract to global state
  const evm::AccountState contract = gs.create(to, 0, code);

  // Create transaction
  evm::NullLogHandler ignore;
  evm::Transaction tx(sender, ignore);

  // Create processor
  evm::Processor p(gs);

  // Execute code. All execution is associated with a transaction. This
  // transaction is called by sender, executing the code in contract, with empty
  // input (and no trace collection)
  const evm::ExecResult e = p.run(tx, sender, contract, {}, 0, nullptr);

  // Check the response
  if (e.er != evm::ExitReason::returned)
  {
    std::cout << "Unexpected return code" << std::endl;
    return 2;
  }

  // Create string from response data, and print it
  const std::string response(reinterpret_cast<const char*>(e.output.data()));
  std::cout << response << std::endl;

  return 0;
}
