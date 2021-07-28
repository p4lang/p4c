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

#include <unordered_map>
#include "dpdkProgram.h"
#include "backend.h"
#include "backends/bmv2/psa_switch/psaSwitch.h"
#include "dpdkHelpers.h"
#include "ir/dbprint.h"
#include "ir/ir.h"
#include "lib/stringify.h"

namespace DPDK {
const IR::DpdkAsmStatement *ConvertToDpdkProgram::createListStatement(
    cstring name,
    std::initializer_list<IR::IndexedVector<IR::DpdkAsmStatement>> list) {

    auto stmts = new IR::IndexedVector<IR::DpdkAsmStatement>();
    for (auto l : list) {
        stmts->append(l);
    }
    return new IR::DpdkListStatement(name, *stmts);
}

/* Insert the metadata structure updated with tmp variables created during parser conversion
   Add annotations to metadata and header structures and add all the structures to DPDK
   structtype.
*/
IR::IndexedVector<IR::DpdkStructType> ConvertToDpdkProgram::UpdateHeaderMetadata
                                      (IR::P4Program *prog, IR::Type_Struct *metadata) {
    IR::IndexedVector<IR::DpdkStructType> structType;
    auto new_objs = new IR::Vector<IR::Node>;
    for (auto obj : prog->objects) {
        if (auto s = obj->to<IR::Type_Struct>()) {
            if (s->name.name == info->local_metadata_type) {
                auto *annotations = new IR::Annotations(
                    {new IR::Annotation(IR::ID("__metadata__"), {})});
                for (auto anno : s->annotations->annotations)
                    annotations->add(anno);
                new_objs->push_back(metadata);
                auto st = new IR::DpdkStructType(s->srcInfo, s->name,
                                                 annotations, metadata->fields);
                structType.push_back(st);
            } else {
                if (args_struct_map->find(s->name.name) !=
                         args_struct_map->end()) {
                    auto st = new IR::DpdkStructType(s->srcInfo, s->name,
                                                 s->annotations, s->fields);
                   structType.push_back(st);
                } else if (s->name.name == info->header_type) {
                    auto *annotations = new IR::Annotations(
                        {new IR::Annotation(IR::ID("__packet_data__"), {})});
                    for (auto anno : s->annotations->annotations)
                         annotations->add(anno);
                    auto st = new IR::DpdkStructType(s->srcInfo, s->name,
                                                 s->annotations, s->fields);
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

const IR::DpdkAsmProgram *ConvertToDpdkProgram::create(IR::P4Program *prog) {
    IR::Type_Struct *metadataStruct = nullptr;

    for (auto obj : prog->objects) {
        if (auto s = obj->to<IR::Type_Struct>()) {
            if (s->name.name == info->local_metadata_type) {
                metadataStruct = s->clone();
                break;
            }
        }
    }

    IR::IndexedVector<IR::DpdkAsmStatement> statements;

    auto ingress_parser_converter =
        new ConvertToDpdkParser(refmap, typemap, collector, csum_map, metadataStruct);
    auto egress_parser_converter =
        new ConvertToDpdkParser(refmap, typemap, collector, csum_map, metadataStruct);
    for (auto kv : structure.parsers) {
        if (kv.first == "ingress")
            kv.second->apply(*ingress_parser_converter);
        else if (kv.first == "egress")
            kv.second->apply(*egress_parser_converter);
        else
            BUG("Unknown parser %s", kv.second->name);
    }
    auto ingress_converter =
        new ConvertToDpdkControl(refmap, typemap, collector, csum_map);
    auto egress_converter =
        new ConvertToDpdkControl(refmap, typemap, collector, csum_map);
    for (auto kv : structure.pipelines) {
        if (kv.first == "ingress")
            kv.second->apply(*ingress_converter);
        else if (kv.first == "egress")
            kv.second->apply(*egress_converter);
        else
            BUG("Unknown control block %s", kv.second->name);
    }
    auto ingress_deparser_converter =
        new ConvertToDpdkControl(refmap, typemap, collector, csum_map, true);
    auto egress_deparser_converter =
        new ConvertToDpdkControl(refmap, typemap, collector, csum_map);
    for (auto kv : structure.deparsers) {
        if (kv.first == "ingress")
            kv.second->apply(*ingress_deparser_converter);
        else if (kv.first == "egress")
            kv.second->apply(*egress_deparser_converter);
        else
            BUG("Unknown deparser block %s", kv.second->name);
    }

    auto s = createListStatement(
        "ingress", {ingress_parser_converter->getInstructions(),
                    ingress_converter->getInstructions(),
                    ingress_deparser_converter->getInstructions(),
                    egress_parser_converter->getInstructions(),
                    egress_converter->getInstructions(),
                    egress_deparser_converter->getInstructions(),
                    });
    statements.push_back(s);

    IR::IndexedVector<IR::DpdkHeaderType> headerType;
    for (auto kv : structure.header_types) {
        LOG3("add header type " << kv.second);
        auto h = kv.second;
        auto ht = new IR::DpdkHeaderType(h->srcInfo, h->name, h->annotations,
                                         h->fields);
        headerType.push_back(ht);
    }
    IR::IndexedVector<IR::DpdkStructType> structType = UpdateHeaderMetadata(prog, metadataStruct);

    for (auto kv : structure.metadata_types) {
        auto s = kv.second;
        auto st = new IR::DpdkStructType(s->srcInfo, s->name, s->annotations,
                                         s->fields);
        structType.push_back(st);
    }

    IR::IndexedVector<IR::DpdkExternDeclaration> dpdkExternDecls;
    for (auto ed : *externDecls) {
        auto st = new IR::DpdkExternDeclaration(ed->name, ed->annotations, ed->type, ed->arguments);
        dpdkExternDecls.push_back(st);
    }

    return new IR::DpdkAsmProgram(
        headerType, structType, dpdkExternDecls, ingress_converter->getActions(),
        ingress_converter->getTables(), ingress_converter->getSelectors(),
        statements, collector->get_globals());
}

const IR::Node *ConvertToDpdkProgram::preorder(IR::P4Program *prog) {
    // std::cout << prog << std::endl;
    dpdk_program = create(prog);
    return prog;
}

cstring ConvertToDpdkParser::append_parser_name(const IR::P4Parser *p, cstring label) {
    return p->name + "_" + label;
}

IR::Declaration_Variable *ConvertToDpdkParser::addNewTmpVarToMetadata(cstring name,
                                                                       const IR::Type* type) {
    auto newTmpVar =  new IR::Declaration_Variable(IR::ID(
                                               refmap->newName(name)), type);
    metadataStruct->fields.push_back(new IR::StructField(IR::ID(newTmpVar->name.name),
                                                         newTmpVar->type));
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
    if (auto maskexpr = ce->to<IR::Mask>()) {
        auto left = maskexpr->left;
        auto right = maskexpr->right;
        unsigned value = right->to<IR::Constant>()->asUnsigned() &
                         left->to<IR::Constant>()->asUnsigned();
        auto tmpDecl = addNewTmpVarToMetadata("tmpMask", sv->type);
        auto tmpMask = new IR::PathExpression(
                           IR::ID("m." + tmpDecl->name.name));
        collector->push_variable(new IR::DpdkDeclaration(tmpDecl));
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
        collector->push_variable(new IR::DpdkDeclaration(l));
    }
    ordered_map<cstring, int> degree_map;
    ordered_map<cstring, const IR::ParserState*> state_map;
    std::vector<const IR::ParserState*> stack;
    for (auto state : p->states) {
        if (state->name == "start")
            stack.push_back(state);
        degree_map.insert({state->name.toString(), 0});
        state_map.insert({state->name.toString(), state});
    }
    for (auto state : p->states) {
        if (state->selectExpression) {
            if (state->selectExpression->is<IR::SelectExpression>()) {
                auto select =
                    state->selectExpression->to<IR::SelectExpression>();
                for (auto pair : select->selectCases) {
                    auto got = degree_map.find(pair->state->path->name);
                    if (got != degree_map.end()) {
                        got->second++;
                    }
                }
            } else if (auto path =
                           state->selectExpression->to<IR::PathExpression>()) {
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
            DPDK::ConvertStatementToDpdk h(refmap, typemap, this->collector,
                                           csum_map);
            stat->apply(h);
            for (auto i : h.get_instr())
                add_instr(i);
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
                        if (!sc->keyset->is<IR::DefaultExpression>()){
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
                                                  tupleInputExpr, inputSize, append_parser_name(p,
                                                  trueLabel), append_parser_name(p, falseLabel));
                            add_instr(new IR::DpdkLabelStatement(
                                      append_parser_name(p, falseLabel)));
                        } else {
                            add_instr(new IR::DpdkJmpLabelStatement(
                                      append_parser_name(p, sc->state->path->name)));
                        }
                    }
                }
            } else if (auto path =
                           state->selectExpression->to<IR::PathExpression>()) {
                auto i = new IR::DpdkJmpLabelStatement(
                        append_parser_name(p, path->path->name));
                add_instr(i);
            } else {
                BUG("P4 Parser switch statement has other situations.");
            }
        }
        // ===========
        if (state->selectExpression) {
            if (state->selectExpression->is<IR::SelectExpression>()) {
                auto select =
                    state->selectExpression->to<IR::SelectExpression>();
                for (auto pair : select->selectCases) {
                    auto result = degree_map.find(pair->state->toString());
                    if (result != degree_map.end()) {
                        result->second--;
                    }
                }
            } else if (state->selectExpression->is<IR::PathExpression>()) {
                auto got = degree_map.find(state->selectExpression->toString());
                if (got != degree_map.end()) {
                    got->second--;
                }
            }
        }

        std::set<cstring> node_to_remove;
        for (auto& it : degree_map) {
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

        if (degree_map.size() > 0 && stack.size() == 0)
            BUG("Unsupported parser loop");

        if (state->name == "start")
            continue;
    }

    add_instr(new IR::DpdkLabelStatement(append_parser_name(p, IR::ParserState::reject)));
    add_instr(new IR::DpdkLabelStatement(append_parser_name(p, IR::ParserState::accept)));
    return false;
}

bool ConvertToDpdkParser::preorder(const IR::ParserState *) { return false; }

// =====================Control=============================
bool ConvertToDpdkControl::preorder(const IR::P4Action *a) {
    auto helper = new DPDK::ConvertStatementToDpdk(refmap, typemap,
                                                   collector, csum_map);
    a->body->apply(*helper);
    auto stmt_list = new IR::IndexedVector<IR::DpdkAsmStatement>();
    for (auto i : helper->get_instr())
        stmt_list->push_back(i);

    auto action = new IR::DpdkAction(*stmt_list, a->name, *a->parameters);
    actions.push_back(action);
    return false;
}

/* This function checks if a table satisfies the DPDK limitations mentioned below:
     - Only one LPM match field allowed per table.
     - Maximum allowed key size of header/metadata field is 64 bits.
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
        /* Maximum allowed key size of header/metadata field is 64 bits */
        if (key->expression->type->width_bits() > DPDK_MAX_HEADER_METADATA_FIELD_SIZE) {
            ::error(ErrorType::ERR_UNEXPECTED, "Key field wider than 64-bit is not permitted %1%",
                    key->expression);
            return false;
        }

        auto matchKind = key->matchType->toString();
        if (matchKind == "lpm") {
            ++lpmCount;
        } else if (matchKind != "exact") {
            ++nonExactCount;
        }
    }

    if (lpmCount > 1) {
        ::error(ErrorType::ERR_UNEXPECTED, "Only one LPM match field is permitted per table, "
                "more than one lpm field found in table (%1%)", a->name.toString());
        return false;
    } else if (lpmCount == 1 && nonExactCount > 0) {
        ::error(ErrorType::ERR_UNEXPECTED, "Non 'exact' match kind not permitted in table (%1%) "
                "with 'lpm' match kind", a->name.toString());
        return false;
    }
    return true;
}

boost::optional<cstring> ConvertToDpdkControl::getIdFromProperty(const IR::P4Table* table,
                                                                 cstring propertyName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return boost::none;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return boost::none;
    }
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (!expr->is<IR::Member>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Exprected %1% property value for table %2% to be a member",
                propertyName, table->controlPlaneName());
        return boost::none;
    }

    return expr->to<IR::Member>()->toString();
}

boost::optional<int> ConvertToDpdkControl::getNumberFromProperty(const IR::P4Table* table,
                                                                 cstring propertyName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return boost::none;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return boost::none;
    }
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (!expr->is<IR::Constant>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Exprected %1% property value for table %2% to be a constant",
                propertyName, table->controlPlaneName());
        return boost::none;
    }

    return expr->to<IR::Constant>()->asInt();
}


bool ConvertToDpdkControl::preorder(const IR::P4Table *t) {
    if (checkTableValid(t)) {
        if (t->properties->getProperty("selector") != nullptr) {
            auto group_id = getIdFromProperty(t, "group_id");
            auto member_id = getIdFromProperty(t, "member_id");
            auto selector_key = t->properties->getProperty("selector");
            auto n_groups_max = getNumberFromProperty(t, "n_groups_max");
            auto n_members_per_group_max = getNumberFromProperty(t, "n_members_per_group_max");

            if (group_id == boost::none || member_id == boost::none ||
                n_groups_max == boost::none || n_members_per_group_max == boost::none)
                return false;

            auto selector = new IR::DpdkSelector(t->name,
                *group_id, *member_id, selector_key->value->to<IR::Key>(),
                *n_groups_max, *n_members_per_group_max);

            selectors.push_back(selector);
        } else {
            auto table = new IR::DpdkTable(t->name.toString(), t->getKey(), t->getActionList(),
                    t->getDefaultAction(), t->properties);
            tables.push_back(table);
        }
    }
    return false;
}

bool ConvertToDpdkControl::preorder(const IR::P4Control *c) {
    for (auto l : c->controlLocals) {
        if (!l->is<IR::P4Action>() && !l->is<IR::P4Table>()) {
            collector->push_variable(new IR::DpdkDeclaration(l));
        }
    }
    auto helper = new DPDK::ConvertStatementToDpdk(refmap, typemap,
                                                   collector, csum_map);
    c->body->apply(*helper);
    if (deparser) {
        add_inst(new IR::DpdkJmpNotEqualStatement("LABEL_DROP",
            new IR::Member(new IR::PathExpression("m"), "psa_ingress_output_metadata_drop"),
            new IR::Constant(0))); }

    for (auto i : helper->get_instr()) {
        add_inst(i);
    }

    return true;
}
}  // namespace DPDK
