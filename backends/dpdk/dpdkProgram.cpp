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

const IR::DpdkAsmProgram *ConvertToDpdkProgram::create(IR::P4Program *prog) {

    IR::Type_Struct *metadataStruct = nullptr;

    for (auto obj : prog->objects) {
        if (auto s = obj->to<IR::Type_Struct>()) {
            if (s->name.name == info->local_metadata_type) {
                metadataStruct = s->clone();
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
    IR::IndexedVector<IR::DpdkStructType> structType;

    /* Insert the metadata structure updated with tmp variables created during parser conversion */
    auto new_objs = new IR::Vector<IR::Node>;
    for (auto obj : prog->objects) {
        if (auto s = obj->to<IR::Type_Struct>()) {
            if (s->name.name == info->local_metadata_type) {
                auto *annotations = new IR::Annotations(
                    {new IR::Annotation(IR::ID("__metadata__"), {})});
                for (auto anno : s->annotations->annotations)
                    annotations->add(anno);
                new_objs->push_back(metadataStruct);
                auto st = new IR::DpdkStructType(s->srcInfo, s->name,
                                                 annotations, metadataStruct->fields);
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

    for (auto kv : structure.metadata_types) {
        auto s = kv.second;
        auto st = new IR::DpdkStructType(s->srcInfo, s->name, s->annotations,
                                         s->fields);
        structType.push_back(st);
    }

    IR::IndexedVector<IR::DpdkExternDeclaration> externDecls;
    for (auto kv : *reg_map) {
        auto s = kv.first;
        auto st = new IR::DpdkExternDeclaration(s->name, s->annotations, s->type, s->arguments);
        externDecls.push_back(st);
    }

    return new IR::DpdkAsmProgram(
        headerType, structType, externDecls, ingress_converter->getActions(),
        ingress_converter->getTables(), statements, collector->get_globals());
}

const IR::Node *ConvertToDpdkProgram::preorder(IR::P4Program *prog) {
    // std::cout << prog << std::endl;
    dpdk_program = create(prog);
    return prog;
}

cstring ConvertToDpdkParser::append_parser_name(const IR::P4Parser *p, cstring label) {
    return p->name + "_" + label;
}

void ConvertToDpdkParser::add_metadata_field(IR::Declaration_Variable *d) {
    metadataStruct->fields.push_back(new IR::StructField(IR::ID(d->name.name), d->type));
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
        if (state->selectExpression) {
            if (auto e = state->selectExpression->to<IR::SelectExpression>()) {
                const IR::Expression *switch_var;
                BUG_CHECK(e->select->components.size() == 1,
                        "Unsupported select expression %1%", e->select);
                switch_var = e->select->components[0];
                for (auto v : e->selectCases) {
                    if (!v->keyset->is<IR::DefaultExpression>()) {
                        if (v->keyset->is<IR::Mask>()) {
                            auto maskexpr = v->keyset->to<IR::Mask>();
                            auto left = maskexpr->left;
                            auto right = maskexpr->right;
                            /* Dpdk architecture requires that both operands of &&& be constants */
                            if (!left->is<IR::Constant>() || !left->is<IR::Constant>())
                                ::error(ErrorType::ERR_UNSUPPORTED,
                                        "Non constant values are not supported in Mask operation");


                            unsigned value = right->to<IR::Constant>()->asUnsigned() &
                                             left->to<IR::Constant>()->asUnsigned();
                            auto tmpDecl = new IR::Declaration_Variable(IR::ID(
                                               refmap->newName("tmpMask")), switch_var->type);
                            auto tmpMask = new IR::PathExpression(
                                               IR::ID("m." + tmpDecl->name.name));
                            collector->push_variable(new IR::DpdkDeclaration(tmpDecl));
                            add_metadata_field(tmpDecl);
                            add_instr(new IR::DpdkMovStatement(tmpMask, switch_var));
                            add_instr(new IR::DpdkAndStatement(tmpMask, tmpMask, right));
                            add_instr(new IR::DpdkJmpEqualStatement(
                                         append_parser_name(p, v->state->path->name),
                                                            tmpMask, new IR::Constant(value)));
                        } else {
                            add_instr(new IR::DpdkJmpEqualStatement(
                              append_parser_name(p, v->state->path->name), switch_var, v->keyset));
                        }
                    } else {
                        auto i = new IR::DpdkJmpLabelStatement(
                                append_parser_name(p, v->state->path->name));
                        add_instr(i);
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

bool ConvertToDpdkControl::preorder(const IR::P4Table *a) {
    auto t = new IR::DpdkTable(a->name.toString(), a->getKey(), a->getActionList(),
                               a->getDefaultAction(), a->properties);
    tables.push_back(t);
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
