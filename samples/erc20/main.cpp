#include "evm/simpleglobalstate.h"
#include "include/opcode.h"
#include "include/processor.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <vector>

using Addresses = std::vector<evm::Address>;

struct Environment
{
  evm::GlobalState& gs;
  const evm::Address& owner_address;
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
  return from_big_endian(raw.begin(), raw.end());
}

evm::Address get_random_address()
{
  return get_random_uint256(20);
}

// Truncate 160-bit addresses to a more human-friendly length, retaining the
// start and end for identification
std::string short_name(const evm::Address& address)
{
  const auto full_hex = to_hex_str(address);
  return full_hex.substr(0, 5) + std::string("...") +
    full_hex.substr(full_hex.size() - 3);
}

std::vector<uint8_t> run_and_check_result(
  Environment& env,
  const evm::Address& from,
  const evm::Address& to,
  const evm::Code& input)
{
  // Ignore any logs produced by this transaction
  evm::NullLogHandler ignore;
  evm::Transaction tx(from, ignore);

  // Record a trace to aid debugging
  evm::Trace tr;
  evm::Processor p(env.gs);

  // Run the transaction
  const auto exec_result = p.run(tx, from, env.gs.get(to), input, 0u, &tr);

  if (exec_result.er != evm::ExitReason::returned)
  {
    // Print the trace if nothing was returned
    std::cerr << tr << std::endl;
    if (exec_result.er == evm::ExitReason::threw)
    {
      // Rethrow to highlight any exceptions raised in execution
      throw std::runtime_error(
        "Execution threw an error: " + exec_result.exmsg);
    }

    throw std::runtime_error("Deployment did not return");
  }

  return exec_result.output;
}

void append_argument(std::vector<uint8_t>& code, const uint256_t& arg)
{
  // To ABI encode a function call with a uint256_t (or Address) argument,
  // simply append the big-endian byte representation to the code (function
  // selector, or bin). ABI-encoding for more complicated types is more
  // complicated, so not shown in this sample.
  const auto pre_size = code.size();
  code.resize(pre_size + 32u);
  to_big_endian(arg, code.data() + pre_size);
}

evm::Address deploy_erc20_contract(
  Environment& env, const uint256_t total_supply)
{
  // Generate the contract address
  const auto contract_address = evm::generate_address(env.owner_address, 0u);

  // Get the binary constructor of the contract
  auto contract_constructor = evm::to_bytes(env.contract_definition["bin"]);

  // The constructor takes a single argument (total_supply) - append it
  append_argument(contract_constructor, total_supply);

  // Set this constructor as the contract's code body
  auto contract = env.gs.create(contract_address, 0u, contract_constructor);

  // Run a transaction to initialise this account
  auto result =
    run_and_check_result(env, env.owner_address, contract_address, {});

  // Result of running the compiled constructor is the code that should be the
  // contract's body (constructor will also have setup contract's Storage)
  contract.acc.code = result;

  return contract.acc.address;
}

uint256_t get_total_supply(
  Environment& env, const evm::Address& contract_address)
{
  // Anyone can call totalSupply - prove this by asking from a randomly
  // generated address
  const auto caller = get_random_address();

  const auto function_call =
    evm::to_bytes(env.contract_definition["hashes"]["totalSupply()"]);

  const auto output =
    run_and_check_result(env, caller, contract_address, function_call);

  return from_big_endian(output.begin(), output.end());
}

uint256_t get_balance(
  Environment& env,
  const evm::Address& contract_address,
  const evm::Address& target_address)
{
  // Anyone can call balanceOf - prove this by asking from a randomly generated
  // address
  const auto caller = get_random_address();

  auto function_call =
    evm::to_bytes(env.contract_definition["hashes"]["balanceOf(address)"]);

  append_argument(function_call, target_address);

  const auto output =
    run_and_check_result(env, caller, contract_address, function_call);

  return from_big_endian(output.begin(), output.end());
}

bool transfer(
  Environment& env,
  const evm::Address& contract_address,
  const evm::Address& source_address,
  const evm::Address& target_address,
  const uint256_t& amount)
{
  // To transfer tokens, the caller must be the intended source address
  auto function_call = evm::to_bytes(
    env.contract_definition["hashes"]["transfer(address,uint256)"]);

  append_argument(function_call, target_address);
  append_argument(function_call, amount);

  std::cout << "Transferring " << amount << " from "
            << short_name(source_address) << " to "
            << short_name(target_address);

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

void run_random_transactions(
  Environment& env, const evm::Address& contract_address, Addresses& users)
{
  constexpr auto transactions = 20;
  const auto total_supply = get_total_supply(env, contract_address);
  const auto transfer_max = (2 * total_supply) / transactions;

  for (auto i = 0; i < transactions; ++i)
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

void print_erc20_state(
  const std::string& heading,
  Environment& env,
  const evm::Address& contract_address,
  const Addresses& users)
{
  const auto total_supply = get_total_supply(env, contract_address);

  using Balances = std::vector<std::pair<evm::Address, uint256_t>>;
  Balances balances;

  for (const auto& user : users)
  {
    balances.emplace_back(
      std::make_pair(user, get_balance(env, contract_address, user)));
  }

  std::cout << heading << std::endl;
  std::cout << "Total supply of tokens is: " << total_supply << std::endl;
  std::cout << "User balances: " << std::endl;
  for (const auto& pair : balances)
  {
    std::cout << "  " << pair.second << " owned by " << short_name(pair.first);
    if (pair.first == env.owner_address)
    {
      std::cout << " (original contract creator)";
    }
    std::cout << std::endl;
  }
  std::cout << std::string(heading.size(), '-') << std::endl;
}

int main(int argc, char** argv)
{
  srand(time(nullptr));

  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " path_to_contract_file" << std::endl;
    return 1;
  }

  const uint256_t total_supply = 1000;
  Addresses users;

  // Create an account at a random address, representing the 'owner' or user who
  // created the ERC20 contract (gets entire token supply initially)
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
      std::string("Unable to open contract definition file ") + contract_path);
  }

  // Parse the contract definition from file
  const auto contracts_definition = nlohmann::json::parse(contract_fstream);
  const auto all_contracts = contracts_definition["contracts"];
  const auto erc20_definition = all_contracts["ERC20.sol:ERC20Token"];

  // Create environment
  evm::SimpleGlobalState gs;
  Environment env{gs, owner_address, erc20_definition};

  // Deploy the ERC20 contract
  const auto contract_address = deploy_erc20_contract(env, total_supply);

  // Report initial state
  print_erc20_state("-- Initial state --", env, contract_address, users);
  std::cout << std::endl;

  // Run a successful transaction
  const auto success =
    transfer(env, contract_address, owner_address, alice, total_supply / 2);
  if (!success)
  {
    throw std::runtime_error("Expected transfer to succeed, but it failed");
  }

  // Trying to transfer more than is owned will fail (gracefully, returning
  // false from the solidity function)
  const auto failure =
    transfer(env, contract_address, owner_address, alice, total_supply * 2);
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
  run_random_transactions(env, contract_address, users);

  // Report final state
  std::cout << std::endl;
  print_erc20_state("-- Final state --", env, contract_address, users);

  return 0;
}
