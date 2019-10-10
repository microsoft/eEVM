// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "eEVM/stack.h"

#include "eEVM/exception.h"
#include "eEVM/util.h"

#include <algorithm>
#include <limits>

using namespace std;

namespace eevm
{
  using ET = Exception::Type;

  uint256_t Stack::pop()
  {
    // TODO: don't check size for every single pop, but rather once at the
    // beginning of each op handler in vm.cpp
    if (st.empty())
      throw Exception(ET::outOfBounds, "Stack out of range");

    uint256_t val = st.front();
    st.pop_front();
    return val;
  }

  uint64_t Stack::pop64()
  {
    const auto val = pop();
    if (val > numeric_limits<uint64_t>::max())
      throw Exception(
        ET::outOfBounds,
        "Value on stack (" + to_hex_string(val) + ") is larger than 2^64");

    return static_cast<uint64_t>(val);
  }

  void Stack::push(const uint256_t& val)
  {
    if (size() == MAX_SIZE)
      throw Exception(
        ET::outOfBounds,
        "Stack mem exceeded (" + to_string(size()) +
          " == " + to_string(MAX_SIZE) + ")");

    try
    {
      st.push_front(val);
    }
    catch (const std::bad_alloc&)
    {
      throw std::runtime_error("bad_alloc while pushing onto stack");
    }
  }

  uint64_t Stack::size() const
  {
    return st.size();
  }

  void Stack::swap(uint64_t i)
  {
    if (i >= size())
      throw Exception(
        ET::outOfBounds,
        "Swap out of range (" + to_string(i) + " >= " + to_string(size()) +
          ")");

    std::swap(st[0], st[i]);
  }

  void Stack::dup(uint64_t a)
  {
    if (a >= size())
      throw Exception(
        ET::outOfBounds,
        "Dup out of range (" + to_string(a) + " >= " + to_string(size()) + ")");

    st.push_front(st[a]);
  }

  std::ostream& operator<<(std::ostream& os, const Stack& s)
  {
    int i = 0;
    os << std::dec;
    for (const auto& elem : s.st)
      os << fmt::format(" {}: {}", i++, to_hex_string(elem)) << std::endl;
    return os;
  }
} // namespace eevm
