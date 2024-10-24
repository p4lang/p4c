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

#ifndef BF_P4C_MIDEND_PARSER_ENFORCE_DEPTH_REQ_H_
#define BF_P4C_MIDEND_PARSER_ENFORCE_DEPTH_REQ_H_

#include "ir/ir.h"
#include "midend/type_checker.h"

namespace BFN {

/**
 * @ingroup midend
 * @ingroup parde
 * @brief Enforce parser min/max depth requirements
 *
 * Programs that exceed the maximum parse depth are rejected.
 * Programs that fall below the minimum parse depth have additional "pad" states added to enforce a
 * minimum parse depth.
 */
class ParserEnforceDepthReq : public PassManager {
 public:
    /// Pad requirements for a parser instance to enforce min parse depth
    struct ParserPadReq {
        // Parser instance is the key of the map below
        // const IR::P4Parser *parser;
        int minParseDepth;
        int maxParseDepth;

        /// Maximum pad states
        int maxPadStates;

        /**
         * Map of states with accept transitions that need padding.
         * Key: state
         * Value: boolean indicating wether to apply padding unconditionally,
         * or whether to transition to an initial state that can potentially bypass padding based on
         * counter.
         */
        std::map<cstring, bool> padStatesAccept;
        std::map<cstring, bool> padStatesReject;
    };

    static const cstring pad_hdr_name;
    static const cstring pad_hdr_type_name;
    static const cstring pad_hdr_field;
    static const cstring pad_ctr_name;
    static const cstring pad_state_name;
    static const cstring non_struct_pad_suf;

 private:
    P4::ReferenceMap *refMap;

    BFN::EvaluatorPass *evaluator;

    std::set<cstring> structs;
    std::map<const IR::P4Parser *, ParserPadReq> padReq;
    std::map<cstring, int> headerPadAmt;
    std::map<const IR::P4Parser *, gress_t> all_parser;
    std::map<const IR::P4Control *, gress_t> all_deparser;
    std::map<const IR::P4Control *, gress_t> all_mau_pipe;
    std::map<const IR::P4Control *, std::set<const IR::P4Parser *>> deparser_parser;
    std::map<const IR::P4Control *, std::set<const IR::P4Parser *>> mau_pipe_parser;
    std::map<const IR::P4Parser *, std::map<cstring, int>> stateSize;

    int ctrShiftAmt;

 public:
    explicit ParserEnforceDepthReq(P4::ReferenceMap *rm, BFN::EvaluatorPass *ev);
};

}  // namespace BFN

#endif /* BF_P4C_MIDEND_PARSER_ENFORCE_DEPTH_REQ_H_ */
