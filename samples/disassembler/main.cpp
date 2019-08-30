// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/disassembler.h"

#include <fmt/format_header_only.h>
#include <iostream>

int usage(const char* bin_name)
{
  std::cout << fmt::format("Usage: {} hex_bytecode", bin_name) << std::endl;
  std::cout << "Prints disassembly of argument" << std::endl;
  return 1;
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    return usage(argv[0]);
  }

  const std::vector<uint8_t> code = eevm::to_bytes(argv[1]);

  const auto dis = eevm::Disassembler::dis(code);

  std::cout << dis << std::endl;

  return 0;
}
