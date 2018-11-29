#include "evm/simpleglobalstate.h"
#include "include/opcode.h"
#include "include/processor.h"

#include <iostream>

int usage(const char* bin_name)
{
  std::cout << "Usage: " << bin_name << " [-v] hex_a hex_b" << std::endl;
  std::cout << "Prints sum of arguments (hex string representation of 256-bit "
               "uints)"
            << std::endl;
  return 1;
}

void push_uint256(std::vector<uint8_t>& code, const uint256_t& n)
{
  // Append opcode
  code.push_back(evm::Opcode::PUSH32);

  // Resize code array
  const size_t pre_size = code.size();
  code.resize(pre_size + 32);

  // Serialize number into code array
  to_big_endian(n, code.begin() + pre_size);
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
  code.push_back(evm::Opcode::ADD);

  // Store result
  code.push_back(evm::Opcode::PUSH1);
  code.push_back(mdest);
  code.push_back(evm::Opcode::MSTORE);

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
  const uint256_t arg_a = from_hex_str(argv[first_arg]);
  const uint256_t arg_b = from_hex_str(argv[first_arg + 1]);

  if (verbose)
  {
    std::cout << "Calculating " << to_hex_str(arg_a) << " + "
              << to_hex_str(arg_b) << std::endl;
  }

  // Invent a random address to use as sender
  std::vector<uint8_t> raw_address(160);
  std::generate(
    raw_address.begin(), raw_address.end(), []() { return rand(); });
  const evm::Address sender =
    from_big_endian(raw_address.begin(), raw_address.end());

  // Generate a target address for the summing contract (this COULD be random,
  // but here we use the scheme for Contract Creation specified in the Yellow
  // Paper)
  const evm::Address to = evm::generate_address(sender, 0);

  // Create summing bytecode
  const evm::Code code = create_a_plus_b_bytecode(arg_a, arg_b);

  // Construct global state
  evm::SimpleGlobalState gs;

  // Populate the global state with the constructed contract
  const evm::AccountState contract = gs.create(to, 0, code);

  if (verbose)
  {
    std::cout << "Address " << to_hex_str(to)
              << " contains the following bytecode:" << std::endl;
    std::cout << evm::to_hex_string(contract.acc.code) << std::endl;
  }

  // Construct a transaction object
  evm::NullLogHandler ignore; //< Ignore any logs produced by this transaction
  evm::Transaction tx(sender, ignore);

  // Construct processor
  evm::Processor p(gs);

  if (verbose)
  {
    std::cout << "Executing a transaction from " << to_hex_str(sender) << " to "
              << to_hex_str(to) << std::endl;
  }

  // Run transaction
  evm::Trace tr;
  const evm::ExecResult e = p.run(
    tx,
    sender,
    contract,
    {}, //< No input - the arguments are hard-coded in the contract
    0, //< No gas value
    &tr //< Record execution trace
  );

  if (e.er != evm::ExitReason::returned)
  {
    std::cout << "Unexpected return code" << std::endl;
    return 2;
  }

  if (verbose)
  {
    std::cout << "Execution completed, and returned a result of "
              << e.output.size() << " bytes" << std::endl;
  }

  const uint256_t result = from_big_endian(e.output.begin(), e.output.end());

  std::cout << to_hex_str(arg_a);
  std::cout << " + ";
  std::cout << to_hex_str(arg_b);
  std::cout << " = ";
  std::cout << to_hex_str(result);
  std::cout << std::endl;

  if (verbose)
  {
    std::cout << "Done" << std::endl;
  }

  return 0;
}
