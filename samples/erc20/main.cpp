// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/opcode.h"
#include "eEVM/processor.h"
#include "eEVM/simple/simpleglobalstate.h"

#include <cassert>
#include <fmt/format_header_only.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// Util typedefs and functions
//
using Addresses = std::vector<eevm::Address>;

struct Environment
{
  eevm::GlobalState& gs;
  const eevm::Address& owner_address;
  const nlohmann::json& contract_definition;
};

size_t rand_range(size_t exclusive_upper_bound)
{
  std::random_device rand_device;
  std::mt19937 generator(rand_device());
  std::uniform_int_distribution<size_t> dist(0, exclusive_upper_bound - 1);

  return dist(generator);
}

uint256_t get_random_uint256(size_t bytes = 32)
{
  std::vector<uint8_t> raw(bytes);
  std::generate(raw.begin(), raw.end(), []() { return rand(); });
  return eevm::from_big_endian(raw.data(), raw.size());
}

eevm::Address get_random_address()
{
  return get_random_uint256(20);
}
///////////////////////////////////////////////////////////////////////////////

// Run input as an EVM transaction, check the result and return the output
std::vector<uint8_t> run_and_check_result(
  Environment& env,
  const eevm::Address& from,
  const eevm::Address& to,
  const eevm::Code& input)
{
  // Ignore any logs produced by this transaction
  eevm::NullLogHandler ignore;
  eevm::Transaction tx(from, ignore);

  // Record a trace to aid debugging
  eevm::Trace tr;
  eevm::Processor p(env.gs);

  // Run the transaction
  const auto exec_result = p.run(tx, from, env.gs.get(to), input, 0u, &tr);

  if (exec_result.er != eevm::ExitReason::returned)
  {
    // Print the trace if nothing was returned
    std::cerr << fmt::format("Trace:\n{}", tr) << std::endl;
    if (exec_result.er == eevm::ExitReason::threw)
    {
      // Rethrow to highlight any exceptions raised in execution
      throw std::runtime_error(
        fmt::format("Execution threw an error: {}", exec_result.exmsg));
    }

    throw std::runtime_error("Deployment did not return");
  }

  return exec_result.output;
}

// Modify code to append ABI-encoding of arg, suitable for passing to contract
// execution
void append_argument(std::vector<uint8_t>& code, const uint256_t& arg)
{
  // To ABI encode a function call with a uint256_t (or Address) argument,
  // simply append the big-endian byte representation to the code (function
  // selector, or bin). ABI-encoding for more complicated types is more
  // complicated, so not shown in this sample.
  const auto pre_size = code.size();
  code.resize(pre_size + 32u);
  eevm::to_big_endian(arg, code.data() + pre_size);
}

// Deploy the ERC20 contract defined in env, with total_supply tokens. Return
// the address the contract was deployed to
eevm::Address deploy_erc20_contract(
  Environment& env, const uint256_t total_supply)
{
  // Generate the contract address
  const auto contract_address = eevm::generate_address(env.owner_address, 0u);

  // Get the binary constructor of the contract
  auto contract_constructor = eevm::to_bytes(env.contract_definition["bin"]);

  // The constructor takes a single argument (total_supply) - append it
  append_argument(contract_constructor, total_supply);

  // Set this constructor as the contract's code body
  auto contract = env.gs.create(contract_address, 0u, contract_constructor);

  // Run a transaction to initialise this account
  auto result =
    run_and_check_result(env, env.owner_address, contract_address, {});

  // Result of running the compiled constructor is the code that should be the
  // contract's body (constructor will also have setup contract's Storage)
  contract.acc.set_code(std::move(result));

  return contract.acc.get_address();
}

// Get the total token supply by calling totalSupply on the contract_address
uint256_t get_total_supply(
  Environment& env, const eevm::Address& contract_address)
{
  // Anyone can call totalSupply - prove this by asking from a randomly
  // generated address
  const auto caller = get_random_address();

  const auto function_call =
    eevm::to_bytes(env.contract_definition["hashes"]["totalSupply()"]);

  const auto output =
    run_and_check_result(env, caller, contract_address, function_call);

  return eevm::from_big_endian(output.data(), output.size());
}

// Get the current token balance of target_address by calling balanceOf on
// contract_address
uint256_t get_balance(
  Environment& env,
  const eevm::Address& contract_address,
  const eevm::Address& target_address)
{
  // Anyone can call balanceOf - prove this by asking from a randomly generated
  // address
  const auto caller = get_random_address();

  auto function_call =
    eevm::to_bytes(env.contract_definition["hashes"]["balanceOf(address)"]);

  append_argument(function_call, target_address);

  const auto output =
    run_and_check_result(env, caller, contract_address, function_call);

  return eevm::from_big_endian(output.data(), output.size());
}

// Transfer tokens from source_address to target_address by calling transfer on
// contract_address
bool transfer(
  Environment& env,
  const eevm::Address& contract_address,
  const eevm::Address& source_address,
  const eevm::Address& target_address,
  const uint256_t& amount)
{
  // To transfer tokens, the caller must be the intended source address
  auto function_call = eevm::to_bytes(
    env.contract_definition["hashes"]["transfer(address,uint256)"]);

  append_argument(function_call, target_address);
  append_argument(function_call, amount);

  std::cout << fmt::format(
                 "Transferring {} from {} to {}",
                 eevm::to_lower_hex_string(amount),
                 eevm::to_checksum_address(source_address),
                 eevm::to_checksum_address(target_address))
            << std::endl;

  const auto output =
    run_and_check_result(env, source_address, contract_address, function_call);

  // Output should be a bool in a 32-byte vector.
  if (output.size() != 32 || (output[31] != 0 && output[31] != 1))
  {
    throw std::runtime_error("Unexpected output from call to transfer");
  }

  const bool success = output[31] == 1;
  std::cout << (success ? " (succeeded)" : " (failed)") << std::endl;

  return success;
}

// Send N randomly generated token transfers. Some will be to new user addresses
template <size_t N>
void run_random_transactions(
  Environment& env, const eevm::Address& contract_address, Addresses& users)
{
  const auto total_supply = get_total_supply(env, contract_address);
  const auto transfer_max = (2 * total_supply) / N;

  for (size_t i = 0; i < N; ++i)
  {
    const auto from_index = rand_range(users.size());
    auto to_index = rand_range(users.size());

    // Occasionally create new users and transfer to them. Also avoids
    // self-transfer
    if (from_index == to_index)
    {
      to_index = users.size();
      users.push_back(get_random_address());
    }

    const auto amount = get_random_uint256() % transfer_max;

    transfer(env, contract_address, users[from_index], users[to_index], amount);
  }
}

// Print the total token supply and current token balance of each user, by
// sending transactions to the given contract_address
void print_erc20_state(
  const std::string& heading,
  Environment& env,
  const eevm::Address& contract_address,
  const Addresses& users)
{
  const auto total_supply = get_total_supply(env, contract_address);

  using Balances = std::vector<std::pair<eevm::Address, uint256_t>>;
  Balances balances;

  for (const auto& user : users)
  {
    balances.emplace_back(
      std::make_pair(user, get_balance(env, contract_address, user)));
  }

  std::cout << heading << std::endl;
  std::cout << fmt::format(
                 "Total supply of tokens is: {}",
                 eevm::to_lower_hex_string(total_supply))
            << std::endl;
  std::cout << "User balances: " << std::endl;
  for (const auto& pair : balances)
  {
    std::cout << fmt::format(
      " {} owned by {}",
      eevm::to_lower_hex_string(pair.second),
      eevm::to_checksum_address(pair.first));
    if (pair.first == env.owner_address)
    {
      std::cout << " (original contract creator)";
    }
    std::cout << std::endl;
  }
  std::cout << std::string(heading.size(), '-') << std::endl;
}

// erc20/main
// - Parse args
// - Parse ERC20 contract definition
// - Deploy ERC20 contract
// - Transfer ERC20 tokens
// - Print summary of state
int main(int argc, char** argv)
{
  srand(time(nullptr));

  if (argc < 2)
  {
    std::cout << fmt::format("Usage: {} path/to/ERC20_combined.json", argv[0])
              << std::endl;
    return 1;
  }

  const uint256_t total_supply = 1000;
  Addresses users;

  // Create an account at a random address, representing the 'owner' who created
  // the ERC20 contract (gets entire token supply initially)
  const auto owner_address = get_random_address();
  users.push_back(owner_address);

  // Create one other initial user
  const auto alice = get_random_address();
  users.push_back(alice);

  // Open the contract definition file
  const auto contract_path = argv[1];
  std::ifstream contract_fstream(contract_path);
  if (!contract_fstream)
  {
    throw std::runtime_error(
      fmt::format("Unable to open contract definition file {}", contract_path));
  }

  // Parse the contract definition from file
  const auto contracts_definition = nlohmann::json::parse(contract_fstream);
  const auto all_contracts = contracts_definition["contracts"];
  const auto erc20_definition = all_contracts["ERC20.sol:ERC20Token"];

  // Create environment
  eevm::SimpleGlobalState gs;
  Environment env{gs, owner_address, erc20_definition};

  // Deploy the ERC20 contract
  const auto contract_address = deploy_erc20_contract(env, total_supply);

  // Report initial state
  print_erc20_state("-- Initial state --", env, contract_address, users);
  std::cout << std::endl;

  // Run a successful transaction
  const auto first_transfer_amount = total_supply / 3;
  const auto success = transfer(
    env, contract_address, owner_address, alice, first_transfer_amount);
  if (!success)
  {
    throw std::runtime_error("Expected transfer to succeed, but it failed");
  }

  // Trying to transfer more than is owned will fail (gracefully, returning
  // false from the solidity function)
  const auto failure = transfer(
    env, contract_address, alice, owner_address, first_transfer_amount + 1);
  if (failure)
  {
    throw std::runtime_error("Expected transfer to fail, but it succeeded");
  }

  // Report intermediate state
  std::cout << std::endl;
  print_erc20_state(
    "-- After one transaction --", env, contract_address, users);
  std::cout << std::endl;

  // Create more users and run more transactions
  run_random_transactions<20>(env, contract_address, users);

  // Report final state
  std::cout << std::endl;
  print_erc20_state("-- Final state --", env, contract_address, users);

  return 0;
}
