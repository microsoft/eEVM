// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <stdint.h>

namespace eevm
{
  /**
   * All opcodes supported by our EVM. Note that we currently target Homestead
   * compliance, so we are missing some more recent additions
   * (RETURNDATASIZE, RETURNDATACOPY, STATICCALL, REVERT, INVALID)
   */
  enum Opcode : uint8_t
  {
    // 0s: Stop and Arithmetic Operations
    STOP       = 0x00, // Halts execution
    ADD        = 0x01, // Addition operation
    MUL        = 0x02, // Multiplication operation
    SUB        = 0x03, // Substraction operation
    DIV        = 0x04, // Integer division operation
    SDIV       = 0x05, // Signed integer division operation (truncated)
    MOD        = 0x06, // Modulo remainder operation
    SMOD       = 0x07, // Signed modulo remainder operation
    ADDMOD     = 0x08, // Modulo addition operation
    MULMOD     = 0x09, // Modulo multiplication operation
    EXP        = 0x0a, // Exponential operation
    SIGNEXTEND = 0x0b, // Extend length of two’s complement signed integer.

    // 10s: Comparison & Bitwise Logic Operations
    LT     = 0x10, // Less-than comparison
    GT     = 0x11, // Greater-than comparison
    SLT    = 0x12, // Signed less-than comparison
    SGT    = 0x13, // Signed greater-than comparison
    EQ     = 0x14, // Equality comparison
    ISZERO = 0x15, // Simple not operator
    AND    = 0x16, // Bitwise AND operation
    OR     = 0x17, // Bitwise OR operation
    XOR    = 0x18, // Bitwise XOR operation
    NOT    = 0x19, // Bitwise NOT operation
    BYTE   = 0x1a, // Retrieve single byte from word

    // 20s: SHA3
    SHA3   = 0x20, // Compute Keccak-256 hash.

    // 30s: Environmental Information
    ADDRESS        = 0x30, // Get address of currently executing account
    BALANCE        = 0x31, // Get balance of the given account.
    ORIGIN         = 0x32, //  Get execution origination address (This is the sender of original transaction; it is never an account with non-emptyassociated code.)
    CALLER         = 0x33, // Get caller address (This is the address of the account that is directly responsible for this execution.)
    CALLVALUE      = 0x34, // Get deposited value by the instruction/transaction responsible for this execution.
    CALLDATALOAD   = 0x35, // Get input data of current environment. (This pertains to the input data passed with the message call instruction or transaction.)
    CALLDATASIZE   = 0x36, // Get size of input data in current environment (This pertains to the input data passed with the message call instruction or transaction.)
    CALLDATACOPY   = 0x37, //  Copy input data in current environment to memory. (This pertains to the input data passed with the message call instruction or transaction.)
    CODESIZE       = 0x38, //  Get size of code running in current environment.
    CODECOPY       = 0x39, // Copy code running in current environment to memory
    GASPRICE       = 0x3a, //  Get price of gas in current environment. (This is gas price specified by the originating transaction.)
    EXTCODESIZE    = 0x3b, // Get size of an account’s code.
    EXTCODECOPY    = 0x3c, // Copy an account’s code to memory
    // RETURNDATASIZE = 0x3d, // Get size of output data from the previous call from the current environment.
    // RETURNDATACOPY = 0x3e, // Copy output data from the previous call to memory.

    // 40s: Block Information
    BLOCKHASH  = 0x40, // Get the hash of one of the 256 most recent complete blocks.
    COINBASE   = 0x41, // Get the block’s beneficiary address
    TIMESTAMP  = 0x42, // Get the block’s timestamp.
    NUMBER     = 0x43, // Get the block’s number
    DIFFICULTY = 0x44, // Get the block’s difficulty.
    GASLIMIT   = 0x45, // Get the block’s gas limit

    // 50s: Stack, Memory, Storage and Flow Operations
    POP      = 0x50, // Remove item from stack.
    MLOAD    = 0x51, // Load word from memory.
    MSTORE   = 0x52, // Save word to memory
    MSTORE8  = 0x53, // Save byte to memory
    SLOAD    = 0x54, // Load word from storage.
    SSTORE   = 0x55, // Save word to storage
    JUMP     = 0x56, // Alter the program counter.
    JUMPI    = 0x57, // Conditionally alter the program counter.
    PC       = 0x58, // Get the value of the program counter prior to the increment corresponding to this instruction.
    MSIZE    = 0x59, // Get the size of active memory in bytes.
    GAS      = 0x5a, // Get the amount of available gas, including the corresponding reduction for the cost of this instruction.
    JUMPDEST = 0x5b, // Mark a valid destination for jumps. This operation has no effect on machine state during execution

    // 60s & 70s: Push Operations
    PUSH1  = 0x60, // Place 1 byte item on stack.
    PUSH2  = 0x61, // Place 2 byte item on stack.
    PUSH3  = 0x62, // Place 3 byte item on stack.
    PUSH4  = 0x63, // Place 4 byte item on stack.
    PUSH5  = 0x64, // Place 5 byte item on stack.
    PUSH6  = 0x65, // Place 6 byte item on stack.
    PUSH7  = 0x66, // Place 7 byte item on stack.
    PUSH8  = 0x67, // Place 8 byte item on stack.
    PUSH9  = 0x68, // Place 9 byte item on stack.
    PUSH10 = 0x69, // Place 10 byte item on stack.
    PUSH11 = 0x6a, // Place 11 byte item on stack.
    PUSH12 = 0x6b, // Place 12 byte item on stack.
    PUSH13 = 0x6c, // Place 13 byte item on stack.
    PUSH14 = 0x6d, // Place 14 byte item on stack.
    PUSH15 = 0x6e, // Place 15 byte item on stack.
    PUSH16 = 0x6f, // Place 16 byte item on stack.
    PUSH17 = 0x70, // Place 17 byte item on stack.
    PUSH18 = 0x71, // Place 18 byte item on stack.
    PUSH19 = 0x72, // Place 19 byte item on stack.
    PUSH20 = 0x73, // Place 20 byte item on stack.
    PUSH21 = 0x74, // Place 21 byte item on stack.
    PUSH22 = 0x75, // Place 22 byte item on stack.
    PUSH23 = 0x76, // Place 23 byte item on stack.
    PUSH24 = 0x77, // Place 24 byte item on stack.
    PUSH25 = 0x78, // Place 25 byte item on stack.
    PUSH26 = 0x79, // Place 26 byte item on stack.
    PUSH27 = 0x7a, // Place 27 byte item on stack.
    PUSH28 = 0x7b, // Place 28 byte item on stack.
    PUSH29 = 0x7c, // Place 29 byte item on stack.
    PUSH30 = 0x7d, // Place 30 byte item on stack.
    PUSH31 = 0x7e, // Place 31 byte item on stack.
    PUSH32 = 0x7f, // Place 32 byte item on stack.

    // 80s: Duplication Operation
    DUP1  = 0x80, // Duplicate 1st stack item
    DUP2  = 0x81, // Duplicate 2nd stack item
    DUP3  = 0x82, // Duplicate 3rd stack item
    DUP4  = 0x83, // Duplicate 4th stack item
    DUP5  = 0x84, // Duplicate 5th stack item
    DUP6  = 0x85, // Duplicate 6th stack item
    DUP7  = 0x86, // Duplicate 7th stack item
    DUP8  = 0x87, // Duplicate 8th stack item
    DUP9  = 0x88, // Duplicate 9th stack item
    DUP10 = 0x89, // Duplicate 10th stack item
    DUP11 = 0x8a, // Duplicate 11th stack item
    DUP12 = 0x8b, // Duplicate 12th stack item
    DUP13 = 0x8c, // Duplicate 13th stack item
    DUP14 = 0x8d, // Duplicate 14th stack item
    DUP15 = 0x8e, // Duplicate 15th stack item
    DUP16 = 0x8f, // Duplicate 16th stack item

    // 90s: Exchange Operation
    SWAP1  = 0x90, // Exchange 1st and 2nd stack item
    SWAP2  = 0x91, // Exchange 1st and 3rd stack item
    SWAP3  = 0x92, // Exchange 1st and 4th stack item
    SWAP4  = 0x93, // Exchange 1st and 5th stack item
    SWAP5  = 0x94, // Exchange 1st and 6th stack item
    SWAP6  = 0x95, // Exchange 1st and 7th stack item
    SWAP7  = 0x96, // Exchange 1st and 8th stack item
    SWAP8  = 0x97, // Exchange 1st and 9th stack item
    SWAP9  = 0x98, // Exchange 1st and 10th stack item
    SWAP10 = 0x99, // Exchange 1st and 11th stack item
    SWAP11 = 0x9a, // Exchange 1st and 12th stack item
    SWAP12 = 0x9b, // Exchange 1st and 13th stack item
    SWAP13 = 0x9c, // Exchange 1st and 14th stack item
    SWAP14 = 0x9d, // Exchange 1st and 15th stack item
    SWAP15 = 0x9e, // Exchange 1st and 16th stack item
    SWAP16 = 0x9f, // Exchange 1st and 17th stack item

    // a0s: Logging Operations
    LOG0 = 0xa0, // Append log record with no topics
    LOG1 = 0xa1, // Append log record with 1 topic
    LOG2 = 0xa2, // Append log record with 2 topics
    LOG3 = 0xa3, // Append log record with 3 topics
    LOG4 = 0xa4, // Append log record with 4 topics

    // f0s: System operations
    CREATE       = 0xf0, // Create a new account with associated code
    CALL         = 0xf1, // Message-call into an account
    CALLCODE     = 0xf2, // Message-call into this account with an alternative account’s code
    RETURN       = 0xf3, // Halt execution returning output data
    DELEGATECALL = 0xf4, // Message-call into this account with an alternative account’s code, but persisting the current values for sender and value.
    // STATICCALL   = 0xfa, // Static message-call into an account. Exactly equivalent to CALL except: The argument µs is replaced with 0.
    // REVERT       = 0xfd, // Halt execution reverting state changes but returning data and remaining gas
    // INVALID      = 0xfe, // Designated invalid instruction
    SELFDESTRUCT = 0xff
  };
} // namespace eevm
