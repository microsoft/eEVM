// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <intx/intx.hpp>

namespace intx
{
div_result<uint256> udiv_qr_knuth_hd_base(const uint256& x, const uint256& y);
div_result<uint256> udiv_qr_knuth_llvm_base(const uint256& u, const uint256& v);
div_result<uint256> udiv_qr_knuth_opt_base(const uint256& x, const uint256& y);
div_result<uint256> udiv_qr_knuth_opt(const uint256& x, const uint256& y);
div_result<uint256> udiv_qr_knuth_64(const uint256& x, const uint256& y);
div_result<uint512> udiv_qr_knuth_512(const uint512& x, const uint512& y);
div_result<uint512> udiv_qr_knuth_512_64(const uint512& x, const uint512& y);
}  // namespace intx
