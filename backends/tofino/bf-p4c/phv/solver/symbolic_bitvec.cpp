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

#include "phv/solver/symbolic_bitvec.h"

#include "lib/exceptions.h"

namespace solver {

namespace symbolic_bitvec {

using namespace P4;

Expr::Expr(const Expr *left, const Expr *right, ExprNodeType type, BitID value)
    : left{left}, right{right}, type{type}, value{value} {
    this->simplify();
}

Expr::Expr(ExprNodeType type, BitID value, const Expr *left, const Expr *right)
    : left{left}, right{right}, type{type}, value{value} {
    this->simplify();
}

void Expr::simplify() {
    switch (type) {
        case ExprNodeType::BIT_NODE:
            break;
        case ExprNodeType::AND_NODE:
            if (left->type == ExprNodeType::BIT_NODE && (left->value == 0 || left->value == 1)) {
                if (left->value == 0) {
                    // One child is 0, so remove subtree and set self to 0
                    this->type = ExprNodeType::BIT_NODE;
                    this->value = 0;
                    this->right = nullptr;
                    this->left = nullptr;
                } else if (left->value == 1) {
                    // Left is 1, so set iself to right child
                    this->type = right->type;
                    this->value = right->value;
                    this->left = this->right->left;
                    this->right = this->right->right;
                }
            } else if (right->type == ExprNodeType::BIT_NODE) {
                if (right->value == 0) {
                    // One child is 0, so remove subtree and set self to 0
                    this->type = ExprNodeType::BIT_NODE;
                    this->value = 0;
                    this->right = nullptr;
                    this->left = nullptr;
                } else if (right->value == 1) {
                    // Right is 1, so set iself to left child
                    this->type = left->type;
                    this->value = this->left->value;
                    this->right = this->left->right;
                    this->left = this->left->left;
                }
            }
            break;
        case ExprNodeType::OR_NODE:
            if (left->type == ExprNodeType::BIT_NODE && (left->value == 0 || left->value == 1)) {
                if (left->value == 1) {
                    // One child is 1, so remove subtree and set self to 1
                    this->type = ExprNodeType::BIT_NODE;
                    this->value = 1;
                    this->right = nullptr;
                    this->left = nullptr;
                } else if (left->value == 0) {
                    // Left is 0, so set iself to right child
                    this->type = right->type;
                    this->value = this->right->value;
                    this->left = this->right->left;
                    this->right = this->right->right;
                }
            } else if (right->type == ExprNodeType::BIT_NODE) {
                if (right->value == 1) {
                    // One child is 1, so remove subtree and set self to 1
                    this->type = ExprNodeType::BIT_NODE;
                    this->value = 1;
                    this->right = nullptr;
                    this->left = nullptr;
                } else if (right->value == 0) {
                    // Right is 0, so set iself to left child
                    this->type = left->type;
                    this->value = this->left->value;
                    this->right = this->left->right;
                    this->left = this->left->left;
                }
            }
            break;
        case ExprNodeType::NEG_NODE:
            if (left->type == ExprNodeType::BIT_NODE) {
                if (left->value == 1) {
                    this->type = ExprNodeType::BIT_NODE;
                    this->value = 0;
                    this->right = nullptr;
                    this->left = nullptr;
                } else if (left->value == 0) {
                    this->type = ExprNodeType::BIT_NODE;
                    this->value = 1;
                    this->right = nullptr;
                    this->left = nullptr;
                }
            }
            break;
    }
}

bool Expr::eq(const Expr *other) const {
    if (type != other->type) {
        return false;
    } else if (type == ExprNodeType::BIT_NODE) {
        return value == other->value;
    } else if (type == ExprNodeType::NEG_NODE) {
        return left->eq(other->left);
    }

    return (left->eq(other->left) && right->eq(other->right)) ||
           (left->eq(other->right) && right->eq(other->left));
}

cstring Expr::to_cstring() const {
    std::stringstream ss;
    switch (type) {
        case ExprNodeType::AND_NODE:
            ss << "(And " << left->to_cstring() << " " << right->to_cstring() << ")";
            break;
        case ExprNodeType::OR_NODE:
            ss << "(Or " << left->to_cstring() << " " << right->to_cstring() << ")";
            break;
        case ExprNodeType::NEG_NODE:
            ss << "(Neg " << left->to_cstring() << ")";
            break;
        default:
            if (value == 0) {
                ss << "0";
            } else if (value == 1) {
                ss << "1";
            } else {
                ss << "{" << value << "}";
            }
            break;
    }
    return ss.str();
}

void BitVec::size_check(const BitVec &other) const {
    BUG_CHECK(other.bits.size() == bits.size(), "BitVec size mismatch");
}

BitVec BitVec::bin_op(const BitVec &other, ExprNodeType type) const {
    size_check(other);
    std::vector<const Expr *> rst(bits.size());
    for (size_t i = 0; i < bits.size(); i++) {
        rst[i] = new Expr(bits[i], other.bits[i], type);
    }
    return BitVec{rst};
}

cstring BitVec::to_cstring() const {
    std::stringstream ss;
    ss << "[";
    std::string sep = "";
    for (size_t i = 0; i < bits.size(); i++) {
        ss << sep;
        sep = ", ";
        ss << bits[i]->to_cstring() << "@" << i;
    }
    ss << "]";
    return ss.str();
}

BitVec BitVec::slice(int start, int sz) const {
    BUG_CHECK(start >= 0 && start < int(bits.size()), "invalid start: %1%");
    BUG_CHECK(start + sz <= int(bits.size()), "invalid sz: %1%");
    auto s = std::vector<const Expr *>(bits.begin() + start, bits.begin() + start + sz);
    return BitVec{s};
}

bool BitVec::eq(const BitVec &other) const {
    BUG_CHECK(bits.size() == other.bits.size(), "BitVec size mismatch: %1% != %2%", bits.size(),
              other.bits.size());
    for (size_t i = 0; i < bits.size(); i++) {
        if (!bits[i]->eq(other.bits[i])) {
            return false;
        }
    }
    return true;
}

BitVec BitVec::bv_and(const BitVec &other) const { return bin_op(other, ExprNodeType::AND_NODE); }

BitVec BitVec::bv_or(const BitVec &other) const { return bin_op(other, ExprNodeType::OR_NODE); }

BitVec BitVec::bv_neg() const {
    std::vector<const Expr *> rst(bits.size());
    for (size_t i = 0; i < bits.size(); i++) {
        rst[i] = new Expr(bits[i], nullptr, ExprNodeType::NEG_NODE);
    }
    return BitVec{rst};
}

BitVec BitVec::rotate_right(int amount) const {
    // It can work with negative shift amount. Starting from C++11, negative module
    // rounding to 0 is a definied behavior, e.g. (-3) % 7 = -3, different from python.
    if (bits.size() == 0) return *this;
    amount %= bits.size();
    std::vector<const Expr *> rst(bits.size());
    for (size_t i = 0; i < bits.size(); i++) {
        int new_i = i - amount;
        if (new_i >= int(bits.size())) {
            new_i -= bits.size();
        } else if (new_i < 0) {
            new_i += bits.size();
        }
        rst[new_i] = bits[i];
    }
    return BitVec{rst};
}

BitVec BitVec::rotate_left(int amount) const { return rotate_right(-amount); }

BitVec BvContext::new_bv(int sz) {
    std::vector<const Expr *> bits(sz);
    for (int i = 0; i < sz; i++) {
        bits[i] = new Expr(ExprNodeType::BIT_NODE, new_uid());
    }
    return BitVec(bits);
}

BitVec BvContext::new_bv_const(int sz, bitvec val) {
    std::vector<const Expr *> bits(sz);
    for (int i = 0; i < sz; i++) {
        int bit_val = int(val.getbit(i));
        bits[i] = new Expr(ExprNodeType::BIT_NODE, bit_val);
    }
    return BitVec(bits);
}

}  // namespace symbolic_bitvec

}  // namespace solver
