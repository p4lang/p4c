/*
Copyright 2020 Intel Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "dpdkProgram.h"

#include <map>
#include <ostream>
#include <utility>

#include "backends/dpdk/dpdkProgramStructure.h"
#include "backends/dpdk/options.h"
#include "dpdkHelpers.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_map.h"

namespace DPDK {
/* Insert the metadata structure updated with tmp variables created during parser conversion
   Add all the structures to DPDK structtype.
*/
IR::IndexedVector<IR::DpdkStructType> ConvertToDpdkProgram::UpdateHeaderMetadata(
    IR::P4Program *prog, IR::Type_Struct *metadata) {
    IR::IndexedVector<IR::DpdkStructType> structType;
    auto new_objs = new IR::Vector<IR::Node>;
    for (auto obj : prog->objects) {
        if (auto s = obj->to<IR::Type_Struct>()) {
            if (s->name.name == structure->local_metadata_type) {
                new_objs->push_back(metadata);
                auto st =
                    new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, metadata->fields);
                structType.push_back(st);
            } else {
                if (structure->args_struct_map.find(s->name.name) !=
                    structure->args_struct_map.end()) {
                    auto st =
                        new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, s->fields);
                    structType.push_back(st);
                } else if (s->name.name == structure->header_type) {
                    auto st =
                        new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, s->fields);
                    structType.push_back(st);
                }
                new_objs->push_back(s);
            }
        } else {
            new_objs->push_back(obj);
        }
    }
    prog->objects = *new_objs;
    return structType;
}

IR::IndexedVector<IR::DpdkAsmStatement> ConvertToDpdkProgram::create_pna_preamble() {
    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    instr.push_back(new IR::DpdkRxStatement(
        new IR::Member(new IR::PathExpression("m"), "pna_main_input_metadata_input_port")));
    return instr;
}

IR::IndexedVector<IR::DpdkAsmStatement> ConvertToDpdkProgram::create_psa_preamble() {
    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    instr.push_back(new IR::DpdkRxStatement(
        new IR::Member(new IR::PathExpression("m"), "psa_ingress_input_metadata_ingress_port")));
    instr.push_back(new IR::DpdkMovStatement(
        new IR::Member(new IR::PathExpression("m"), "psa_ingress_output_metadata_drop"),
        new IR::Constant(1)));
    return instr;
}

IR::IndexedVector<IR::DpdkAsmStatement> ConvertToDpdkProgram::create_pna_postamble() {
    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    instr.push_back(new IR::DpdkTxStatement(
        new IR::Member(new IR::PathExpression("m"), PnaMainOutputMetadataOutputPortName)));
    return instr;
}

IR::IndexedVector<IR::DpdkAsmStatement> ConvertToDpdkProgram::create_psa_postamble() {
    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    instr.push_back(new IR::DpdkTxStatement(
        new IR::Member(new IR::PathExpression("m"), "psa_ingress_output_metadata_egress_port")));
    instr.push_back(new IR::DpdkLabelStatement("label_drop"));
    instr.push_back(new IR::DpdkDropStatement());
    return instr;
}

const IR::DpdkAsmProgram *ConvertToDpdkProgram::create(IR::P4Program *prog) {
    IR::Type_Struct *metadataStruct = nullptr;

    for (auto obj : prog->objects) {
        if (auto s = obj->to<IR::Type_Struct>()) {
            if (s->name.name == structure->local_metadata_type) {
                metadataStruct = s->clone();
                break;
            }
        }
    }

    IR::IndexedVector<IR::DpdkAsmStatement> statements;

    auto ingress_parser_converter =
        new ConvertToDpdkParser(refmap, typemap, structure, metadataStruct);
    auto egress_parser_converter =
        new ConvertToDpdkParser(refmap, typemap, structure, metadataStruct);
    for (auto kv : structure->parsers) {
        if (kv.first == "IngressParser")
            kv.second->apply(*ingress_parser_converter);
        else if (kv.first == "EgressParser") {
            if (options.enableEgress) kv.second->apply(*egress_parser_converter);
        } else if (kv.first == "MainParserT")
            kv.second->apply(*ingress_parser_converter);
        else
            BUG("Unknown parser %s", kv.second->name);
    }
    auto ingress_converter = new ConvertToDpdkControl(refmap, typemap, structure, metadataStruct);
    auto egress_converter = new ConvertToDpdkControl(refmap, typemap, structure, metadataStruct);
    for (auto kv : structure->pipelines) {
        if (kv.first == "Ingress")
            kv.second->apply(*ingress_converter);
        else if (kv.first == "Egress") {
            if (options.enableEgress) kv.second->apply(*egress_converter);
        } else if (kv.first == "PreControlT")
            kv.second->apply(*ingress_converter);
        else if (kv.first == "MainControlT")
            kv.second->apply(*ingress_converter);
        else
            BUG("Unknown control block %s", kv.second->name);
    }
    auto ingress_deparser_converter =
        new ConvertToDpdkControl(refmap, typemap, structure, metadataStruct, true);
    auto egress_deparser_converter =
        new ConvertToDpdkControl(refmap, typemap, structure, metadataStruct);
    for (auto kv : structure->deparsers) {
        if (kv.first == "IngressDeparser")
            kv.second->apply(*ingress_deparser_converter);
        else if (kv.first == "EgressDeparser") {
            if (options.enableEgress) kv.second->apply(*egress_deparser_converter);
        } else if (kv.first == "MainDeparserT")
            kv.second->apply(*ingress_deparser_converter);
        else
            BUG("Unknown deparser block %s", kv.second->name);
    }

    IR::IndexedVector<IR::DpdkAsmStatement> instr;
    if (structure->isPNA())
        instr.append(create_pna_preamble());
    else if (structure->isPSA())
        instr.append(create_psa_preamble());

    instr.append(ingress_parser_converter->getInstructions());
    instr.append(ingress_converter->getInstructions());
    instr.append(ingress_deparser_converter->getInstructions());
    if (options.enableEgress) {
        instr.append(egress_parser_converter->getInstructions());
        instr.append(egress_converter->getInstructions());
        instr.append(egress_deparser_converter->getInstructions());
    }

    if (structure->isPNA())
        instr.append(create_pna_postamble());
    else if (structure->isPSA())
        instr.append(create_psa_postamble());

    statements.push_back(new IR::DpdkListStatement(instr));

    IR::IndexedVector<IR::DpdkHeaderType> headerType;
    for (auto kv : structure->header_types) {
        auto h = kv.second;
        auto ht = new IR::DpdkHeaderType(h->srcInfo, h->name, h->annotations, h->fields);
        headerType.push_back(ht);
    }

    IR::IndexedVector<IR::DpdkStructType> structType;
    IR::IndexedVector<IR::DpdkStructType> updatedHeaderMetadataStructs =
        UpdateHeaderMetadata(prog, metadataStruct);
    for (auto kv : structure->metadata_types) {
        auto s = kv.second;
        auto structTypeName = s->getName().name;
        if (updatedHeaderMetadataStructs.getDeclaration(structTypeName) != nullptr) {
            /**
             * UpdateHeaderMetadata returns IndexedVector filled with following 3 types of structs:
             * - main metadata structure (whose name is stored in structure->local_metadata_type)
             * - structures for action arguments created internally by compiler (stored in
             *   structure->args_struct_map)
             * - main structure which holds all headers (whose name is stored in
             *   structure->header_type)
             *
             * Only the first one (main metadata structure) can be present in
             * structure->metadata_types map.
             * Main metadata structure is added to the map when it contains metadata whose
             * type is a structure.
             * It happens during InspectDpdkProgram pass which visits IR::Parameter nodes in
             * InspectDpdkProgram::preorder(const IR::Parameter* param).
             * During that pass the visitor only visits the parameter in the statement from
             * architecture.
             * Structures internally created for action arguments are not visited in that pass,
             * so they are not added to structure->metadata_types.
             * As they are created internally by compiler they are not used as types of fields
             * in main metadata structure neither.
             * Main headers structure contains only headers so it is not added to
             * structure->metadata_types map neither.
             * InspectDpdkProgram adds there only the structures which contain some field
             * with structure type.
             */
            LOG3("Struct type already added to DpdkStructType vector: "
                 << s << std::endl
                 << "Main metadata structure is: " << structure->local_metadata_type);
            BUG_CHECK(structTypeName == structure->local_metadata_type,
                      "Unexpectedly duplicating declaration of: %1%", structTypeName);
        } else {
            LOG3("Adding DpdkStructType: " << s);
            auto st = new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, s->fields);
            structType.push_back(st);
        }
    }
    structType.append(updatedHeaderMetadataStructs);

    IR::IndexedVector<IR::DpdkExternDeclaration> dpdkExternDecls;
    for (auto ed : structure->externDecls) {
        auto st = new IR::DpdkExternDeclaration(ed->name, ed->annotations, ed->type, ed->arguments);
        dpdkExternDecls.push_back(st);
    }

    auto tables = ingress_converter->getTables();
    auto actions = ingress_converter->getActions();
    auto selectors = ingress_converter->getSelectors();
    auto learners = ingress_converter->getLearners();
    if (options.enableEgress) {
        tables.append(egress_converter->getTables());
        actions.append(egress_converter->getActions());
        selectors.append(egress_converter->getSelectors());
        learners.append(egress_converter->getLearners());
    }

    return new IR::DpdkAsmProgram(headerType, structType, dpdkExternDecls, actions, tables,
                                  selectors, learners, statements, structure->get_globals());
}

const IR::Node *ConvertToDpdkProgram::preorder(IR::P4Program *prog) {
    dpdk_program = create(prog);
    return prog;
}

cstring ConvertToDpdkParser::append_parser_name(const IR::P4Parser *p, cstring label) {
    return p->name + "_" + label;
}

IR::Declaration_Variable *ConvertToDpdkParser::addNewTmpVarToMetadata(cstring name,
                                                                      const IR::Type *type) {
    auto newTmpVar = new IR::Declaration_Variable(IR::ID(refmap->newName(name)), type);
    metadataStruct->fields.push_back(
        new IR::StructField(IR::ID(newTmpVar->name.name), newTmpVar->type));
    return newTmpVar;
}

/* This is a helper function for handling the transition select statement. It populates the left
   and right operands of comparison, to be used  in conditional jump instructions. When the keyset
   of select case is simple expressions, it populates the left and rigt operands with the input
   expressions. When the keyset is Mask expression "a &&& b", it inserts temporary variables in
   Metadata structure and populates the left and right operands of comparison with
   "input & b" and "a & b" */
void ConvertToDpdkParser::getCondVars(const IR::Expression *sv, const IR::Expression *ce,
                                      IR::Expression **leftExpr, IR::Expression **rightExpr) {
    if (sv->is<IR::Constant>() && sv->type->width_bits() > 32) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%, Constant expression wider than 32-bit is not permitted", sv);
        return;
    }
    if (sv->type->width_bits() > 64) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%, Select expression wider than 64-bit is not permitted", sv);
        return;
    }
    auto width = sv->type->width_bits();
    auto byteAlignedWidth = (width + 7) & (~7);
    if (auto maskexpr = ce->to<IR::Mask>()) {
        auto left = maskexpr->left;
        auto right = maskexpr->right;
        unsigned value =
            right->to<IR::Constant>()->asUnsigned() & left->to<IR::Constant>()->asUnsigned();
        auto tmpDecl = addNewTmpVarToMetadata("tmpMask", IR::Type_Bits::get(byteAlignedWidth));
        auto tmpMask =
            new IR::Member(new IR::PathExpression(IR::ID("m")), IR::ID(tmpDecl->name.name));
        structure->push_variable(new IR::DpdkDeclaration(tmpDecl));
        add_instr(new IR::DpdkMovStatement(tmpMask, sv));
        add_instr(new IR::DpdkAndStatement(tmpMask, tmpMask, right));
        *leftExpr = tmpMask;
        *rightExpr = new IR::Constant(value);
    } else {
        *leftExpr = sv->clone();
        *rightExpr = ce->clone();
    }
}

/* This is a helper function for handling tuple expressions in select statement in parser block.
   The tuple expressions are evaluated as exact match of each element in the tuple and is
   translated to dpdk assembly by short-circuiting the conditions where beginning of each keyset
   starts with a unique case label.

   transition select(a,b) {              Equivalent dpdk assembly:
       (a1, b1) : state1;
       (a2, b2) : state2;                jmpneq case1 a, a1
       (a3, b3) : state3;                jmpneq case1 b, b1
       }                    ==>          jmp state1
   is evaluated as                       case1:   jmpneq case2 a, a2
   if (a == a1 && b == b1)               jmpneq case2 b, b2
       goto state1                       jmp state2
   else (a == a2 && b == b2)             case2:    jmpneq case a, a3
       goto state2                       jmpneq case2 b, b3
   .....                                 jmp state3
                                         ....
    This function emits a series of jmps for each keyset.
*/
void ConvertToDpdkParser::handleTupleExpression(const IR::ListExpression *cl,
                                                const IR::ListExpression *input, int inputSize,
                                                cstring trueLabel, cstring falseLabel) {
    IR::Expression *left;
    IR::Expression *right;
    /* Compare each element in the input tuple with the keyset tuple */
    for (auto i = 0; i < inputSize; i++) {
        auto switch_var = input->components.at(i);
        auto caseExpr = cl->components.at(i);
        if (!caseExpr->is<IR::DefaultExpression>()) {
            getCondVars(switch_var, caseExpr, &left, &right);
            add_instr(new IR::DpdkJmpNotEqualStatement(falseLabel, left, right));
        }
    }
    add_instr(new IR::DpdkJmpLabelStatement(trueLabel));
}

bool ConvertToDpdkParser::preorder(const IR::P4Parser *p) {
    for (auto l : p->parserLocals) {
        structure->push_variable(new IR::DpdkDeclaration(l));
    }
    ordered_map<cstring, int> degree_map;
    ordered_map<cstring, const IR::ParserState *> state_map;
    std::vector<const IR::ParserState *> stack;
    for (auto state : p->states) {
        if (state->name == "start") stack.push_back(state);
        degree_map.insert({state->name, 0});
        state_map.insert({state->name, state});
    }
    for (auto state : p->states) {
        if (state->selectExpression) {
            if (auto select = state->selectExpression->to<IR::SelectExpression>()) {
                for (auto pair : select->selectCases) {
                    auto got = degree_map.find(pair->state->path->name);
                    if (got != degree_map.end()) {
                        got->second++;
                    }
                }
            } else if (auto path = state->selectExpression->to<IR::PathExpression>()) {
                auto got = degree_map.find(path->path->name);
                if (got != degree_map.end()) {
                    got->second++;
                }
            }
        }
    }
    degree_map.erase("start");
    state_map.erase("start");

    while (stack.size() > 0) {
        auto state = stack.back();
        stack.pop_back();

        // the main body
        auto i = new IR::DpdkLabelStatement(append_parser_name(p, state->name));
        // 'accept' and 'rejet' states are hardcoded to the end of parser.
        if (state->name != IR::ParserState::accept && state->name != IR::ParserState::reject)
            add_instr(i);
        auto c = state->components;
        for (auto stat : c) {
            DPDK::ConvertStatementToDpdk h(refmap, typemap, structure, metadataStruct);
            h.set_parser(p);
            stat->apply(h);
            for (auto i : h.get_instr()) add_instr(i);
        }

        // Handle transition select statement
        if (state->selectExpression) {
            if (auto e = state->selectExpression->to<IR::SelectExpression>()) {
                auto caseList = e->selectCases;
                const IR::Expression *switch_var;
                const IR::Expression *caseExpr;
                IR::Expression *left;
                IR::Expression *right;

                /* Handling single expression in keyset as special case because :
                   - for tuple expression we emit a series of jmpneq followed by an unconditional
                     jump, handling single expression separately by emitting a jmpeq saves us one
                     unconditional jmp
                   - the keyset for single expression is not a ListExpression, so handling it with
                     tuple expressions would anyways require additional and conversion to
                     ListExpression.
                */
                if (e->select->components.size() == 1) {
                    switch_var = e->select->components.at(0);
                    for (auto sc : caseList) {
                        caseExpr = sc->keyset;
                        if (!sc->keyset->is<IR::DefaultExpression>()) {
                            getCondVars(switch_var, caseExpr, &left, &right);
                            add_instr(new IR::DpdkJmpEqualStatement(
                                append_parser_name(p, sc->state->path->name), left, right));
                        } else {
                            add_instr(new IR::DpdkJmpLabelStatement(
                                append_parser_name(p, sc->state->path->name)));
                        }
                    }
                } else {
                    auto tupleInputExpr = e->select;
                    auto inputSize = tupleInputExpr->components.size();
                    cstring trueLabel, falseLabel;
                    /* For each select case, emit the block start label and then a series of
                       jmp instructions. */
                    for (auto sc : caseList) {
                        if (!sc->keyset->is<IR::DefaultExpression>()) {
                            /* Create label names, falseLabel for next keyset comparison and
                               trueLabel for the state to jump on match */
                            falseLabel = refmap->newName(state->name);
                            trueLabel = sc->state->path->name;
                            handleTupleExpression(sc->keyset->to<IR::ListExpression>(),
                                                  tupleInputExpr, inputSize,
                                                  append_parser_name(p, trueLabel),
                                                  append_parser_name(p, falseLabel));
                            add_instr(
                                new IR::DpdkLabelStatement(append_parser_name(p, falseLabel)));
                        } else {
                            add_instr(new IR::DpdkJmpLabelStatement(
                                append_parser_name(p, sc->state->path->name)));
                        }
                    }
                }
            } else if (auto path = state->selectExpression->to<IR::PathExpression>()) {
                auto i = new IR::DpdkJmpLabelStatement(append_parser_name(p, path->path->name));
                add_instr(i);
            } else {
                BUG("P4 Parser switch statement has other situations.");
            }
        }
        // ===========
        if (state->selectExpression) {
            if (auto select = state->selectExpression->to<IR::SelectExpression>()) {
                for (auto pair : select->selectCases) {
                    auto result = degree_map.find(pair->state->path->name);
                    if (result != degree_map.end()) {
                        result->second--;
                    }
                }
            } else if (auto path = state->selectExpression->to<IR::PathExpression>()) {
                auto got = degree_map.find(path->path->name);
                if (got != degree_map.end()) {
                    got->second--;
                }
            }
        }

        std::set<cstring> node_to_remove;
        for (auto &it : degree_map) {
            auto degree = it.second;
            if (degree == 0) {
                auto result = state_map.find(it.first);
                if (result != state_map.end()) {
                    stack.push_back(result->second);
                    node_to_remove.insert(it.first);
                }
            }
        }
        for (auto node : node_to_remove) {
            degree_map.erase(node);
        }

        if (degree_map.size() > 0 && stack.size() == 0) BUG("Unsupported parser loop");

        if (state->name == "start") continue;
    }

    add_instr(new IR::DpdkLabelStatement(append_parser_name(p, IR::ParserState::reject)));
    add_instr(new IR::DpdkLabelStatement(append_parser_name(p, IR::ParserState::accept)));
    return false;
}

bool ConvertToDpdkParser::preorder(const IR::ParserState *) { return false; }

// =====================Control=============================
bool ConvertToDpdkControl::preorder(const IR::P4Action *a) {
    auto helper = new DPDK::ConvertStatementToDpdk(refmap, typemap, structure, metadataStruct);
    helper->setCalledBy(this);
    a->body->apply(*helper);
    auto stmt_list = new IR::IndexedVector<IR::DpdkAsmStatement>();
    for (auto i : helper->get_instr()) stmt_list->push_back(i);

    auto actName = a->name.name;
    if (a->name.originalName == "NoAction") actName = "NoAction";
    auto action = new IR::DpdkAction(*stmt_list, actName, *a->parameters);
    actions.push_back(action);
    return false;
}

/* This function checks if a table satisfies the DPDK limitations mentioned below:
     - Only one LPM match field allowed per table.
     - If there is a key field with lpm match kind, the other match fields, if any,
       must all be exact match.
*/
bool ConvertToDpdkControl::checkTableValid(const IR::P4Table *a) {
    auto keys = a->getKey();
    auto lpmCount = 0;
    auto nonExactCount = 0;

    if (!keys || keys->keyElements.size() == 0) {
        return true;
    }

    for (auto key : keys->keyElements) {
        auto matchKind = key->matchType->toString();
        if (matchKind == "lpm") {
            ++lpmCount;
        } else if (matchKind != "exact") {
            ++nonExactCount;
        }
    }

    if (lpmCount > 1) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "Only one LPM match field is permitted per table, "
                "more than one lpm field found in table (%1%)",
                a->name.toString());
        return false;
    } else if (lpmCount == 1 && nonExactCount > 0) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "Non 'exact' match kind not permitted in table (%1%) "
                "with 'lpm' match kind",
                a->name.toString());
        return false;
    }
    return true;
}

std::optional<const IR::Member *> ConvertToDpdkControl::getMemExprFromProperty(
    const IR::P4Table *table, cstring propertyName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return std::nullopt;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%", propertyName,
                table->controlPlaneName(), property);
        return std::nullopt;
    }
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (!expr->is<IR::Member>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Exprected %1% property value for table %2% to be a member", propertyName,
                table->controlPlaneName());
        return std::nullopt;
    }

    return expr->to<IR::Member>();
}

std::optional<int> ConvertToDpdkControl::getNumberFromProperty(const IR::P4Table *table,
                                                               cstring propertyName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return std::nullopt;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%", propertyName,
                table->controlPlaneName(), property);
        return std::nullopt;
    }
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (!expr->is<IR::Constant>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Exprected %1% property value for table %2% to be a constant", propertyName,
                table->controlPlaneName());
        return std::nullopt;
    }

    return expr->to<IR::Constant>()->asInt();
}

bool ConvertToDpdkControl::preorder(const IR::P4Table *t) {
    if (!checkTableValid(t)) return false;

    if (t->properties->getProperty("selector") != nullptr) {
        auto group_id = getMemExprFromProperty(t, "group_id");
        auto member_id = getMemExprFromProperty(t, "member_id");
        auto selector_key = t->properties->getProperty("selector");
        auto n_groups_max = getNumberFromProperty(t, "n_groups_max");
        auto n_members_per_group_max = getNumberFromProperty(t, "n_members_per_group_max");

        if (group_id == std::nullopt || member_id == std::nullopt || n_groups_max == std::nullopt ||
            n_members_per_group_max == std::nullopt)
            return false;

        auto selector = new IR::DpdkSelector(t->name, (*group_id)->clone(), (*member_id)->clone(),
                                             selector_key->value->to<IR::Key>(), *n_groups_max,
                                             *n_members_per_group_max);

        selectors.push_back(selector);
    } else if (structure->learner_tables.count(t->name.name) != 0) {
        auto learner = new IR::DpdkLearner(t->name.toString(), t->getKey(), t->getActionList(),
                                           t->getDefaultAction(), t->properties);
        learners.push_back(learner);
    } else {
        auto paramList = structure->defActionParamList[t->toString()];
        auto table = new IR::DpdkTable(t->name.toString(), t->getKey(), t->getActionList(),
                                       t->getDefaultAction(), t->properties, *paramList);
        tables.push_back(table);
    }
    return false;
}

bool ConvertToDpdkControl::preorder(const IR::P4Control *c) {
    LOG3("P4Control: " << dbp(c) << std::endl << c);
    for (auto l : c->controlLocals) {
        if (!l->is<IR::P4Action>() && !l->is<IR::P4Table>()) {
            structure->push_variable(new IR::DpdkDeclaration(l));
        }
    }
    auto helper = new DPDK::ConvertStatementToDpdk(refmap, typemap, structure, metadataStruct);
    helper->setCalledBy(this);
    c->body->apply(*helper);
    if (deparser && structure->isPSA()) {
        add_inst(new IR::DpdkJmpNotEqualStatement(
            "LABEL_DROP",
            new IR::Member(new IR::PathExpression("m"), "psa_ingress_output_metadata_drop"),
            new IR::Constant(0)));
    }

    for (auto i : helper->get_instr()) {
        add_inst(i);
        LOG3("Adding instruction: " << i);
    }

    return true;
}
}  // namespace DPDK
