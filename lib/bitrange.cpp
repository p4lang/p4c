// Copyright 2024 Intel Corp.
// SPDX-FileCopyrightText: 2024 Intel Corp.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/bitrange.h"

#include <iostream>
#include <utility>

namespace P4 {

std::ostream &toStream(std::ostream &out, RangeUnit unit, Endian order, int lo, int hi,
                       bool closed) {
    if (unit == RangeUnit::Bit)
        out << "bit";
    else if (unit == RangeUnit::Byte)
        out << "byte";
    else
        BUG("unknown range unit");

    out << (!closed && order == Endian::Little ? "(" : "[");

    if (order == Endian::Little) std::swap(lo, hi);

    out << std::dec << lo;
    if (lo != hi) out << ".." << hi;

    out << (!closed && order == Endian::Network ? ")" : "]");

    return out;
}

}  // namespace P4
