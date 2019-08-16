// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "disassembler.h"
#include "opcode.h"
#include "stack.h"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace eevm
{
  struct TraceEvent
  {
    const uint64_t pc;
    const Opcode op;
    const uint16_t call_depth;
    std::unique_ptr<Stack> s;

    TraceEvent(
      const uint64_t pc,
      const Opcode op,
      const uint16_t call_depth,
      const Stack s) :
      pc(pc),
      op(op),
      call_depth(call_depth),
      s(std::make_unique<Stack>(s))
    {}

    TraceEvent(TraceEvent&& other) :
      pc(other.pc),
      op(other.op),
      call_depth(other.call_depth),
      s(std::move(other.s))
    {}
  };

  inline std::ostream& operator<<(std::ostream& os, const TraceEvent& e)
  {
    os << e.pc << " (" << e.call_depth
       << "): " << Disassembler::getOp(e.op).mnemonic;
    if (e.s)
      os << "\nstack before:\n" << *e.s;
    return os;
  }

  /**
   * Runtime trace of a smart contract (for debugging)
   */
  struct Trace
  {
    std::vector<TraceEvent> events;

    template <class... Args>
    TraceEvent& add(Args&&... args)
    {
      events.emplace_back(std::forward<Args>(args)...);
      auto& e = events.back();
      return e;
    }

    void reset()
    {
      events.clear();
    }

    void print_last_n(std::ostream& os, size_t n) const
    {
      auto first = n < events.size() ? events.size() - n : 0;
      for (auto i = first; i < events.size(); ++i)
        os << events[i] << std::endl;
      os << std::endl;
    }
  };

  inline std::ostream& operator<<(std::ostream& os, const Trace& t)
  {
    for (const auto& e : t.events)
      os << e << std::endl;
    return os;
  }
} // namespace eevm
