// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "disassembler.h"
#include "opcode.h"
#include "stack.h"

#include <fmt/format_header_only.h>
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
      {
        os << fmt::format("{}", events[i]) << std::endl;
      }
    }
  };
} // namespace eevm

namespace fmt
{
  template <>
  struct formatter<eevm::TraceEvent>
  {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
      return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const eevm::TraceEvent& e, FormatContext& ctx)
    {
      auto s = format_to(
        ctx.out(),
        "{} ({}): {}",
        e.pc,
        e.call_depth,
        eevm::Disassembler::getOp(e.op).mnemonic);

      if (e.s)
        s = format_to(ctx.out(), "\nstack before:\n{}", *e.s);

      return s;
    }
  };

  template <>
  struct formatter<eevm::Trace>
  {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
      return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const eevm::Trace& t, FormatContext& ctx)
    {
      return format_to(ctx.out(), "{}", fmt::join(t.events, "\n"));
    }
  };
} // namespace fmt
