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

#ifndef BF_P4C_MAU_ASM_FORMAT_HASH_H_
#define BF_P4C_MAU_ASM_FORMAT_HASH_H_

#include <iterator>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/mau/asm_output.h"
#include "bf-p4c/phv/asm_output.h"
#include "boost/range/adaptor/reversed.hpp"
#include "lib/algorithm.h"
#include "lib/bitops.h"
#include "lib/bitrange.h"
#include "lib/hex.h"
#include "lib/indent.h"
#include "lib/stringref.h"

using namespace P4;

struct FormatHash {
    using RangeOfConstant = std::map<le_bitrange, const IR::Constant *>;
    const safe_vector<Slice> *match_data;
    const std::multimap<int, Slice> *match_data_map;
    const RangeOfConstant *constant_map;
    const Slice *ghost;
    IR::MAU::HashFunction func;
    int total_bits = 0;
    le_bitrange *field_range;

    FormatHash(const safe_vector<Slice> *md, const std::multimap<int, Slice> *mdm,
               const std::map<le_bitrange, const IR::Constant *> *cm, const Slice *g,
               IR::MAU::HashFunction f, int tb = 0, le_bitrange *fr = nullptr)
        : match_data(md),
          match_data_map(mdm),
          constant_map(cm),
          ghost(g),
          func(f),
          total_bits(tb),
          field_range(fr) {
        BUG_CHECK(match_data == nullptr || match_data_map == nullptr,
                  "FormatHash not "
                  "configured correctly");
    }

    // functors for formatting expressions as hash expression in bfa.  FIXME -- Should be
    // folded in to operator<< or as non-static methods?  Problem is we need dynamic
    // dispatch on IR types, which can only be done with visitors (or messy dynamic_cast
    // chains)  So we just define them as nested classes here.
    class SliceWidth;  // find maximum slice width that can output as one line
    class ZeroHash;    // true iff slice contributes nothing to hash table (all 0s)
    class Output;      // output a .bfa expression for a slice of a hash expression
};

inline std::ostream &operator<<(std::ostream &out, const FormatHash &hash) {
    if (hash.field_range != nullptr) {
        FormatHash hash2(hash.match_data, hash.match_data_map, hash.constant_map, hash.ghost,
                         hash.func, hash.total_bits);
        out << "slice(" << hash2 << ", " << hash.field_range->lo << "..";
        out << hash.field_range->hi << ")";
        return out;
    }

    if (hash.func.type == IR::MAU::HashFunction::IDENTITY) {
        BUG_CHECK(hash.match_data, "For an identity, must be a standard vector");
        out << "stripe(" << emit_vector(*hash.match_data) << ")";
    } else if (hash.func.type == IR::MAU::HashFunction::RANDOM) {
        BUG_CHECK(hash.match_data, "For a random, must be a standard vector");
        if (!hash.match_data->empty()) {
            out << "random(" << emit_vector(*hash.match_data, ", ") << ")";
            if (hash.ghost) out << " ^ ";
        }
        if (hash.ghost) {
            out << *hash.ghost;
        }
    } else if (hash.func.type == IR::MAU::HashFunction::CRC) {
        BUG_CHECK(hash.match_data_map, "For a crc, must be a map");
        out << "stripe(crc";
        if (hash.func.reverse) out << "_rev";
        out << "(0x" << hex(hash.func.poly) << ", ";
        out << "0x" << hex(hash.func.init) << ", ";
        out << "0x" << hex(hash.func.final_xor) << ", ";
        out << hash.total_bits << ", ";
        out << *hash.match_data_map;
        // could not use the operator<< on std::map because the le_bitrange
        // does not print to valid json range.
        if (hash.constant_map) {
            out << ", {";
            const char *sep = " ";
            for (const auto &kv : *hash.constant_map) {
                out << sep << kv.first.lo << ".." << kv.first.hi << ": " << kv.second;
                sep = ", ";
            }
            out << " }";
        }
        out << ")";
        // FIXME -- final_xor needs to go into the seed for the hash group
        out << ")";
    } else if (hash.func.type == IR::MAU::HashFunction::XOR) {
        /* -- note: constants are computed by the midend into the seed value.
         *    There is no need to pass them through the xor directive. */
        BUG_CHECK(hash.match_data_map, "For a xor, must be a map");
        out << "stripe(xor(" << "0x" << hex(hash.func.size) << ", " << *hash.match_data_map << "))";
    } else if (hash.func.type == IR::MAU::HashFunction::CSUM) {
        BUG("csum hashing algorithm not supported");
    } else {
        BUG("unknown hashing algorithm %d", hash.func.type);
    }
    return out;
}

/* Find the maximum width of a slice starting from bit that can be output as a single
 * expression in a hash table in a .bfa file
 */
class FormatHash::SliceWidth : public Inspector {
    const PhvInfo &phv;
    safe_vector<Slice> &match_data;
    int bit;
    int width;
    bool preorder(const IR::Annotation *) { return false; }
    bool preorder(const IR::Type *) { return false; }
    bool preorder(const IR::BXor *) { return true; }
    bool preorder(const IR::BAnd *) { return true; }
    bool preorder(const IR::BOr *) { return true; }
    bool preorder(const IR::Constant *c) {
        if (getParent<IR::BOr>()) {
            // can't do OR in the .bfa -- need to slice based on bit ranges
            big_int v = c->value >> bit;
            if (v != 0) {
                int b(v & 1);
                int w = 1;
                while (((v >> w) & 1) == b) ++w;
                if (width > w) width = w;
            }
        }
        return false;
    }
    bool preorder(const IR::Concat *e) {
        if (bit < e->right->type->width_bits()) {
            if (width > e->right->type->width_bits() - bit)
                width = e->right->type->width_bits() - bit;
            visit(e->right, "right");
        } else {
            int tmp = bit;
            bit -= e->right->type->width_bits();
            BUG_CHECK(bit < e->left->type->width_bits(), "bit out of range in SliceWidth");
            if (width > e->left->type->width_bits() - bit)
                width = e->left->type->width_bits() - bit;
            visit(e->left, "left");
            bit = tmp;
        }
        return false;
    }
    bool preorder(const IR::ListExpression *fl) {
        int tmp = bit;
        for (auto *e : boost::adaptors::reverse(fl->components)) {
            if (bit < e->type->width_bits()) {
                if (width > e->type->width_bits() - bit) width = e->type->width_bits() - bit;
                visit(e);
                break;
            }
            bit -= e->type->width_bits();
        }
        bit = tmp;
        return false;
    }
    bool preorder(const IR::StructExpression *sl) {
        int tmp = bit;
        for (auto *e : boost::adaptors::reverse(sl->components)) {
            if (bit < e->expression->type->width_bits()) {
                if (width > e->expression->type->width_bits() - bit)
                    width = e->expression->type->width_bits() - bit;
                visit(e->expression);
                break;
            }
            bit -= e->expression->type->width_bits();
        }
        bit = tmp;
        return false;
    }
    bool preorder(const IR::BFN::SignExtend *e) {
        int w = e->type->width_bits() - bit;
        if (width > w) width = w;
        w = width;
        visit(e->expr, "e");
        if (width < w && width >= e->expr->type->width_bits()) width = w;
        return false;
    }
    bool preorder(const IR::Expression *e) {
        int w = e->type->width_bits() - bit;
        if (width > w) width = w;
        Slice sl(phv, e, bit, bit + width - 1);
        BUG_CHECK(sl, "Invalid expression %s in FormatHash::SliceWidth", e);
        for (auto &md : match_data) {
            if (auto tmp = sl & md) {
                if (tmp.get_lo() > sl.get_lo()) {
                    w = tmp.get_lo() - sl.get_lo();
                    if (width > w) width = w;
                    continue;
                }
                if (width > tmp.width()) width = tmp.width();
                break;
            }
        }
        return false;
    }

 public:
    SliceWidth(const PhvInfo &p, const IR::Expression *e, int b, safe_vector<Slice> &md)
        : phv(p), match_data(md), bit(b), width(e->type->width_bits() - b) {
        e->apply(*this);
    }
    operator int() const { return width; }
};

/* True if a slice of a hash expression is all 0s in the GF matrix.  Slice cannot be
 * be wider than what is returned by SliceWidth for its start bit */
class FormatHash::ZeroHash : public Inspector {
    const PhvInfo &phv;
    le_bitrange slice;
    safe_vector<Slice> &match_data;
    bool rv = false;
    bool preorder(const IR::Annotation *) { return false; }
    bool preorder(const IR::Type *) { return false; }
    bool preorder(const IR::BXor *e) {
        if (!rv) {
            visit(e->left, "left");
            bool tmp = rv;
            rv = false;
            visit(e->right, "right");
            rv &= tmp;
        }
        return false;
    }
    bool preorder(const IR::BAnd *) { return true; }
    bool preorder(const IR::BOr *) { return true; }
    bool preorder(const IR::BFN::SignExtend *) { return true; }
    bool preorder(const IR::Constant *k) {
        if (getParent<IR::BAnd>() || getParent<IR::BOr>()) {
            big_int v = k->value, mask = 1;
            v >>= slice.lo;
            mask <<= slice.size();
            mask -= 1;
            v &= mask;
            if (getParent<IR::BAnd>()) {
                if (v == 0) rv = true;
            } else if (v == mask) {
                rv = true;
            } else {
                BUG_CHECK(v == 0, "Incorrect slicing of BOr in hash expression");
            }
        } else {
            rv = true;
        }
        return false;
    }
    bool preorder(const IR::Concat *e) {
        int rwidth = e->right->type->width_bits();
        if (slice.lo < rwidth) {
            BUG_CHECK(slice.hi < rwidth, "Slice too wide in FormatHash::ZeroHash");
            visit(e->right, "right");
        } else {
            auto tmp = slice;
            slice = slice.shiftedByBits(-rwidth);
            visit(e->left, "left");
            slice = tmp;
        }
        return false;
    }
    bool preorder(const IR::ListExpression *fl) {
        auto tmp = slice;
        for (auto *e : boost::adaptors::reverse(fl->components)) {
            int width = e->type->width_bits();
            if (slice.lo < width) {
                BUG_CHECK(slice.hi < width, "Slice too wide in FormatHash::ZeroHash");
                visit(e);
                break;
            }
            slice = slice.shiftedByBits(-width);
        }
        slice = tmp;
        return false;
    }
    bool preorder(const IR::StructExpression *sl) {
        auto tmp = slice;
        for (auto *e : boost::adaptors::reverse(sl->components)) {
            int width = e->expression->type->width_bits();
            if (slice.lo < width) {
                BUG_CHECK(slice.hi < width, "Slice too wide in FormatHash::ZeroHash");
                visit(e);
                break;
            }
            slice = slice.shiftedByBits(-width);
        }
        slice = tmp;
        return false;
    }
    bool preorder(const IR::Expression *e) {
        Slice sl(phv, e, slice);
        BUG_CHECK(sl, "Invalid expression %s in FormatHash::ZeroHash", e);
        for (auto &md : match_data) {
            if (auto tmp = sl & md) {
                return false;
            }
        }
        rv = true;
        return false;
    }

 public:
    ZeroHash(const PhvInfo &p, const IR::Expression *e, le_bitrange s, safe_vector<Slice> &md)
        : phv(p), slice(s), match_data(md) {
        e->apply(*this);
    }
    explicit operator bool() const { return rv; }
};

/* output a slice of a hash expression as a .bfa hash expression */
class FormatHash::Output : public Inspector {
    const PhvInfo &phv;
    std::ostream &out;
    le_bitrange slice;
    safe_vector<Slice> &match_data;
    bool preorder(const IR::Annotation *) { return false; }
    bool preorder(const IR::Type *) { return false; }
    bool preorder(const IR::BXor *e) {
        bool need_left = !ZeroHash(phv, e->left, slice, match_data);
        bool need_right = !ZeroHash(phv, e->right, slice, match_data);
        if (need_left) visit(e->left, "left");
        if (need_left && need_right) out << " ^ ";
        if (need_right) visit(e->right, "right");
        return false;
    }
    bool preorder(const IR::BAnd *e) {
        visit(e->left, "left");
        out << "&";
        visit(e->right, "right");
        return false;
    }
    bool preorder(const IR::BOr *e) {
        auto k = e->left->to<IR::Constant>();
        auto &op = k ? e->right : e->left;
        if (!k) k = e->right->to<IR::Constant>();
        big_int v = k->value >> slice.lo;
        if ((v & 1) == 0) {
            visit(op);
        } else {
            out << "0";
        }
        return false;
    }
    bool preorder(const IR::Constant *k) {
        big_int v = k->value, mask = 1;
        v >>= slice.lo;
        mask <<= slice.size();
        mask -= 1;
        v &= mask;
        out << "0x" << std::hex << v << std::dec;
        return false;
    }
    bool preorder(const IR::Concat *e) {
        if (slice.lo < e->right->type->width_bits()) {
            visit(e->right, "right");
        } else {
            auto tmp = slice;
            slice = slice.shiftedByBits(-e->right->type->width_bits());
            visit(e->left, "left");
            slice = tmp;
        }
        return false;
    }
    bool preorder(const IR::ListExpression *fl) {
        auto tmp = slice;
        for (auto *e : boost::adaptors::reverse(fl->components)) {
            if (slice.lo < e->type->width_bits()) {
                visit(e);
                break;
            }
            slice = slice.shiftedByBits(-e->type->width_bits());
        }
        slice = tmp;
        return false;
    }
    bool preorder(const IR::StructExpression *sl) {
        auto tmp = slice;
        for (auto *e : boost::adaptors::reverse(sl->components)) {
            if (slice.lo < e->expression->type->width_bits()) {
                visit(e->expression);
                break;
            }
            slice = slice.shiftedByBits(-e->expression->type->width_bits());
        }
        slice = tmp;
        return false;
    }
    bool preorder(const IR::BFN::SignExtend *e) {
        if (slice.hi < e->expr->type->width_bits()) {
            // a slice or mask on a sign extension such that we don't need any of the
            // sign extended bits.  Could have been eliinated earlier, but ignore it here
            return true;
        }
        auto tmp = slice;
        slice.hi = e->expr->type->width_bits() - 1;
        out << "sign_extend(";
        visit(e->expr, "expr");
        slice = tmp;
        out << ")";
        return false;
    }
    bool preorder(const IR::Expression *e) {
        Slice sl(phv, e, slice);
        BUG_CHECK(sl, "Invalid expression %s in FormatHash::Output", e);
        for (auto &md : match_data) {
            if (auto tmp = sl & md) {
                out << tmp;
                break;
            }
        }
        return false;
    }

 public:
    Output(const PhvInfo &p, std::ostream &o, const IR::Expression *e, le_bitrange s,
           safe_vector<Slice> &md)
        : phv(p), out(o), slice(s), match_data(md) {
        e->apply(*this);
    }
};

#endif /* BF_P4C_MAU_ASM_FORMAT_HASH_H_ */
