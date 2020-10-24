#include "dpdkProgram.h"
#include <unordered_map>
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
  IR::IndexedVector<IR::DpdkHeaderType> headerType;
  for (auto kv : structure.header_types) {
    // std::cout << kv.second << std::endl;
    auto h = kv.second;
    auto ht =
        new IR::DpdkHeaderType(h->srcInfo, h->name, h->annotations, h->fields);
    headerType.push_back(ht);
  }
  IR::IndexedVector<IR::DpdkStructType> structType;
  for (auto obj : prog->objects) {
    if (auto s = obj->to<IR::Type_Struct>()) {
      if (s->name.name == info->local_metadata_type) {
        auto *annotations = new IR::Annotations(
            {new IR::Annotation(IR::ID("__metadata__"), {})});
        for (auto anno : s->annotations->annotations)
          annotations->add(anno);
        auto st =
            new IR::DpdkStructType(s->srcInfo, s->name, annotations, s->fields);
        structType.push_back(st);
      } else if (args_struct_map->find(s->name.name) !=
                 args_struct_map->end()) {
        auto st = new IR::DpdkArgStructType(s->srcInfo, s->name, s->annotations,
                                            s->fields);
        structType.push_back(st);
      } else if (s->name.name == info->header_type) {
        auto *annotations = new IR::Annotations(
            {new IR::Annotation(IR::ID("__packet_data__"), {})});
        for (auto anno : s->annotations->annotations)
          annotations->add(anno);
        auto st = new IR::DpdkHeaderStructType(s->srcInfo, s->name,
                                               s->annotations, s->fields);
        structType.push_back(st);
      }
    }
  }

  for (auto kv : structure.metadata_types) {
    auto s = kv.second;
    auto st =
        new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, s->fields);
    structType.push_back(st);
  }

  IR::IndexedVector<IR::DpdkAsmStatement> statements;
  auto ingress_parser_converter =
      new ConvertToDpdkParser(refmap, typemap, collector, csum_map);
  auto egress_parser_converter =
      new ConvertToDpdkParser(refmap, typemap, collector, csum_map);
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
      new ConvertToDpdkControl(refmap, typemap, collector, csum_map);
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
  auto s = createListStatement("ingress",
                               {ingress_parser_converter->getInstructions(),
                                ingress_converter->getInstructions(),
                                ingress_deparser_converter->getInstructions(),
                                egress_parser_converter->getInstructions(),
                                egress_converter->getInstructions(),
                                egress_deparser_converter->getInstructions()});
  statements.push_back(s);

  return new IR::DpdkAsmProgram(
      headerType, structType, ingress_converter->getActions(),
      ingress_converter->getTables(), statements, collector->get_globals());
}

const IR::Node *ConvertToDpdkProgram::preorder(IR::P4Program *prog) {
  // std::cout << prog << std::endl;
  dpdk_program = create(prog);
  return prog;
}

bool ConvertToDpdkParser::preorder(const IR::P4Parser *p) {
  for (auto l : p->parserLocals) {
    collector->push_variable(new IR::DpdkDeclaration(l));
  }
  std::unordered_map<cstring, int> degree_map;
  std::unordered_map<cstring, IR::ParserState> state_map;
  std::vector<IR::ParserState> stack;
  for (auto state : p->states) {
    if (state->name == "start")
      stack.push_back(*state);
    degree_map.insert({state->name.toString(), 0});
    state_map.insert({state->name.toString(), *state});
  }
  for (auto state : p->states) {
    if (state->selectExpression) {
      if (state->selectExpression->is<IR::SelectExpression>()) {
        auto select = state->selectExpression->to<IR::SelectExpression>();
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
    auto i = new IR::DpdkLabelStatement("L_" + state.name.toString());
    add_instr(i);
    auto c = state.components;
    for (auto stat : c) {
      DPDK::ConvertStatementToDpdk h(refmap, typemap, 0, this->collector,
                                     csum_map);
      stat->apply(h);
      for (auto i : h.get_instr())
        add_instr(i);
    }
    if (state.selectExpression) {
      if (auto e = state.selectExpression->to<IR::SelectExpression>()) {
        const IR::Expression *switch_var;
        switch_var = e->select->components[0];
        for (auto v : e->selectCases) {
          if (!v->keyset->is<IR::DefaultExpression>()) {
            add_instr(new IR::DpdkJmpEqualStatement("L_" + v->state->path->name,
                                                    v->keyset, switch_var));
          } else {
            auto i = new IR::DpdkJmpStatement("L_" + v->state->path->name);
            add_instr(i);
          }
        }
      } else if (auto p = state.selectExpression->to<IR::PathExpression>()) {
        auto i = new IR::DpdkJmpStatement("L_" + p->path->name);
        add_instr(i);
      } else {
        BUG("P4 Parser switch statement has other situations.");
      }
    }
    // ===========
    if (state.selectExpression) {
      if (state.selectExpression->is<IR::SelectExpression>()) {
        auto select = state.selectExpression->to<IR::SelectExpression>();
        for (auto pair : select->selectCases) {
          auto result = degree_map.find(pair->state->toString());
          if (result != degree_map.end()) {
            result->second--;
          }
        }
      } else if (state.selectExpression->is<IR::PathExpression>()) {
        auto got = degree_map.find(state.selectExpression->toString());
        if (got != degree_map.end()) {
          got->second--;
        }
      }
    }

    for (auto pair : degree_map) {
      if (pair.second == 0) {
        auto result = state_map.find(pair.first);
        if (result != state_map.end()) {
          stack.push_back(result->second);
          degree_map.erase(result->first);
        }
      }
    }

    if (state.name == "start")
      continue;
  }
  return false;
}

bool ConvertToDpdkParser::preorder(const IR::ParserState *) { return false; }

// =====================Control=============================
bool ConvertToDpdkControl::preorder(const IR::P4Action *a) {
  auto helper =
      new DPDK::ConvertStatementToDpdk(refmap, typemap, 0, collector, csum_map);
  a->body->apply(*helper);
  auto stmt_list = new IR::IndexedVector<IR::DpdkAsmStatement>();
  for (auto i : helper->get_instr())
    stmt_list->push_back(i);

  auto action = new IR::DpdkAction(*stmt_list, a->name, *a->parameters);
  actions.push_back(action);
  return false;
}

bool ConvertToDpdkControl::preorder(const IR::P4Table *a) {
  // auto psa_implementation = a->properties->getProperty("psa_implementation");
  auto t = new IR::DpdkTable(a->name, a->getKey(), a->getActionList(),
                             a->getDefaultAction(), a->properties);
  tables.push_back(t);
  return false;
}

bool ConvertToDpdkControl::preorder(const IR::P4Control *c) {
  for (auto l : c->controlLocals) {
    if (!l->is<IR::P4Action>() && !l->is<IR::P4Table>()) {
      // std::cout << l << std::endl;
      collector->push_variable(new IR::DpdkDeclaration(l));
    }
  }
  auto helper =
      new DPDK::ConvertStatementToDpdk(refmap, typemap, 0, collector, csum_map);
  c->body->apply(*helper);
  for (auto i : helper->get_instr()) {
    add_inst(i);
  }
  return true;
}
}  // namespace DPDK
