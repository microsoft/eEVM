// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "bigint.h"
#include "exception.h"
#include "opcode.h"
#include "util.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace eevm
{
  struct Op
  {
    Opcode opcode;
    const char* mnemonic;
    const uint8_t immediate_bytes;
    const uint32_t gas = 0;

    Op(
      Opcode opcode,
      const char* mnemonic,
      uint8_t immediate_bytes = 0,
      uint32_t gas = 0) :
      opcode(opcode),
      mnemonic(mnemonic),
      immediate_bytes(immediate_bytes),
      gas(gas)
    {}

    bool has_immediate() const
    {
      return immediate_bytes > 0;
    }
  };

  inline std::ostream& operator<<(std::ostream& os, const Op& op)
  {
    os << op.mnemonic;
    return os;
  }

  struct Instr
  {
    const Op op;
    const uint64_t pc;
    const std::vector<uint8_t> raw_imm;
    std::string comment;

    Instr(const Op& op, const uint64_t pc, const std::vector<uint8_t> raw_imm) :
      op(op),
      pc(pc),
      raw_imm(raw_imm)
    {}

    auto get_immediate() const
    {
      if (!raw_imm.size())
        throw std::logic_error("Instruction does not have immediate.");
      return from_big_endian(raw_imm.data(), raw_imm.size());
    }
  };

  inline std::ostream& operator<<(std::ostream& os, const Instr& i)
  {
    os << fmt::format(
      "{:>5}: {}{} [{:02x}{}]; {}",
      i.pc,
      i.op.mnemonic,
      i.op.has_immediate() ?
        fmt::format(" {}", to_lower_hex_string(i.get_immediate())) :
        "",
      (int)i.op.opcode,
      i.raw_imm.size() > 0 ? fmt::format(" {:02x}", fmt::join(i.raw_imm, " ")) :
                             "",
      i.comment);
    return os;
  }

  struct Disassembly
  {
    std::map<uint64_t, std::unique_ptr<Instr>> instrs;
  };

  inline std::ostream& operator<<(std::ostream& os, const Disassembly& d)
  {
    for (const auto& instr : d.instrs)
      os << *instr.second << std::endl;
    return os;
  }

  struct Disassembler
  {
    const static std::unordered_map<uint8_t, Op> ops;

    static Op getOp(Opcode oc)
    {
      const auto op_it = ops.find(oc);
      if (op_it == ops.end())
      {
        return Op(oc, "INVALID");
      }
      else
      {
        return op_it->second;
      }
    }

    static Disassembly dis(const std::vector<uint8_t>& prog)
    {
      Disassembly d;
      std::unordered_map<uint8_t, std::set<Instr*>> reverse_lookup;
      // construct initial disassembly
      for (auto it = prog.cbegin(); it != prog.cend();)
      {
        const auto pc = it - prog.cbegin();
        const auto opcode = *it++;
        const auto op = getOp((Opcode)opcode);
        const auto bytes_left = prog.cend() - it;
        if (bytes_left < op.immediate_bytes)
          throw std::out_of_range(fmt::format(
            "Immediate exceeds instruction stream (op {} at "
            "instruction {} wants {} bytes, only {} remain)",
            op.mnemonic,
            (size_t)(it - prog.cbegin()),
            op.immediate_bytes,
            bytes_left));

        auto instr = std::make_unique<Instr>(
          op, pc, std::vector<uint8_t>(it, it + op.immediate_bytes));
        it += op.immediate_bytes;
        reverse_lookup[opcode].insert(instr.get());
        d.instrs[pc] = std::move(instr);
      }

      // give names to jump dests
      for (auto dest : reverse_lookup[Opcode::JUMPDEST])
      {
        std::stringstream comment;
        comment << "loc_" << std::dec << dest->pc;
        dest->comment = comment.str();
      }

      auto jumps = reverse_lookup[Opcode::JUMP];
      auto& cond_jumps = reverse_lookup[Opcode::JUMPI];
      jumps.insert(cond_jumps.begin(), cond_jumps.end());

      // resolve jumps using simple heuristics
      for (auto jump : jumps)
      {
        auto _jump = d.instrs.find(jump->pc);
        if (_jump == d.instrs.begin())
          continue;
        const auto& prev = (--_jump)->second;
        // is the preceding instruction a push?
        if (!prev->op.has_immediate())
          continue;
        std::stringstream comment;
        const auto target_address =
          static_cast<uint64_t>(prev->get_immediate());
        const auto target = d.instrs.find(target_address);
        if (
          target == d.instrs.end() ||
          target->second->op.opcode != Opcode::JUMPDEST)
          comment << "illegal target";
        else
          comment << "branches to " << target->second->comment;
        jump->comment = comment.str();
      }
      return d;
    }
  };
} // namespace eevm
