/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_PHV_SOLVER_SYMBOLIC_BITVEC_H_
#define BF_P4C_PHV_SOLVER_SYMBOLIC_BITVEC_H_

#include <sstream>
#include <vector>

#include "lib/bitvec.h"
#include "lib/cstring.h"

namespace solver {

namespace symbolic_bitvec {

using namespace P4;

/// BitID encodes a symbolic bit
/// < 0  illegal
///   0  represents the const zero.
///   1  represents the const one.
/// > 1  represents a boolean variable.
using BitID = int;

enum ExprNodeType { BIT_NODE, AND_NODE, OR_NODE, NEG_NODE };

class Expr {
 public:
    const Expr *left;
    const Expr *right;
    ExprNodeType type;
    BitID value;

    Expr(const Expr *left, const Expr *right, ExprNodeType type, BitID value = -1);
    Expr(ExprNodeType type, BitID value, const Expr *left = nullptr, const Expr *right = nullptr);
    bool eq(const Expr *other) const;
    cstring to_cstring() const;

 private:
    void simplify();
};

// Bitvec is a vector of Bits. The size is fixed after construction.
class BitVec {
 private:
    std::vector<const Expr *> bits;

 private:
    void size_check(const BitVec &other) const;

    BitVec bin_op(const BitVec &other, ExprNodeType type) const;

 public:
    explicit BitVec(std::vector<const Expr *> &bits) : bits(bits) {}
    void set(int i, bool value) { bits[i] = new Expr(ExprNodeType::BIT_NODE, int(value)); }
    const Expr *get(int i) const { return bits[i]; }
    BitVec slice(int start, int sz) const;
    bool eq(const BitVec &other) const;
    cstring to_cstring() const;
    BitVec bv_and(const BitVec &other) const;
    BitVec bv_or(const BitVec &other) const;
    BitVec bv_neg() const;
    BitVec rotate_right(int amount) const;
    BitVec rotate_left(int amount) const;
    bool operator==(const BitVec &other) const { return eq(other); }
    bool operator!=(const BitVec &other) const { return !eq(other); }
    BitVec operator<<(int v) const { return rotate_left(v); }
    BitVec operator>>(int v) const { return rotate_right(v); }
    BitVec operator~() const { return bv_neg(); }
    BitVec operator&(const BitVec &other) const { return bv_and(other); }
    BitVec operator|(const BitVec &other) const { return bv_or(other); }
};

// BvContext maintains a context for symbolic bit vectors and bit expressions created.
// Expressions and BitVecs are comparable only if they were created by the same context.
class BvContext {
 private:
    BitID uid = 1;  // 0 and 1 are reserved for const zero and one.
    BitID new_uid() { return ++uid; };

 public:
    BitVec new_bv(int sz);
    BitVec new_bv_const(int sz, bitvec val);
};

}  // namespace symbolic_bitvec

}  // namespace solver

#endif /* BF_P4C_PHV_SOLVER_SYMBOLIC_BITVEC_H_ */
