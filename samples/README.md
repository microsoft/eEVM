# Samples

These samples some basic examples of apps which uses eEVM. They are intended as an introduction; they do not test or document every opcode or feature. They do not demonstrate any expensive computations or cross-contract calls.

They are compiled by the root CMakeLists.txt, and run from the command line.

## hello_world

This sample creates a contract which returns "Hello world!" as a byte string.

### Details

- Generate some random Ethereum-compliant addresses. Real addresses should be derived from public-private key-pairs or from the contract generation method specified in the Yellow Paper, but here they are essentially arbitrary identifiers so are generated randomly.
- Construct contract bytecode. Real smart contracts will generally be compiled from some higher-level language (such as Solidity), but this shows it is also possible to write by hand. When the contract is executed, it stores each byte of the string in EVM memory, then returns the stored memory range.
- Deploy the contract. To execute bytecode, it must be deployed to an address in the GlobalState.
- Execute the bytecode. This requires creating a Transaction and a sending address, as all EVM execution is associated with a transaction.
- Check and print the result. Execution may throw exceptions or halt unexpectedly, and these should be handled by checking `evm::ExitReason evm::ExecResult::er`. If it succeeds, any output (sent by `RETURN` opcodes in the top level code) will be in `evm::ExecResult::output`.

### Expected output

The compiled app can be run with no arguments:

```bash
$ ./hello_world
Hello world!
```

## sum

This sample creates a contract which sums a pair of arguments passed on the command line. The summation is done within the EVM, by the `ADD` op code.

### Details

In addition to the concepts from [hello_world](#hello_world), this sample:

- Parses args from command line. This demonstrates some of the helpers from [util.h](../include/util.h), specifically `from_hex_str` and `to_hex_str` for reading and writing 256-bit EVM numbers (or 160-bit addresses).
- Produces bytecode containing 256-bit immediates. Although the bytecode and each op code is a single byte, the complete code may contain larger values (such as the immediate for a `PUSH32` instruction) which must be serialized.
- Produces verbose output. If the `-v` option is given, this sample will print the precise contract address and code contents.

### Expected output

The compiled app expects 2 arguments, which will be treated as hex-encoded 256-bit unsigned numbers (arguments which are too long will be truncated, invalid arguments will generally parse to 0):

```bash
$ ./sum
Usage: ./sum [-v] hex_a hex_b
Prints sum of arguments (hex string representation of 256-bit uints)

$ ./sum C0C0 EDA
0xC0C0 + 0xEDA = 0xCF9A
```

