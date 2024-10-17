/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_PARDE_PARDE_UTILS_H_
#define EXTENSIONS_BF_P4C_PARDE_PARDE_UTILS_H_

#include "bf-p4c/device.h"


static const IR::BFN::PacketRVal* get_packet_range(const IR::BFN::ParserPrimitive* p) {
    if (auto e = p->to<IR::BFN::Extract>()) {
        if (auto range = e->source->to<IR::BFN::PacketRVal>()) {
            return range;
        } else {
            return nullptr;
        }
    } else if (auto c = p->to<IR::BFN::ChecksumResidualDeposit>()) {
        return c->header_end_byte;
    } else if (auto c = p->to<IR::BFN::ChecksumSubtract>()) {
        return c->source;
    } else if (auto c = p->to<IR::BFN::ChecksumAdd>()) {
        return c->source;
    }
    return nullptr;
}

struct SortExtracts {
    explicit SortExtracts(IR::BFN::ParserState* state) {
        std::stable_sort(state->statements.begin(), state->statements.end(),
            [&] (const IR::BFN::ParserPrimitive* a,
                 const IR::BFN::ParserPrimitive* b) {
                auto va = get_packet_range(a);
                auto vb = get_packet_range(b);
                return (va && vb) ? (va->range < vb->range) : !!va;
            });

        if (LOGGING(5)) {
            std::clog << "sorted primitives in " << state->name << std::endl;
            for (auto p : state->statements)
                std::clog << p << std::endl;
        }
    }
};

struct GetMaxBufferPos : Inspector {
    int rv = -1;

    bool preorder(const IR::BFN::InputBufferRVal* rval) override {
        rv = std::max(rval->range.hi, rv);
        return false;
    }
};

struct GetMinBufferPos : Inspector {
    int rv = Device::pardeSpec().byteInputBufferSize() * 8;

    bool preorder(const IR::BFN::InputBufferRVal* rval) override {
        if (rval->range.hi < 0) return false;
        if (rval->range.lo < 0) BUG("rval straddles input buffer?");
        rv = std::min(rval->range.lo, rv);
        return false;
    }
};

struct Shift : Transform {
    int shift_amt = 0;  // in bits
    explicit Shift(int shft) : shift_amt(shft) {
        BUG_CHECK(shift_amt % 8 == 0, "shift amount not byte-aligned?");
    }
};

struct ShiftPacketRVal : Shift {
    bool negative_ok = false;
    explicit ShiftPacketRVal(int shft, bool neg_ok = false) :
        Shift(shft), negative_ok(neg_ok) { }

    IR::Node* preorder(IR::BFN::PacketRVal* rval) override {
        auto new_range = rval->range.shiftedByBits(-shift_amt);
        if (!negative_ok)
            BUG_CHECK(new_range.lo >= 0, "packet rval shifted to be negative?");
        rval->range = new_range;
        return rval;
    }

    IR::Node* postorder(IR::BFN::ChecksumSubtract* csum) {
        auto* orig = getOriginal<IR::BFN::ChecksumSubtract>();
        if (csum->source->range.loByte() % 2 != orig->source->range.loByte() % 2) {
            return new IR::BFN::ChecksumSubtract(csum->declName,
                    csum->source, !csum->swap, csum->isPayloadChecksum);
        }
        return csum;
    }

    IR::Node* postorder(IR::BFN::ChecksumAdd* csum) {
        auto* orig = getOriginal<IR::BFN::ChecksumAdd>();
        if (csum->source->range.loByte() % 2 != orig->source->range.loByte() % 2) {
            return new IR::BFN::ChecksumAdd(csum->declName,
                    csum->source, !csum->swap, csum->isHeaderChecksum);
        }
        return csum;
    }
};

inline unsigned get_state_shift(const IR::BFN::ParserState* state) {
    unsigned state_shift = 0;

    for (unsigned i = 0; i < state->transitions.size(); i++) {
        auto t = state->transitions[i];

        if (i == 0)
            state_shift = t->shift;
        else
            BUG_CHECK(state_shift == t->shift, "Inconsistent shifts in %1%", state->name);
    }

    return state_shift;
}

#endif /* EXTENSIONS_BF_P4C_PARDE_PARDE_UTILS_H_ */
