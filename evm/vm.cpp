// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../include/vm.h"

#include "../include/bigint.h"
#include "../include/exception.h"
#include "../include/opcode.h"
#include "../include/stack.h"
#include "../include/util.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <type_traits>
#include <utility>

using namespace std;

namespace evm
{
  struct Consts
  {
    static constexpr auto MAX_CALL_DEPTH = 1024u;
    static constexpr auto WORD_SIZE = 32u;
    static constexpr auto MAX_MEM_SIZE = 1ull << 25; // 32 MB
  };

  /**
   * bytecode program
   */
  class Program
  {
  public:
    const vector<uint8_t>& code;
    const set<uint64_t> jump_dests;

    Program(const vector<uint8_t>& code) :
      code(code),
      jump_dests(compute_jump_dests(code))
    {}

  private:
    set<uint64_t> compute_jump_dests(const vector<uint8_t>& code)
    {
      set<uint64_t> dests;
      for (uint64_t i = 0; i < code.size(); i++)
      {
        const auto op = code[i];
        if (op >= PUSH1 && op <= PUSH32)
        {
          const uint8_t immediate_bytes = op - static_cast<uint8_t>(PUSH1) + 1;
          i += immediate_bytes;
        }
        else if (op == JUMPDEST)
          dests.insert(i);
      }
      return dests;
    }
  };

  /**
   * execution context of a call
   */
  class Context
  {
  private:
    uint64_t pc = 0;
    bool pc_changed = true;

    using PcType = decltype(pc);

  public:
    using ReturnHandler = function<void(vector<uint8_t>)>;
    using HaltHandler = function<void()>;
    using ExceptionHandler = function<void(const Exception&)>;

    vector<uint8_t> mem;
    Stack s;

    AccountState as;
    Account& acc;
    Storage& st;
    const Address caller;
    const vector<uint8_t> input;
    const uint64_t call_value;
    const Program prog;
    ReturnHandler rh;
    HaltHandler hh;
    ExceptionHandler eh;

    Context(
      const Address& caller,
      AccountState as,
      vector<uint8_t>&& input,
      uint64_t call_value,
      Program&& prog,
      ReturnHandler&& rh,
      HaltHandler&& hh,
      ExceptionHandler&& eh) :
      as(as),
      acc(as.acc),
      st(as.st),
      caller(caller),
      input(input),
      call_value(call_value),
      prog(prog),
      rh(rh),
      hh(hh),
      eh(eh)
    {}

    /// increment the pc if it wasn't changed before
    void step()
    {
      if (pc_changed)
        pc_changed = false;
      else
        pc++;
    }

    PcType get_pc() const
    {
      return pc;
    }

    void set_pc(const PcType pc_)
    {
      pc = pc_;
      pc_changed = true;
    }

    bool pc_valid() const
    {
      return pc < prog.code.size();
    }

    auto get_used_mem() const
    {
      return (mem.size() + Consts::WORD_SIZE - 1) / Consts::WORD_SIZE;
    }
  };

  /**
   * implementation of the VM
   */
  class _Processor
  {
  private:
    /// the interface to the global state
    GlobalState& gs;
    /// the transaction object
    Transaction& tx;
    /// pointer to trace object (for debugging)
    Trace* const tr;
    /// the stack of contexts (one per nested call)
    vector<unique_ptr<Context>> ctxts;
    /// pointer to the current context
    Context* ctxt;

    using ET = Exception::Type;

  public:
    _Processor(GlobalState& gs, Transaction& tx, Trace* tr) :
      gs(gs),
      tx(tx),
      tr(tr)
    {}

    ExecResult run(
      const Address& caller,
      AccountState callee,
      vector<uint8_t> input, // Take a copy here, then move it into context
      uint64_t call_value)
    {
      // create the first context
      ExecResult result;
      auto rh = [&result](vector<uint8_t> output_) {
        result.er = ExitReason::returned;
        result.output = move(output_);
      };
      auto hh = [&result]() { result.er = ExitReason::halted; };
      auto eh = [&result](const Exception& ex_) {
        result.er = ExitReason::threw;
        result.ex = ex_.type;
        result.exmsg = ex_.what();
      };

      push_context(
        caller, callee, move(input), callee.acc.code, call_value, rh, hh, eh);

      // run
      while (ctxt->get_pc() < ctxt->prog.code.size())
      {
        try
        {
          dispatch();
        }
        catch (Exception& ex)
        {
          ctxt->eh(ex);
          pop_context();
        }

        if (!ctxt)
          break;
        ctxt->step();
      }

      // halt outer context if it did not do so itself
      if (ctxt)
        stop();

      // clean-up
      for (const auto& addr : tx.destroy_list)
        gs.remove(addr);

      return result;
    }

  private:
    void push_context(
      const Address& caller,
      AccountState as,
      vector<uint8_t>&& input,
      Program&& prog,
      uint64_t call_value,
      Context::ReturnHandler&& rh,
      Context::HaltHandler&& hh,
      Context::ExceptionHandler&& eh)
    {
      if (get_call_depth() >= Consts::MAX_CALL_DEPTH)
        throw Exception(
          ET::outOfBounds,
          "Reached max call depth (" + to_string(Consts::MAX_CALL_DEPTH) + ")");

      auto c = make_unique<Context>(
        caller,
        as,
        move(input),
        call_value,
        move(prog),
        move(rh),
        move(hh),
        move(eh));
      ctxts.emplace_back(move(c));
      ctxt = ctxts.back().get();
    }

    uint16_t get_call_depth() const
    {
      return static_cast<uint16_t>(ctxts.size());
    }

    Opcode get_op() const
    {
      return static_cast<Opcode>(ctxt->prog.code[ctxt->get_pc()]);
    }

    uint256_t pop_addr(Stack& st)
    {
      static const uint256_t MASK_160 = (uint256_t(1) << 160) - 1;
      return st.pop() & MASK_160;
    }

    void pop_context()
    {
      ctxts.pop_back();
      if (!ctxts.empty())
        ctxt = ctxts.back().get();
      else
        ctxt = nullptr;
    }

    static void copy_mem_raw(
      const uint64_t offDst,
      const uint64_t offSrc,
      const uint64_t size,
      vector<uint8_t>& dst,
      const vector<uint8_t>& src,
      const uint8_t pad = 0)
    {
      if (!size)
        return;

      const auto lastDst = offDst + size;
      if (lastDst < offDst)
        throw Exception(
          ET::outOfBounds,
          "Integer overflow in copy_mem (" + to_string(lastDst) + " < " +
            to_string(offDst) + ")");

      if (lastDst > Consts::MAX_MEM_SIZE)
        throw Exception(
          ET::outOfBounds,
          "Memory limit exceeded (" + to_string(lastDst) + " > " +
            to_string(Consts::MAX_MEM_SIZE) + ")");

      if (lastDst > dst.size())
        dst.resize(lastDst);

      const auto lastSrc = offSrc + size;
      const auto endSrc =
        min(lastSrc, static_cast<decltype(lastSrc)>(src.size()));
      uint64_t remaining;
      if (endSrc > offSrc)
      {
        copy(src.begin() + offSrc, src.begin() + endSrc, dst.begin() + offDst);
        remaining = lastSrc - endSrc;
      }
      else
      {
        remaining = size;
      }

      // if there are more bytes to copy than available, add padding
      fill(dst.begin() + lastDst - remaining, dst.begin() + lastDst, pad);
    }

    void copy_mem(
      vector<uint8_t>& dst, const vector<uint8_t>& src, const uint8_t pad)
    {
      const auto offDst = ctxt->s.pop64();
      const auto offSrc = ctxt->s.pop64();
      const auto size = ctxt->s.pop64();

      copy_mem_raw(offDst, offSrc, size, dst, src, pad);
    }

    void prepare_mem_access(const uint64_t offset, const uint64_t size)
    {
      const auto end = offset + size;
      if (end < offset)
        throw Exception(
          ET::outOfBounds,
          "Integer overflow in memory access (" + to_string(end) + " < " +
            to_string(offset) + ")");

      if (end > Consts::MAX_MEM_SIZE)
        throw Exception(
          ET::outOfBounds,
          "Memory limit exceeded (" + to_string(end) + " > " +
            to_string(Consts::MAX_MEM_SIZE) + ")");

      if (end > ctxt->mem.size())
        ctxt->mem.resize(end);
    }

    vector<uint8_t> copy_from_mem(const uint64_t offset, const uint64_t size)
    {
      prepare_mem_access(offset, size);
      return {ctxt->mem.begin() + offset, ctxt->mem.begin() + offset + size};
    }

    void jump_to(const uint64_t newPc)
    {
      if (ctxt->prog.jump_dests.find(newPc) == ctxt->prog.jump_dests.end())
        throw Exception(
          ET::illegalInstruction,
          to_string(newPc) + " is not a jump destination");
      ctxt->set_pc(newPc);
    }

    template <
      typename X,
      typename Y,
      typename = enable_if_t<is_unsigned<X>::value && is_unsigned<Y>::value>>
    static auto safeAdd(const X x, const Y y)
    {
      const auto r = x + y;
      if (r < x)
        throw overflow_error("integer overflow");
      return r;
    }

    template <typename T>
    static T shrink(uint256_t i)
    {
      return static_cast<T>(i & numeric_limits<T>::max());
    }

    void dispatch()
    {
      const auto op = get_op();
      if (tr) // TODO: remove if from critical path
        tr->add(ctxt->get_pc(), op, get_call_depth(), ctxt->s);

      switch (op)
      {
        case Opcode::PUSH1:
        case Opcode::PUSH2:
        case Opcode::PUSH3:
        case Opcode::PUSH4:
        case Opcode::PUSH5:
        case Opcode::PUSH6:
        case Opcode::PUSH7:
        case Opcode::PUSH8:
        case Opcode::PUSH9:
        case Opcode::PUSH10:
        case Opcode::PUSH11:
        case Opcode::PUSH12:
        case Opcode::PUSH13:
        case Opcode::PUSH14:
        case Opcode::PUSH15:
        case Opcode::PUSH16:
        case Opcode::PUSH17:
        case Opcode::PUSH18:
        case Opcode::PUSH19:
        case Opcode::PUSH20:
        case Opcode::PUSH21:
        case Opcode::PUSH22:
        case Opcode::PUSH23:
        case Opcode::PUSH24:
        case Opcode::PUSH25:
        case Opcode::PUSH26:
        case Opcode::PUSH27:
        case Opcode::PUSH28:
        case Opcode::PUSH29:
        case Opcode::PUSH30:
        case Opcode::PUSH31:
        case Opcode::PUSH32:
          push();
          break;
        case Opcode::POP:
          pop();
          break;
        case Opcode::SWAP1:
        case Opcode::SWAP2:
        case Opcode::SWAP3:
        case Opcode::SWAP4:
        case Opcode::SWAP5:
        case Opcode::SWAP6:
        case Opcode::SWAP7:
        case Opcode::SWAP8:
        case Opcode::SWAP9:
        case Opcode::SWAP10:
        case Opcode::SWAP11:
        case Opcode::SWAP12:
        case Opcode::SWAP13:
        case Opcode::SWAP14:
        case Opcode::SWAP15:
        case Opcode::SWAP16:
          swap();
          break;
        case Opcode::DUP1:
        case Opcode::DUP2:
        case Opcode::DUP3:
        case Opcode::DUP4:
        case Opcode::DUP5:
        case Opcode::DUP6:
        case Opcode::DUP7:
        case Opcode::DUP8:
        case Opcode::DUP9:
        case Opcode::DUP10:
        case Opcode::DUP11:
        case Opcode::DUP12:
        case Opcode::DUP13:
        case Opcode::DUP14:
        case Opcode::DUP15:
        case Opcode::DUP16:
          dup();
          break;
        case Opcode::LOG0:
        case Opcode::LOG1:
        case Opcode::LOG2:
        case Opcode::LOG3:
        case Opcode::LOG4:
          log();
          break;
        case Opcode::ADD:
          add();
          break;
        case Opcode::MUL:
          mul();
          break;
        case Opcode::SUB:
          sub();
          break;
        case Opcode::DIV:
          div();
          break;
        case Opcode::SDIV:
          sdiv();
          break;
        case Opcode::MOD:
          mod();
          break;
        case Opcode::SMOD:
          smod();
          break;
        case Opcode::ADDMOD:
          addmod();
          break;
        case Opcode::MULMOD:
          mulmod();
          break;
        case Opcode::EXP:
          exp();
          break;
        case Opcode::SIGNEXTEND:
          signextend();
          break;
        case Opcode::LT:
          lt();
          break;
        case Opcode::GT:
          gt();
          break;
        case Opcode::SLT:
          slt();
          break;
        case Opcode::SGT:
          sgt();
          break;
        case Opcode::EQ:
          eq();
          break;
        case Opcode::ISZERO:
          isZero();
          break;
        case Opcode::AND:
          and_();
          break;
        case Opcode::OR:
          or_();
          break;
        case Opcode::XOR:
          xor_();
          break;
        case Opcode::NOT:
          not_();
          break;
        case Opcode::BYTE:
          byte();
          break;
        case Opcode::JUMP:
          jump();
          break;
        case Opcode::JUMPI:
          jumpi();
          break;
        case Opcode::PC:
          pc();
          break;
        case Opcode::MSIZE:
          msize();
          break;
        case Opcode::MLOAD:
          mload();
          break;
        case Opcode::MSTORE:
          mstore();
          break;
        case Opcode::MSTORE8:
          mstore8();
          break;
        case Opcode::CODESIZE:
          codesize();
          break;
        case Opcode::CODECOPY:
          codecopy();
          break;
        case Opcode::EXTCODESIZE:
          extcodesize();
          break;
        case Opcode::EXTCODECOPY:
          extcodecopy();
          break;
        case Opcode::SLOAD:
          sload();
          break;
        case Opcode::SSTORE:
          sstore();
          break;
        case Opcode::ADDRESS:
          address();
          break;
        case Opcode::BALANCE:
          balance();
          break;
        case Opcode::ORIGIN:
          origin();
          break;
        case Opcode::CALLER:
          caller();
          break;
        case Opcode::CALLVALUE:
          callvalue();
          break;
        case Opcode::CALLDATALOAD:
          calldataload();
          break;
        case Opcode::CALLDATASIZE:
          calldatasize();
          break;
        case Opcode::CALLDATACOPY:
          calldatacopy();
          break;
        case Opcode::RETURN:
          return_();
          break;
        case Opcode::DESTROY:
          destroy();
          break;
        case Opcode::CREATE:
          create();
          break;
        case Opcode::CALL:
        case Opcode::CALLCODE:
        case Opcode::DELEGATECALL:
          call();
          break;
        case Opcode::JUMPDEST:
          jumpdest();
          break;
        case Opcode::BLOCKHASH:
          blockhash();
          break;
        case Opcode::NUMBER:
          number();
          break;
        case Opcode::GASPRICE:
          gasprice();
          break;
        case Opcode::COINBASE:
          coinbase();
          break;
        case Opcode::TIMESTAMP:
          timestamp();
          break;
        case Opcode::DIFFICULTY:
          difficulty();
          break;
        case Opcode::GASLIMIT:
          gaslimit();
          break;
        case Opcode::GAS:
          gas();
          break;
        case Opcode::SHA3:
          sha3();
          break;
        case Opcode::STOP:
          stop();
          break;
        default:
          stringstream err;
          err << "unknown/unsupported Opcode: 0x" << hex << int{get_op()}
              << endl;
          err << dec << " seen at position " << ctxt->get_pc() << " in "
              << to_hex_str(ctxt->as.acc.address) << ", at call-depth "
              << get_call_depth() << " called by " << to_hex_str(ctxt->caller);
          throw Exception(Exception::Type::illegalInstruction, err.str());
      };
    }

    //
    // op codes
    //
    void swap()
    {
      ctxt->s.swap(get_op() - SWAP1 + 1);
    }

    void dup()
    {
      ctxt->s.dup(get_op() - DUP1);
    }

    void add()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x + y);
    }

    void sub()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x - y);
    }

    void mul()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x * y);
    }

    void div()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (!y)
      {
        ctxt->s.push(0);
      }
      else
      {
        ctxt->s.push(x / y);
      }
    }

    void sdiv()
    {
      auto x = ctxt->s.pop();
      auto y = ctxt->s.pop();
      const auto min = (numeric_limits<uint256_t>::max() / 2) + 1;

      if (y == 0)
        ctxt->s.push(0);
      // special "overflow case" from the yellow paper
      else if (x == min && y == -1)
        ctxt->s.push(x);
      else
      {
        const auto signX = get_sign(x);
        const auto signY = get_sign(y);
        if (signX == -1)
          x = 0 - x;
        if (signY == -1)
          y = 0 - y;
        ctxt->s.push((x / y) * signX * signY);
      }
    }

    void mod()
    {
      const auto x = ctxt->s.pop();
      const auto m = ctxt->s.pop();
      if (!m)
        ctxt->s.push(0);
      else
        ctxt->s.push(x % m);
    }

    void smod()
    {
      auto x = ctxt->s.pop();
      auto m = ctxt->s.pop();
      if (m == 0)
        ctxt->s.push(0);
      else
      {
        const auto signX = get_sign(x);
        const auto signM = get_sign(m);
        if (signX == -1)
          x = 0 - x;
        if (signM == -1)
          m = 0 - m;
        ctxt->s.push((x % m) * signX);
      }
    }

    void addmod()
    {
      const uint512_t x = ctxt->s.pop();
      const uint512_t y = ctxt->s.pop();
      const auto m = ctxt->s.pop();
      if (!m)
        ctxt->s.push(0);
      else
        ctxt->s.push(static_cast<uint256_t>((x + y) % m));
    }

    void mulmod()
    {
      const uint512_t x = ctxt->s.pop();
      const uint512_t y = ctxt->s.pop();
      const auto m = ctxt->s.pop();
      if (!m)
        ctxt->s.push(m);
      else
        ctxt->s.push(static_cast<uint256_t>((x * y) % m));
    }

    void exp()
    {
      const auto b = ctxt->s.pop();
      const auto e = ctxt->s.pop64();
      ctxt->s.push(power(b, e));
    }

    void signextend()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (x >= 32)
      {
        ctxt->s.push(y);
        return;
      }
      const uint8_t idx = 8 * shrink<uint8_t>(x) + 7;
      const auto sign = static_cast<uint8_t>((y >> idx) & 1);
      const auto mask = uint256_t(-1) >> (256 - idx);
      const auto yex = (uint256_t(-sign) << idx) | (y & mask);
      ctxt->s.push(yex);
    }

    void lt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push((x < y) ? 1 : 0);
    }

    void gt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push((x > y) ? 1 : 0);
    }

    void slt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (x == y)
      {
        ctxt->s.push(0);
        return;
      }

      uint8_t signX = x.sign();
      uint8_t signY = y.sign();
      if (signX != signY)
      {
        if (signX == 1)
          ctxt->s.push(1);
        else
          ctxt->s.push(0);
      }
      else
      {
        ctxt->s.push((x < y) ? 1 : 0);
      }
    }

    void sgt()
    {
      ctxt->s.swap(1);
      slt();
    }

    void eq()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (x == y)
        ctxt->s.push(1);
      else
        ctxt->s.push(0);
    }

    void isZero()
    {
      const auto x = ctxt->s.pop();
      if (x == 0)
        ctxt->s.push(1);
      else
        ctxt->s.push(0);
    }

    void and_()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x & y);
    }

    void or_()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x | y);
    }

    void xor_()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x ^ y);
    }

    void not_()
    {
      const auto x = ctxt->s.pop();
      ctxt->s.push(~x);
    }

    void byte()
    {
      const auto idx = ctxt->s.pop();
      if (idx >= 32)
      {
        ctxt->s.push(0);
        return;
      }
      const auto shift = 256 - 8 - 8 * shrink<uint8_t>(idx);
      const auto mask = uint256_t(255) << shift;
      const auto val = ctxt->s.pop();
      ctxt->s.push((val & mask) >> shift);
    }

    void jump()
    {
      const auto newPc = ctxt->s.pop64();
      jump_to(newPc);
    }

    void jumpi()
    {
      const auto newPc = ctxt->s.pop64();
      const auto cond = ctxt->s.pop();
      if (cond)
        jump_to(newPc);
    }

    void jumpdest() {}

    void pc()
    {
      ctxt->s.push(ctxt->get_pc());
    }

    void msize()
    {
      ctxt->s.push(ctxt->get_used_mem() * 32);
    }

    void mload()
    {
      const auto offset = ctxt->s.pop64();
      prepare_mem_access(offset, Consts::WORD_SIZE);
      const auto start = ctxt->mem.begin() + offset;
      ctxt->s.push(from_big_endian(start, start + Consts::WORD_SIZE));
    }

    void mstore()
    {
      const auto offset = ctxt->s.pop64();
      const auto word = ctxt->s.pop();
      prepare_mem_access(offset, Consts::WORD_SIZE);
      to_big_endian(word, ctxt->mem.data() + offset);
    }

    void mstore8()
    {
      const auto offset = ctxt->s.pop64();
      const auto b = shrink<uint8_t>(ctxt->s.pop());
      prepare_mem_access(offset, sizeof(b));
      ctxt->mem[offset] = b;
    }

    void sload()
    {
      const auto k = ctxt->s.pop();
      ctxt->s.push(ctxt->st.load(k));
    }

    void sstore()
    {
      const auto k = ctxt->s.pop();
      const auto v = ctxt->s.pop();
      if (!v)
        ctxt->st.remove(k);
      else
        ctxt->st.store(k, v);
    }

    void codecopy()
    {
      copy_mem(ctxt->mem, ctxt->prog.code, Opcode::STOP);
    }

    void extcodesize()
    {
      ctxt->s.push(gs.get(pop_addr(ctxt->s)).acc.code.size());
    }

    void extcodecopy()
    {
      copy_mem(ctxt->mem, gs.get(pop_addr(ctxt->s)).acc.code, Opcode::STOP);
    }

    void codesize()
    {
      ctxt->s.push(ctxt->acc.code.size());
    }

    void calldataload()
    {
      const auto offset = ctxt->s.pop64();
      safeAdd(offset, Consts::WORD_SIZE);
      const auto sizeInput = ctxt->input.size();

      uint256_t v = 0;
      for (uint8_t i = 0; i < Consts::WORD_SIZE; i++)
      {
        const auto j = offset + i;
        if (j < sizeInput)
        {
          v = (v << 8) + ctxt->input[j];
        }
        else
        {
          v <<= 8 * (Consts::WORD_SIZE - i);
          break;
        }
      }
      ctxt->s.push(v);
    }

    void calldatasize()
    {
      ctxt->s.push(ctxt->input.size());
    }

    void calldatacopy()
    {
      copy_mem(ctxt->mem, ctxt->input, 0);
    }

    void address()
    {
      ctxt->s.push(ctxt->acc.address);
    }

    void balance()
    {
      decltype(auto) acc = gs.get(pop_addr(ctxt->s)).acc;
      ctxt->s.push(acc.balance);
    }

    void origin()
    {
      ctxt->s.push(tx.origin);
    }

    void caller()
    {
      ctxt->s.push(ctxt->caller);
    }

    void callvalue()
    {
      ctxt->s.push(ctxt->call_value);
    }

    void push()
    {
      const uint8_t bytes = get_op() - PUSH1 + 1;
      const auto end = ctxt->get_pc() + bytes;
      if (end < ctxt->get_pc())
        throw Exception(
          ET::outOfBounds,
          "Integer overflow in push (" + to_string(end) + " < " +
            to_string(ctxt->get_pc()) + ")");

      if (end >= ctxt->prog.code.size())
        throw Exception(
          ET::outOfBounds,
          "Push immediate exceeds size of program (" + to_string(end) +
            " >= " + to_string(ctxt->prog.code.size()) + ")");

      // TODO: parse immediate once and not every time
      auto pc = ctxt->get_pc() + 1;
      uint256_t imm = 0;
      for (int i = 0; i < bytes; i++)
        imm = (imm << 8) | ctxt->prog.code[pc++];

      ctxt->s.push(imm);
      ctxt->set_pc(pc);
    }

    void pop()
    {
      ctxt->s.pop();
    }

    void log()
    {
      const uint8_t n = get_op() - LOG0;
      const auto offset = ctxt->s.pop64();
      const auto size = ctxt->s.pop64();

      vector<uint256_t> topics(n);
      for (int i = 0; i < n; i++)
        topics[i] = ctxt->s.pop();

      tx.log_handler.handle(
        {ctxt->acc.address, copy_from_mem(offset, size), topics});
    }

    void blockhash()
    {
      const auto i = ctxt->s.pop64();
      if (i >= 256)
        ctxt->s.push(0);
      else
        ctxt->s.push(gs.get_block_hash(i));
    }

    void number()
    {
      ctxt->s.push(gs.get_current_block().number);
    }

    void gasprice()
    {
      ctxt->s.push(tx.gas_price);
    }

    void coinbase()
    {
      ctxt->s.push(gs.get_current_block().coinbase);
    }

    void timestamp()
    {
      ctxt->s.push(gs.get_current_block().timestamp);
    }

    void difficulty()
    {
      ctxt->s.push(gs.get_current_block().difficulty);
    }

    void gas()
    {
      // NB: we do not currently track gas. This will always return the tx's
      // initial gas value
      ctxt->s.push(tx.gas_limit);
    }

    void gaslimit()
    {
      ctxt->s.push(tx.gas_limit);
    }

    void sha3()
    {
      const auto offset = ctxt->s.pop64();
      const auto size = ctxt->s.pop64();
      prepare_mem_access(offset, size);

      uint8_t h[32];
      Keccak_256(ctxt->mem.data() + offset, static_cast<unsigned int>(size), h);
      ctxt->s.push(from_big_endian(h, h + sizeof(h)));
    }

    void return_()
    {
      const auto offset = ctxt->s.pop64();
      const auto size = ctxt->s.pop64();

      // invoke caller's return handler
      ctxt->rh(copy_from_mem(offset, size));
      pop_context();
    }

    void stop()
    {
      // (1) save halt handler
      auto hh = ctxt->hh;
      // (2) pop current context
      pop_context();
      // (3) invoke halt handler
      hh();
    }

    void destroy()
    {
      gs.get(pop_addr(ctxt->s)).acc.balance += ctxt->acc.balance;
      tx.destroy_list.push_back(ctxt->acc.address);
      stop();
    }

    void create()
    {
      const auto contractValue = ctxt->s.pop();
      const auto offset = ctxt->s.pop64();
      const auto size = ctxt->s.pop64();

      ctxt->acc.pay(contractValue);
      const auto initCode = copy_from_mem(offset, size);

      const auto newAddress =
        generate_address(ctxt->acc.address, ctxt->acc.nonce);
      decltype(auto) newAcc = gs.create(newAddress, contractValue, {});

      auto parentContext = ctxt;
      auto rh = [&newAcc, parentContext](vector<uint8_t> output) {
        newAcc.acc.set_code(move(output));
        parentContext->s.push(newAcc.acc.address);
      };
      auto hh = [parentContext]() { parentContext->s.push(0); };
      auto eh = [parentContext](const Exception&) { parentContext->s.push(0); };

      // create new context for init code execution
      push_context(ctxt->acc.address, newAcc, {}, initCode, 0, rh, hh, eh);
    }

    void call()
    {
      const auto op = get_op();
      ctxt->s.pop(); // gas limit not used
      const auto addr = pop_addr(ctxt->s);
      const auto value = op == DELEGATECALL ? 0 : ctxt->s.pop64();
      const auto offIn = ctxt->s.pop64();
      const auto sizeIn = ctxt->s.pop64();
      const auto offOut = ctxt->s.pop64();
      const auto sizeOut = ctxt->s.pop64();

      if (addr >= 1 && addr <= 4)
      {
        // TODO: implement native extensions
        throw Exception(
          ET::notImplemented, "Native extensions are not available.");
      }

      decltype(auto) callee = gs.get(addr);
      ctxt->acc.pay(callee.acc, value);
      if (!callee.acc.has_code())
      {
        ctxt->s.push(1);
        return;
      }

      prepare_mem_access(offOut, sizeOut);
      auto input = copy_from_mem(offIn, sizeIn);

      auto parentContext = ctxt;
      auto rh =
        [offOut, sizeOut, parentContext](const vector<uint8_t>& output) {
          copy_mem_raw(offOut, 0, sizeOut, parentContext->mem, output);
          parentContext->s.push(1);
        };
      auto hh = [parentContext]() { parentContext->s.push(0); };
      auto he = [parentContext](const Exception&) { parentContext->s.push(0); };

      switch (op)
      {
        case Opcode::CALL:
          push_context(
            ctxt->acc.address,
            callee,
            move(input),
            callee.acc.code,
            value,
            rh,
            hh,
            he);
          break;
        case Opcode::CALLCODE:
          push_context(
            ctxt->acc.address,
            ctxt->as,
            move(input),
            callee.acc.code,
            value,
            rh,
            hh,
            he);
          break;
        case Opcode::DELEGATECALL:
          push_context(
            ctxt->caller,
            ctxt->as,
            move(input),
            callee.acc.code,
            ctxt->call_value,
            rh,
            hh,
            he);
          break;
        default:
          throw UnexpectedState("Unknown call opcode.");
      }
    }
  };

  Processor::Processor(GlobalState& gs) : gs(gs) {}

  ExecResult Processor::run(
    Transaction& tx,
    const Address& caller,
    AccountState callee,
    const vector<uint8_t>& input,
    uint64_t call_value,
    Trace* tr)
  {
    return _Processor(gs, tx, tr).run(caller, callee, input, call_value);
  }
} // namespace evm
