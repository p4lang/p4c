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

#ifndef BF_P4C_PARDE_RESET_INVALIDATED_CHECKSUM_HEADERS_H_
#define BF_P4C_PARDE_RESET_INVALIDATED_CHECKSUM_HEADERS_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

/**
 * \defgroup ResetInvalidatedChecksumHeaders ResetInvalidatedChecksumHeaders
 * \ingroup parde
 * \brief Reset fields that are used in deparser checksum operations and were invalidated.
 *
 * Fields involved in deparser checksum update can be invalidated in the
 * MAU and need to be reset before they reach the deparser. Otherwise
 * the fields will corrupt the checksum calculation.
 * This is only needed for Tofino whose deparser checksum entries
 * are statically configured.
 */

struct CollectPovBitToFields : public DeparserInspector {
    const PhvInfo &phv;

    std::map<const PHV::Field *, std::set<const PHV::Field *>> pov_bit_to_fields;

    std::map<const PHV::Field *, const IR::Expression *> phv_field_to_expr;

    explicit CollectPovBitToFields(const PhvInfo &phv) : phv(phv) {}

    bool preorder(const IR::BFN::EmitField *emit) override {
        auto pov_bit = phv.field(emit->povBit->field);
        auto source = phv.field(emit->source->field);

        phv_field_to_expr[pov_bit] = emit->povBit->field;
        phv_field_to_expr[source] = emit->source->field;

        pov_bit_to_fields[pov_bit].insert(source);
        return false;
    }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        pov_bit_to_fields.clear();
        phv_field_to_expr.clear();
        return rv;
    }
};

/**
 * \ingroup ResetInvalidatedChecksumHeaders
 */
struct CollectInvalidatedHeaders : public Inspector {
    const PhvInfo &phv;
    const std::map<const PHV::Field *, std::set<const PHV::Field *>> &pov_bit_to_fields;

    std::map<gress_t, ordered_set<const PHV::Field *>> invalidated_header_pov_bits;

    std::map<const PHV::Field *, const PHV::Field *> invalidated_field_to_pov_bit;

    std::map<const PHV::Field *, ordered_set<const PHV::Field *>>
        pov_bit_to_invalidated_checksum_fields;

    CollectInvalidatedHeaders(
        const PhvInfo &phv,
        const std::map<const PHV::Field *, std::set<const PHV::Field *>> &pov_bit_to_fields)
        : phv(phv), pov_bit_to_fields(pov_bit_to_fields) {}

    bool preorder(const IR::MAU::Primitive *p) override {
        auto table = findContext<IR::MAU::Table>();
        auto gress = table->gress;

        if (p->name == "modify_field") {
            auto dst = p->operands[0]->to<IR::Member>();
            auto src = p->operands[1]->to<IR::Constant>();

            if (dst && dst->member == "$valid" && src && src->asInt() == 0) {
                auto dst_field = phv.field(dst);
                invalidated_header_pov_bits[gress].insert(dst_field);

                if (pov_bit_to_fields.count(dst_field)) {
                    for (auto f : pov_bit_to_fields.at(dst_field)) {
                        invalidated_field_to_pov_bit[f] = dst_field;
                        LOG4(f->name << " can be invalidated");
                    }
                }
            }
        }

        return false;
    }

    bool preorder(const IR::BFN::EmitChecksum *checksum) override {
        auto csum_dest = phv.field(checksum->dest->field);
        for (auto source : checksum->sources) {
            auto field = phv.field(source->field->field);

            if (invalidated_field_to_pov_bit.count(field)) {
                auto pov_bit = invalidated_field_to_pov_bit.at(field);
                if (pov_bit->header() == csum_dest->header()) continue;
                auto checksum_pov_bit = phv.field(checksum->povBit->field);
                if (pov_bit != checksum_pov_bit) {
                    pov_bit_to_invalidated_checksum_fields[pov_bit].insert(field);

                    LOG4("checksum payload field " << field->name << " can be invalidated");
                }
            }
        }

        return false;
    }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        invalidated_field_to_pov_bit.clear();
        invalidated_header_pov_bits.clear();
        pov_bit_to_invalidated_checksum_fields.clear();
        return rv;
    }
};

/**
 * \ingroup ResetInvalidatedChecksumHeaders
 */
class InsertParsedValidBits : public ParserModifier {
    const PhvInfo &phv;
    const CollectInvalidatedHeaders &invalidated_headers;

 public:
    std::map<const PHV::Field *, const IR::TempVar *> pov_bit_to_parsed_valid_bit;

    InsertParsedValidBits(const PhvInfo &phv, const CollectInvalidatedHeaders &invalidated_headers)
        : phv(phv), invalidated_headers(invalidated_headers) {}

 private:
    IR::TempVar *create_parsed_valid_bit(const PHV::Field *pov_bit) {
        std::string pov_bit_name(pov_bit->name.c_str());
        auto pos = pov_bit_name.find("$valid");

        BUG_CHECK(pos != std::string::npos, "pov bit name does not end with $valid?");

        std::string parsed_valid_bit_name = pov_bit_name.replace(pos, 7, "$parsed");

        auto parsed_valid_bit =
            new IR::TempVar(IR::Type::Bits::get(1), false, cstring(parsed_valid_bit_name));

        LOG4("created " << parsed_valid_bit_name);
        return parsed_valid_bit;
    }

    bool preorder(IR::BFN::ParserState *state) override {
        IR::Vector<IR::BFN::ParserPrimitive> statements;

        for (auto stmt : state->statements) {
            statements.push_back(stmt);

            if (auto extract = stmt->to<IR::BFN::Extract>()) {
                auto dest = phv.field(extract->dest->field);
                if (!dest || !dest->pov) continue;

                if (invalidated_headers.pov_bit_to_invalidated_checksum_fields.count(dest)) {
                    auto parsed_valid_bit = create_parsed_valid_bit(dest);
                    pov_bit_to_parsed_valid_bit[dest] = parsed_valid_bit;

                    auto rval = new IR::BFN::ConstantRVal(IR::Type::Bits::get(1), 1);
                    auto extract = new IR::BFN::Extract(parsed_valid_bit, rval);
                    statements.push_back(extract);
                }
            }
        }

        state->statements = statements;

        return true;
    }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        profile_t rv = ParserModifier::init_apply(root);
        pov_bit_to_parsed_valid_bit.clear();
        return rv;
    }
};

/**
 * \ingroup ResetInvalidatedChecksumHeaders
 */
class InsertTableToResetInvalidatedHeaders : public MauTransform {
    const std::map<const PHV::Field *, const IR::Expression *> &phv_field_to_expr;
    const CollectInvalidatedHeaders &invalidated_headers;
    const InsertParsedValidBits &parsed_valid_bits;

    std::map<gress_t, std::vector<IR::MAU::Table *>> tables_to_insert;
    IR::MAU::Table *create_actions(gress_t gress,
                                   const ordered_set<const PHV::Field *> &fields_to_reset,
                                   IR::BFN::Pipe *pipe) {
        auto action = new IR::MAU::Action("__reset_invalidated_checksum_fields__");
        action->default_allowed = action->init_default = true;

        for (auto field : fields_to_reset) {
            auto field_expr = phv_field_to_expr.at(field);
            auto reset = new IR::MAU::Instruction(
                "set"_cs, field_expr, new IR::Constant(IR::Type_Bits::get(field->size), 0));
            action->action.push_back(reset);
            auto *annotation = new IR::Annotation(IR::ID("pa_no_overlay"),
                                                  {new IR::StringLiteral(IR::ID(toString(gress))),
                                                   new IR::StringLiteral(IR::ID(field->name))});
            pipe->global_pragmas.push_back(annotation);
            LOG3("Added pa_no_overlay pragma on field: " << field->name
                                                         << " in gress: " << field->gress);
        }

        static int id = 0;
        auto table_name =
            toString(gress) + "_reset_invalidated_checksum_fields_" + cstring::to_cstring(id++);
        auto t = new IR::MAU::Table(table_name, gress);

        t->is_compiler_generated = true;
        t->run_before_exit = true;
        t->actions[action->name] = action;
        t->match_table = new IR::P4Table(table_name.c_str(), new IR::TableProperties());

        return t;
    }

    IR::MAU::Table *create_reset_table(gress_t gress, const PHV::Field *pov_bit,
                                       const ordered_set<const PHV::Field *> &fields_to_reset,
                                       IR::BFN::Pipe *pipe) {
        auto ac = create_actions(gress, fields_to_reset, pipe);

        static int id = 0;
        cstring gateway_name = toString(gress) + "_reset_invalidated_checksum_fields_cond_" +
                               cstring::to_cstring(id++);

        auto pov_bit_expr = phv_field_to_expr.at(pov_bit);
        auto parsed_valid_bit = parsed_valid_bits.pov_bit_to_parsed_valid_bit.at(pov_bit);

        auto cond = new IR::LAnd(new IR::LNot(pov_bit_expr), parsed_valid_bit);
        auto gw = new IR::MAU::Table(gateway_name, gress, cond);

        gw->is_compiler_generated = true;
        gw->next.emplace("$true"_cs, new IR::MAU::TableSeq(ac));

        return gw;
    }

    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *seq) override {
        prune();

        gress_t gress = VisitingThread(this);

        if (tables_to_insert.count(gress)) {
            auto &tables = tables_to_insert.at(gress);

            LOG4(tables.size() << " reset invalidated checksum fields table to insert at "
                               << gress);

            for (auto t : tables) seq->tables.push_back(t);
        }

        return seq;
    }

    IR::BFN::Pipe *preorder(IR::BFN::Pipe *pipe) override {
        for (auto &gf : invalidated_headers.invalidated_header_pov_bits) {
            auto gress = gf.first;

            for (auto pov_bit : gf.second) {
                if (invalidated_headers.pov_bit_to_invalidated_checksum_fields.count(pov_bit)) {
                    auto &fields_to_reset =
                        invalidated_headers.pov_bit_to_invalidated_checksum_fields.at(pov_bit);

                    // At the end of the control flow, insert table to reset invalidated header:
                    //
                    //    if (hdr.$parsed && !hdr.$valid)
                    //        hdr = 0;
                    //
                    // For now, create one table for each invalidated header.
                    //
                    // Combine multiple headers into single table? What's the tradeoff?
                    // IMEM capacity, instr color, and induces no overlay constraint? TODO
                    //
                    auto t = create_reset_table(gress, pov_bit, fields_to_reset, pipe);
                    tables_to_insert[gress].push_back(t);
                }
            }
        }
        return pipe;
    }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        profile_t rv = MauTransform::init_apply(root);
        tables_to_insert.clear();
        return rv;
    }

 public:
    InsertTableToResetInvalidatedHeaders(
        const std::map<const PHV::Field *, const IR::Expression *> &phv_field_to_expr,
        const CollectInvalidatedHeaders &invalidated_headers,
        const InsertParsedValidBits &parsed_valid_bits)
        : phv_field_to_expr(phv_field_to_expr),
          invalidated_headers(invalidated_headers),
          parsed_valid_bits(parsed_valid_bits) {}
};

/**
 * \ingroup ResetInvalidatedChecksumHeaders
 * \brief Top level PassManager.
 */
class ResetInvalidatedChecksumHeaders : public PassManager {
 public:
    explicit ResetInvalidatedChecksumHeaders(const PhvInfo &phv) {
        auto collect_pov_bit_to_fields = new CollectPovBitToFields(phv);

        auto collect_invalidated_headers =
            new CollectInvalidatedHeaders(phv, collect_pov_bit_to_fields->pov_bit_to_fields);

        auto insert_parsed_valid_bits =
            new InsertParsedValidBits(phv, *collect_invalidated_headers);

        auto insert_reset_tables = new InsertTableToResetInvalidatedHeaders(
            collect_pov_bit_to_fields->phv_field_to_expr, *collect_invalidated_headers,
            *insert_parsed_valid_bits);

        addPasses({collect_pov_bit_to_fields, collect_invalidated_headers, insert_parsed_valid_bits,
                   insert_reset_tables});
    }
};

#endif /* BF_P4C_PARDE_RESET_INVALIDATED_CHECKSUM_HEADERS_H_ */
