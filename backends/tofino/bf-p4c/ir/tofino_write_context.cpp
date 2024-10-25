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

#include "bf-p4c/ir/tofino_write_context.h"

#include "lib/log.h"

bool TofinoWriteContext::isWrite(bool root_value) {
    bool rv = P4WriteContext::isWrite(root_value);
    const IR::Node *current = getCurrentNode();
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node || !current) return rv;

    while (ctxt->child_index == 0 &&
           (ctxt->node->is<IR::ArrayIndex>() || ctxt->node->is<IR::HeaderStackItemRef>() ||
            ctxt->node->is<IR::MAU::MultiOperand>() || ctxt->node->is<IR::ListExpression>() ||
            ctxt->node->is<IR::StructExpression>() || ctxt->node->is<IR::Slice>() ||
            ctxt->node->is<IR::Member>())) {
        ctxt = ctxt->parent;
        if (!ctxt || !ctxt->node) return rv;
    }

    BUG_CHECK(!ctxt->node->is<IR::BFN::LoweredParserRVal>(),
              "Computing reads and writes over lowered parser IR?");

    auto *salu = findContext<IR::MAU::SaluAction>();
    if (salu && current == salu->output_dst) {
        return true;
    }

    // Parser primitives write to l-values - for example,
    // (1) an Extract writes to its destination
    // (2) ghost parser write to ghost lvalues.
    if (ctxt->node->is<IR::BFN::ParserLVal>() &&
        (findContext<IR::BFN::ParserPrimitive>() || findContext<IR::BFN::GhostParser>()))
        return true;

    // TODO: Does C++ support monads?  The following if statements are nested
    // because C++ only supports declaring variables in if predicates if the
    // declaration is the only clause.
    if (auto *prim = ctxt->node->to<IR::MAU::TypedPrimitive>()) {
        if (prim->method_type) {
            if (auto *tm = prim->method_type->to<IR::Type_Method>()) {
                if (tm->parameters) {
                    if (const IR::IndexedVector<IR::Parameter> *params =
                            &tm->parameters->parameters) {
                        if (ctxt->child_index > 0) {
                            // child 0 is 'this', which is not included in the parameter list
                            if (static_cast<size_t>(ctxt->child_index) <= params->size()) {
                                const IR::Direction d =
                                    params->at(ctxt->child_index - 1)->direction;
                                if (d == IR::Direction::Out || d == IR::Direction::InOut)
                                    return true;
                                else
                                    return false;
                            }
                        }
                    }
                }
            }
        }
    }

    if (auto *instr = ctxt->node->to<IR::MAU::Instruction>())
        return instr->isOutput(ctxt->child_index);

    return rv;
}

bool TofinoWriteContext::isRead(bool root_value) {
    bool rv = P4WriteContext::isRead(root_value);
    const IR::Node *current = getCurrentNode();
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node || !current) return rv;
    while (ctxt->child_index == 0 &&
           (ctxt->node->is<IR::ArrayIndex>() || ctxt->node->is<IR::HeaderStackItemRef>() ||
            ctxt->node->is<IR::Slice>() || ctxt->node->is<IR::Member>())) {
        ctxt = ctxt->parent;
        if (!ctxt || !ctxt->node) return rv;
    }

    if (ctxt->child_index < 0) return rv;

    BUG_CHECK(!ctxt->node->is<IR::BFN::LoweredParserRVal>(),
              "Computing reads and writes over lowered parser IR?");

    auto *salu = findContext<IR::MAU::SaluAction>();
    if (salu && current == salu->output_dst) {
        return false;
    }

    // TODO: This treats both the destination and the source of the extract
    // as read. This is a hack intended to capture the fact that parser writes
    // OR the destination PHV container with its existing contents.
    // TODO: We should remove this as soon as it's no longer needed; it
    // doesn't make sense.
    if (ctxt->node->is<IR::BFN::ParserLVal>() && ctxt->parent &&
        ctxt->parent->node->is<IR::BFN::Extract>())
        return true;

    // Parser r-values (e.g. the source of a Select) are read.
    if (ctxt->node->is<IR::BFN::ParserRVal>()) return true;

    // Deparser parameters obtain their value from l-values.
    if (ctxt->node->is<IR::BFN::ParserLVal>() && findContext<IR::BFN::DeparserPrimitive>())
        return true;

    // An Emit reads both the emitted field and the POV bit.
    // TODO: These should be represented as l-values as well.
    if (ctxt->node->is<IR::BFN::EmitField>()) return true;

    // An EmitChecksum reads the POV bit and all checksummed fields.
    // TODO: These also.
    if (ctxt->node->is<IR::BFN::EmitChecksum>()) return true;

    // TODO: Does C++ support monads?  The following if statements are nested
    // because C++ only supports declaring variables in if predicates if the
    // declaration is the only clause.
    if (auto *prim = ctxt->node->to<IR::MAU::TypedPrimitive>()) {
        if (prim->method_type) {
            if (auto *tm = prim->method_type->to<IR::Type_Method>()) {
                if (tm->parameters) {
                    if (const IR::IndexedVector<IR::Parameter> *params =
                            &tm->parameters->parameters) {
                        if (ctxt->child_index > 0) {
                            // child 0 is 'this', which is not included in the parameter list
                            if ((size_t)(ctxt->child_index) <= params->size()) {
                                const IR::Direction d =
                                    params->at(ctxt->child_index - 1)->direction;
                                if (d == IR::Direction::In || d == IR::Direction::InOut)
                                    return true;
                                else
                                    return false;
                            }
                        }
                    }
                }
            }
        }
    }

    if (auto *instr = ctxt->node->to<IR::MAU::Instruction>())
        return !instr->isOutput(ctxt->child_index);

    if (findContext<IR::MAU::TableKey>()) return true;

    if (findContext<IR::MAU::HashDist>()) return true;

    return rv;
}

bool TofinoWriteContext::isIxbarRead(bool /* root_value */) {
    bool rv = isRead();
    const IR::Node *current = getCurrentNode();
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node || !current || !rv) return false;
    while (ctxt->child_index == 0 &&
           (ctxt->node->is<IR::ArrayIndex>() || ctxt->node->is<IR::HeaderStackItemRef>() ||
            ctxt->node->is<IR::Slice>() || ctxt->node->is<IR::Member>())) {
        ctxt = ctxt->parent;
        if (!ctxt || !ctxt->node) return false;
    }
    if (findContext<IR::MAU::TableKey>()) {
        return true;
    }
    if (findContext<IR::MAU::HashDist>()) {
        return true;
    }
    if (findContext<IR::MAU::AttachedMemory>()) {
        return true;
    }

    // Gateway expressions
    // This doesn't work for some reason...
    if (ctxt && ctxt->node->is<IR::Expression>()) {
        while (ctxt && ctxt->node->is<IR::Expression>()) {
            ctxt = ctxt->parent;
        }
        if (ctxt && ctxt->node->is<IR::MAU::Table>()) {
            return true;
        }
    }
    return false;
}
