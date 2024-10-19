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

/**
 * \defgroup DesugarVarbitExtract DesugarVarbitExtract
 * \ingroup midend
 * \ingroup parde
 * \brief %Set of passes that rewrite usage of varbit.
 *
 * Header field may have variable size (varbit), which can be encoded
 * by another header field, e.g. IPv4 options length is encoded by ihl
 * (see below).
 *
 * Tofino's parser does not have an ALU and cannot evaluate such complex
 * expression. To support extraction of varbit field, we must
 * decompose the length expression into a set of constructs that is
 * implementable using the parser features, i.e. match and state transition.
 *
 * To implement this, we enumerate all possible compile time values
 * of the length expression, and assign a parser state to each length
 * where the varbit field is extracted.
 *
 *
 *     header ipv4_t {
 *         bit<4>  version;
 *         bit<4>  ihl;
 *         bit<8>  diffserv;
 *         bit<16> totalLen;
 *         bit<16> identification;
 *         bit<3>  flags;
 *         bit<13> fragOffset;
 *         bit<8>  ttl;
 *         bit<8>  protocol;
 *         bit<16> hdrChecksum;
 *         bit<32> srcAddr;
 *         bit<32> dstAddr;
 *         varbit<320> options;
 *     }
 *
 *     state start {
 *         verify(p.ipv4.ihl >= 5, error.NoMatch);
 *         b.extract(p.ipv4, (bit<32>)(((bit<16>)p.ipv4.ihl - 5) * 32));
 *         transition accept;
 *     }
 *
 *
 * After rewrite:
 *
 *     state start {
 *         b.extract<ipv4_t>(p.ipv4);
 *         transition select(p.ipv4.ihl) {
 *             4w5 &&& 4w15: parse_options_no_option;
 *             4w6 &&& 4w15: parse_options_32b;
 *             4w7 &&& 4w15: parse_options_64b;
 *             4w8 &&& 4w15: parse_options_96b;
 *             4w9 &&& 4w15: parse_options_128b;
 *             4w10 &&& 4w15: parse_options_160b;
 *             4w11 &&& 4w15: parse_options_192b;
 *             4w12 &&& 4w15: parse_options_224b;
 *             4w13 &&& 4w15: parse_options_256b;
 *             4w14 &&& 4w15: parse_options_288b;
 *             4w15 &&& 4w15: parse_options_320b;
 *             4w0 &&& 4w15: reject;
 *             4w1 &&& 4w15: reject;
 *             4w2 &&& 4w15: reject;
 *             4w3 &&& 4w15: reject;
 *             4w4 &&& 4w15: reject;
 *         }
 *     }
 *
 *
 *
 * Each compiler-added varbit state extracts multiple small size headers. The total number of
 * header extracted in that state will be proportional to length calculated in
 * compile time for state.  The sizes of the small headers can differ with the pattern of
 * varbit lengths:
 *
 *     state parse_32b {
 *         extract<bit<32>>(); }
 *     state parse_48b {
 *         extract<bit<32>>();
 *         extract<bit<16>>(); }
 *
 */
#ifndef BF_P4C_MIDEND_DESUGAR_VARBIT_EXTRACT_H_
#define BF_P4C_MIDEND_DESUGAR_VARBIT_EXTRACT_H_

#include "bf-p4c/common/utils.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/cloner.h"
#include "ir/ir.h"

namespace BFN {

/**
 * \ingroup DesugarVarbitExtract
 */
class AnnotateVarbitExtractStates : public Transform {
    IR::Node *preorder(IR::ParserState *state) override {
        for (const auto *component : state->components) {
            const auto *statement = component->to<IR::MethodCallStatement>();
            if (!statement) continue;
            const IR::MethodCallExpression *call = statement->methodCall;
            if (!call) continue;
            const auto *method = call->method->to<IR::Member>();
            if (!method) continue;

            if (method->member == "extract" && call->arguments->size() == 2) {
                IR::Annotations *annotations = state->annotations->clone();
                annotations->add(new IR::Annotation(IR::ID("dontmerge"), {}));
                state->annotations = annotations;
                break;
            }
        }
        return state;
    }
};

/**
 * \ingroup DesugarVarbitExtract
 * \brief Checks that varbit accesses in pipeline are valid.
 *
 * In particular, this checks that there are no unsupported varbit accesses
 * (basically anything except for extract and emit currently).
 * This should run rather early in midend.
 */
class CheckVarbitAccess : public PassManager {
 public:
    CheckVarbitAccess();
};

/**
 * \ingroup DesugarVarbitExtract
 * \brief Top level PassManager that governs the rewrite of varbit usage.
 */
class DesugarVarbitExtract : public PassManager {
 public:
    explicit DesugarVarbitExtract(P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
};

}  // namespace BFN

#endif /* BF_P4C_MIDEND_DESUGAR_VARBIT_EXTRACT_H_ */
