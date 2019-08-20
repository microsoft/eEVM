// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "bigint.h"

namespace eevm
{
  // NOTE: Addresses will only use the low 160-bits, but it is simpler to use
  // overloads to serialise/pass these as any other 256-bit value. This is how
  // they are stored in EVM bytecode/memory.
  using Address = uint256_t;
}
