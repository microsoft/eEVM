# Samples

These samples show some basic examples of apps using eEVM. They are intended as an introduction; they do not test or document every opcode or feature. In particular they do not demonstrate any expensive computations or cross-contract calls.

They are compiled by the root CMakeLists.txt, and run from the command line.

## hello_world

This sample creates and calls a contract which returns the string "Hello world!".

#### Details

- Generates some random Ethereum-compliant addresses. Real addresses should be derived from public-private key-pairs or from the contract generation method specified in the Yellow Paper, but here they are essentially arbitrary identifiers so can be random.
- Constructs contract bytecode. Real smart contracts will generally be compiled from some higher-level language (such as [Solidity](https://solidity.readthedocs.io/en/v0.5.0/)), but this shows it is also possible to write by hand. When the contract is executed, it stores each byte of the string in EVM memory, then returns the stored memory range.
- Deploys the contract. To execute bytecode, it must be deployed to an address in the GlobalState.
- Executes the bytecode. This requires creating an `eevm::Transaction` and a sending address, as all EVM execution is transactional.
- Checks and prints the result. Execution may throw exceptions or halt unexpectedly, and these should be handled by checking `eevm::ExitReason eevm::ExecResult::er`. On success, any output (sent by `RETURN` opcodes in the top level code) will be in `eevm::ExecResult::output`.

#### Expected output

The compiled app can be run with no arguments:

```bash
$ ./hello_world
Hello world!
```

## sum

This sample creates a contract which sums a pair of arguments passed on the command line. The summation is done within the EVM, by the `ADD` op code.

#### Details

In addition to the concepts from [hello_world](#hello_world), this sample:

- Parses args from command line. This demonstrates some of the helpers from [util.h](../include/eEVM/util.h), specifically `to_uint256` and `to_hex_string` for reading and writing 256-bit EVM numbers (or 160-bit addresses).
- Produces bytecode containing 256-bit immediates. Although the bytecode and each op code is a single byte, the complete code may contain larger values (such as the immediate for a `PUSH32` instruction) which must be serialized.
- Produces verbose output. If the `-v` option is given, this sample will print the precise contract address and code contents.

#### Expected output

The compiled app expects 2 arguments, which will be treated as hex-encoded 256-bit unsigned numbers (arguments which are too long will be truncated, invalid arguments will generally parse to 0):

```bash
$ ./sum
Usage: ./sum [-v] hex_a hex_b
Prints sum of arguments (hex string representation of 256-bit uints)

$ ./sum C0C0 EDA
0xC0C0 + 0xEDA = 0xCF9A
```

## erc20

This sample demonstrates an [ERC20](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-20.md) token contract compiled from [Solidity](https://solidity.readthedocs.io/en/v0.5.0/index.html). The contract is deployed, then multiple calls are made to transfer tokens between addresses. The contract source is in [ERC20.sol](erc20/ERC20.sol), with the compiled result in [ERC20_combined.json](erc20/ERC20_combined.json) produced by:

```bash
$ solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize ERC20.sol > ERC20_combined.json
```

#### Details

- Parses a contract definition. The compiled output is read from a file, then [nlohmann::json](https://github.com/nlohmann/json) is used to extract the relevant fields from json.
- Deploys contract from definition. This demonstrates basic ABI encoding of arguments. The "bin" field from the contract definition is the raw code for the constructor - it must be given a single argument then run to produce the actual contract state and code.
- Calls functions on a deployed contract. The function selectors are retrieved from the "hashes" field of the contract definition, their ABI-encoded arguments appended, and the result is passed as input to an EVM transaction. All function calls (totalSupply, balanceOf, transfer) are like this, and use `run_and_check_result` to wrap the execution in `eevm::Processor::run`.
- Address storage. Addresses are individual identities, and may have associated balances or state in any number of contracts' storage - all of which is contained within `GlobalState`. Only the address must be retained and passed.
- State reporting. The contract (and entire network) state is embedded in `GlobalState`, but not in an easily readable form. Instead the state can be reconstructed by additional read-only transactions sent to the ERC20 contract, and those results converted to a human-readable representation of the returned values.

#### Expected output

The combined json file produced by solc should be passed as an argument:

```bash
$ ./erc20 ../samples/erc20/ERC20_combined.json
-- Initial state --
Total supply of tokens is: 1000
User balances:
  1000 owned by 0x7B6...7EF (original contract creator)
  0 owned by 0x53D...3F0
-------------------

Transferring 333 from 0x7B6...7EF to 0x53D...3F0 (succeeded)
Transferring 334 from 0x53D...3F0 to 0x7B6...7EF (failed)

-- After one transaction --
Total supply of tokens is: 1000
User balances:
  667 owned by 0x7B6...7EF (original contract creator)
  333 owned by 0x53D...3F0
---------------------------

Transferring 12 from 0x53D...3F0 to 0x7B6...7EF (succeeded)
Transferring 76 from 0x53D...3F0 to 0x7B6...7EF (succeeded)
<...>
Transferring 60 from 0x7B6...7EF to 0x53D...3F0 (succeeded)
Transferring 83 from 0xB2B...733 to 0x8EE...584 (failed)

-- Final state --
Total supply of tokens is: 1000
User balances:
  391 owned by 0x7B6...7EF (original contract creator)
  329 owned by 0x53D...3F0
  160 owned by 0x223...3E7
  3 owned by 0x8EE...584
  16 owned by 0x590...CE7
  22 owned by 0xA1D...A3A
  79 owned by 0xB2B...733
-----------------
```

