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

#ifndef BF_P4C_PARDE_PARSER_HEADER_SEQUENCES_H_
#define BF_P4C_PARDE_PARSER_HEADER_SEQUENCES_H_

#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace {

static constexpr unsigned int numHeaderIDs = 255;
static constexpr unsigned int numHeadersPerSeq = 16;
static constexpr unsigned int payloadHeaderID = 254;
static cstring payloadHeaderStateName = "$final"_cs;
static cstring payloadHeaderName = "payload"_cs;

}  // namespace

/**
 * @brief Identify _parsed_ header sequences.
 */
class ParserHeaderSequences : public BFN::ControlFlowVisitor, public PardeInspector {
 protected:
    PhvInfo &phv;

    /**
     * @brief Record that @p header was parsed in @p gress
     */
    void record_header(gress_t gress, cstring header, size_t size);

 public:
    /** Headers extracted in the parser */
    std::map<gress_t, ordered_set<cstring>> headers;
    std::map<std::pair<gress_t, cstring>, int> header_ids;

    /** Header sequences extracted in the parser */
    std::map<gress_t, std::vector<ordered_set<cstring>>> sequences;
    // Name to size in bits of headers extracted in the ingress.
    std::map<cstring, size_t> header_sizes;

    explicit ParserHeaderSequences(PhvInfo &phv) : phv(phv) {}

    Visitor::profile_t init_apply(const IR::Node *node) override;
    bool preorder(const IR::BFN::Parser *) override;
    bool preorder(const IR::BFN::Extract *) override;

    void flow_merge(Visitor &) override;
    void flow_copy(::ControlFlowVisitor &) override;

    void end_apply() override;

    ParserHeaderSequences *clone() const override { return new ParserHeaderSequences(*this); }
};

#endif /* BF_P4C_PARDE_PARSER_HEADER_SEQUENCES_H_ */
