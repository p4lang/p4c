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

/**
 * DPDK architecture assume the following control block signature
 *
 * control ingress(header h, metadata m);
 * control egress(header h, metadata m);
 *
 * We need to convert psa control blocks to this form.
 */

#include "dpdkArch.h"

namespace DPDK {

bool isSimpleExpression(const IR::Expression *e) {
  if (e->is<IR::Member>() || e->is<IR::PathExpression>() ||
      e->is<IR::Constant>() || e->is<IR::BoolLiteral>())
    return true;
  return false;
}
bool isNonConstantSimpleExpression(const IR::Expression *e) {
  if (e->is<IR::Member>() || e->is<IR::PathExpression>())
    return true;
  return false;
}

cstring TypeStruct2Name(const cstring s) {
  if (s == "psa_ingress_parser_input_metadata_t") {
    return "psa_ingress_parser_input_metadata";
  } else if (s == "psa_ingress_input_metadata_t") {
    return "psa_ingress_input_metadata";
  } else if (s == "psa_ingress_output_metadata_t") {
    return "psa_ingress_output_metadata";
  } else if (s == "psa_egress_parser_input_metadata_t") {
    return "psa_egress_parser_input_metadata";
  } else if (s == "psa_egress_input_metadata_t") {
    return "psa_egress_input_metadata";
  } else if (s == "psa_egress_output_metadata_t") {
    return "psa_egress_output_metadata";
  } else if (s == "psa_egress_deparser_input_metadata_t") {
    return "psa_egress_deparser_input_metadata";
  } else {
    return "local_metadata";
  }
}

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Program *p) {
  // std::cout << p << std::endl;
  return p;
}

const IR::Type_Control *
ConvertToDpdkArch::rewriteControlType(const IR::Type_Control *c, cstring name) {
  auto applyParams = new IR::ParameterList();
  if (name == "Ingress" || name == "Egress") {
    applyParams->push_back(c->applyParams->parameters.at(0));
    applyParams->push_back(c->applyParams->parameters.at(1));
  } else if (name == "IngressDeparser") {
    applyParams->push_back(c->applyParams->parameters.at(0));
    applyParams->push_back(c->applyParams->parameters.at(4));
    applyParams->push_back(c->applyParams->parameters.at(5));
  } else if (name == "EgressDeparser") {
    applyParams->push_back(c->applyParams->parameters.at(0));
    applyParams->push_back(c->applyParams->parameters.at(3));
    applyParams->push_back(c->applyParams->parameters.at(4));
  }
  auto tc = new IR::Type_Control(c->name, c->annotations, c->typeParameters,
                                 applyParams);
  return tc;
}

// translate control block signature in arch.p4
const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Control *c) {
  auto ctxt = findOrigCtxt<IR::P4Control>();
  if (ctxt)
    return c;
  return rewriteControlType(c, c->name);
}

const IR::Type_Parser *
ConvertToDpdkArch::rewriteParserType(const IR::Type_Parser *p, cstring name) {
  auto applyParams = new IR::ParameterList();
  if (name == "IngressParser" || name == "EgressParser") {
    applyParams->push_back(p->applyParams->parameters.at(0));
    applyParams->push_back(p->applyParams->parameters.at(1));
    applyParams->push_back(p->applyParams->parameters.at(2));
  }
  auto tp = new IR::Type_Parser(p->name, p->annotations, p->typeParameters,
                                applyParams);

  return tp;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Parser *p) {
  auto ctxt = findOrigCtxt<IR::P4Parser>();
  if (ctxt)
    return p;
  return rewriteParserType(p, p->name);
}

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Control *c) {
  auto orig = getOriginal();
  if (block_info->count(orig) != 0) {
    auto bi = block_info->at(orig);
    LOG1("bi " << bi.pipe << " " << bi.gress << " " << bi.block);
    auto tc = rewriteControlType(c->type, bi.pipe);
    auto cont = new IR::P4Control(c->name, tc, c->constructorParams,
                                  c->controlLocals, c->body);
    LOG1(cont);
    return cont;
  }
  return c;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Parser *p) {
  auto orig = getOriginal();
  if (block_info->count(orig) != 0) {
    auto bi = block_info->at(orig);
    LOG1("bi " << bi.pipe << " " << bi.gress << " " << bi.block);
    auto tp = rewriteParserType(p->type, bi.pipe);
    auto prsr = new IR::P4Parser(p->name, tp, p->constructorParams,
                                 p->parserLocals, p->states);
    LOG1(prsr);
    return prsr;
  }
  return p;
}

void ParsePsa::parseIngressPipeline(const IR::PackageBlock *block) {
  auto ingress_parser = block->getParameterValue("ip");
  if (!ingress_parser->is<IR::ParserBlock>())
      ::error(ErrorType::ERR_UNEXPECTED, "Expected ParserBlock for %1%", ingress_parser);
  BlockInfo ip("IngressParser", INGRESS, PARSER);
  toBlockInfo.emplace(ingress_parser->to<IR::ParserBlock>()->container, ip);

  auto ingress = block->getParameterValue("ig");
  if (!ingress->is<IR::ControlBlock>())
      ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%", ingress);
  BlockInfo ig("Ingress", INGRESS, PIPELINE);
  toBlockInfo.emplace(ingress->to<IR::ControlBlock>()->container, ig);

  auto ingress_deparser = block->getParameterValue("id");
  if (!ingress_deparser->is<IR::ControlBlock>())
      ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%", ingress_deparser);
  BlockInfo id("IngressDeparser", INGRESS, DEPARSER);
  toBlockInfo.emplace(ingress_deparser->to<IR::ControlBlock>()->container, id);
}

void ParsePsa::parseEgressPipeline(const IR::PackageBlock *block) {
  auto egress_parser = block->getParameterValue("ep");
  if (!egress_parser->is<IR::ParserBlock>())
      ::error(ErrorType::ERR_UNEXPECTED, "Expected ParserBlock for %1%", egress_parser);
  BlockInfo ep("EgressParser", EGRESS, PARSER);
  toBlockInfo.emplace(egress_parser->to<IR::ParserBlock>()->container, ep);

  auto egress = block->getParameterValue("eg");
  if (!egress->is<IR::ControlBlock>())
      ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%", egress);
  BlockInfo eg("Egress", EGRESS, PIPELINE);
  toBlockInfo.emplace(egress->to<IR::ControlBlock>()->container, eg);

  auto egress_deparser = block->getParameterValue("ed");
  if (!egress_deparser->is<IR::ControlBlock>())
      ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%", egress_deparser);
  BlockInfo ed("EgressDeparser", EGRESS, DEPARSER);
  toBlockInfo.emplace(egress_deparser->to<IR::ControlBlock>()->container, ed);
}

bool ParsePsa::preorder(const IR::PackageBlock *block) {
  auto decl = block->node->to<IR::Declaration_Instance>();
  // If no declaration found (anonymous instantiation) get the pipe name from
  // arch definition
  cstring pipe = decl->Name();
  BUG_CHECK(!pipe.isNullOrEmpty(), "Cannot determine pipe name for pipe block");

  auto ingress = block->getParameterValue("ingress");
  if (auto block = ingress->to<IR::PackageBlock>())
    parseIngressPipeline(block);
  auto egress = block->getParameterValue("egress");
  if (auto block = egress->to<IR::PackageBlock>())
    parseEgressPipeline(block);

  return false;
}

void CollectMetadataHeaderInfo::pushMetadata(const IR::Parameter *p) {
  for (auto m : used_metadata) {
    if (m->to<IR::Type_Name>()->path->name.name ==
        p->type->to<IR::Type_Name>()->path->name.name) {
      return;
    }
  }
  used_metadata.push_back(p->type);
}

bool CollectMetadataHeaderInfo::preorder(const IR::P4Program *) {
  for (auto kv : *toBlockInfo) {
    if (kv.second.pipe == "IngressParser") {
      auto parser = kv.first->to<IR::P4Parser>();
      auto local_metadata = parser->getApplyParameters()->getParameter(2);
      local_metadata_type =
          local_metadata->type->to<IR::Type_Name>()->path->name;
      auto header = parser->getApplyParameters()->getParameter(1);
      header_type = header->type->to<IR::Type_Name>()->path->name;
      auto params = parser->getApplyParameters();
      pushMetadata(params->getParameter(2));
      pushMetadata(params->getParameter(3));
      pushMetadata(params->getParameter(4));
      pushMetadata(params->getParameter(5));
    } else if (kv.second.pipe == "Ingress") {
      auto control = kv.first->to<IR::P4Control>();
      auto params = control->getApplyParameters();
      pushMetadata(params->getParameter(1));
      pushMetadata(params->getParameter(2));
      pushMetadata(params->getParameter(3));
    } else if (kv.second.pipe == "IngressParser") {
      auto deparser = kv.first->to<IR::P4Control>();
      auto params = deparser->getApplyParameters();
      pushMetadata(params->getParameter(1));
      pushMetadata(params->getParameter(2));
      pushMetadata(params->getParameter(3));
      pushMetadata(params->getParameter(5));
      pushMetadata(params->getParameter(6));
    } else if (kv.second.pipe == "EgressParser") {
      auto parser = kv.first->to<IR::P4Parser>();
      auto params = parser->getApplyParameters();
      pushMetadata(params->getParameter(2));
      pushMetadata(params->getParameter(3));
      pushMetadata(params->getParameter(4));
      pushMetadata(params->getParameter(5));
      pushMetadata(params->getParameter(6));
    } else if (kv.second.pipe == "Egress") {
      auto control = kv.first->to<IR::P4Control>();
      auto params = control->getApplyParameters();
      pushMetadata(params->getParameter(1));
      pushMetadata(params->getParameter(2));
      pushMetadata(params->getParameter(3));
    } else if (kv.second.pipe == "EgressDeparser") {
      auto deparser = kv.first->to<IR::P4Control>();
      auto params = deparser->getApplyParameters();
      pushMetadata(params->getParameter(1));
      pushMetadata(params->getParameter(2));
      pushMetadata(params->getParameter(4));
      pushMetadata(params->getParameter(5));
      pushMetadata(params->getParameter(6));
    }
  }
  return true;
}

bool CollectMetadataHeaderInfo::preorder(const IR::Type_Struct *s) {
  for (auto m : used_metadata) {
    if (m->to<IR::Type_Name>()->path->name.name == s->name.name) {
      for (auto field : s->fields) {
        fields.push_back(new IR::StructField(
            IR::ID(TypeStruct2Name(s->name.name) + "_" + field->name),
            field->type));
      }
      return true;
    }
  }
  return true;
}

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Member *m) {
  if (auto p = m->expr->to<IR::PathExpression>()) {
    auto declaration = refMap->getDeclaration(p->path);
    if (auto decl = declaration->to<IR::Parameter>()) {
      if (auto type = decl->type->to<IR::Type_Name>()) {
        if (type->path->name == "psa_ingress_parser_input_metadata_t" ||
            type->path->name == "psa_ingress_input_metadata_t" ||
            type->path->name == "psa_ingress_output_metadata_t" ||
            type->path->name == "psa_egress_parser_input_metadata_t" ||
            type->path->name == "psa_egress_input_metadata_t" ||
            type->path->name == "psa_egress_output_metadata_t" ||
            type->path->name == "psa_egress_deparser_input_metadata_t") {
          return new IR::Member(new IR::PathExpression(IR::ID("m")),
                                IR::ID(TypeStruct2Name(type->path->name.name) +
                                       "_" + m->member.name));
        } else if (type->path->name == info->header_type) {
          return new IR::Member(new IR::PathExpression(IR::ID("h")),
                                IR::ID(m->member.name));
        } else if (type->path->name == info->local_metadata_type) {
          return new IR::Member(new IR::PathExpression(IR::ID("m")),
                                IR::ID("local_metadata_" + m->member.name));
        }
      }
    }
  }
  return m;
}

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Parameter *p) {
  if (auto type = p->type->to<IR::Type_Name>()) {
    if (type->path->name == info->header_type) {
      return new IR::Parameter(IR::ID("h"), p->direction, p->type);
    } else if (type->path->name == info->local_metadata_type) {
      return new IR::Parameter(IR::ID("m"), p->direction, p->type);
    }
  }
  return p;
}

const IR::Node *InjectJumboStruct::preorder(IR::Type_Struct *s) {
  if (s->name == info->local_metadata_type) {
    auto *annotations =
        new IR::Annotations({new IR::Annotation(IR::ID("__metadata__"), {})});
    return new IR::Type_Struct(s->name, annotations, info->fields);
  } else if (s->name == info->header_type) {
    auto *annotations = new IR::Annotations(
        {new IR::Annotation(IR::ID("__packet_data__"), {})});
    return new IR::Type_Struct(s->name, annotations, s->fields);
  }
  return s;
}

const IR::Node *StatementUnroll::preorder(IR::AssignmentStatement *a) {
  auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
  auto right = a->right;
  auto control = findOrigCtxt<IR::P4Control>();
  auto parser = findOrigCtxt<IR::P4Parser>();

  if (right->is<IR::MethodCallExpression>()) {
    prune();
    return a;
  } else if (auto bin = right->to<IR::Operation_Binary>()) {
    ExpressionUnroll::sanity(bin->right);
    ExpressionUnroll::sanity(bin->left);
    auto left_unroller = new ExpressionUnroll(collector);
    auto right_unroller = new ExpressionUnroll(collector);
    bin->left->apply(*left_unroller);
    const IR::Expression *left_tmp = left_unroller->root;
    bin->right->apply(*right_unroller);
    const IR::Expression *right_tmp = right_unroller->root;
    if (!left_tmp)
      left_tmp = bin->left;
    if (!right_tmp)
      right_tmp = bin->right;
    for (auto s : left_unroller->stmt)
      code_block->push_back(s);
    for (auto d : left_unroller->decl)
      injector.collect(control, parser, d);
    for (auto s : right_unroller->stmt)
      code_block->push_back(s);
    for (auto d : right_unroller->decl)
      injector.collect(control, parser, d);
    prune();
    if (right->is<IR::Add>()) {
      a->right = new IR::Add(left_tmp, right_tmp);
    } else if (right->is<IR::Sub>()) {
      a->right = new IR::Sub(left_tmp, right_tmp);
    } else if (right->is<IR::Shl>()) {
      a->right = new IR::Shl(left_tmp, right_tmp);
    } else if (right->is<IR::Shr>()) {
      a->right = new IR::Shr(left_tmp, right_tmp);
    } else if (right->is<IR::Equ>()) {
      a->right = new IR::Equ(left_tmp, right_tmp);
    } else {
      std::cerr << right->node_type_name() << std::endl;
      BUG("not implemented.");
    }
    code_block->push_back(a);
    return new IR::BlockStatement(*code_block);
  } else if (isSimpleExpression(right)) {
    prune();
  } else if (auto un = right->to<IR::Operation_Unary>()) {
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    ExpressionUnroll::sanity(un->expr);
    auto unroller = new ExpressionUnroll(collector);
    un->expr->apply(*unroller);
    prune();
    const IR::Expression *un_tmp = unroller->root;
    for (auto s : unroller->stmt)
      code_block->push_back(s);
    for (auto d : unroller->decl)
      injector.collect(control, parser, d);
    if (!un_tmp)
      un_tmp = un->expr;
    if (right->to<IR::Neg>()) {
      a->right = new IR::Neg(un_tmp);
    } else if (right->to<IR::Cmpl>()) {
      a->right = new IR::Cmpl(un_tmp);
    } else if (right->to<IR::LNot>()) {
      a->right = new IR::LNot(un_tmp);
    } else if (auto c = right->to<IR::Cast>()) {
      a->right = new IR::Cast(c->destType, un_tmp);
    } else {
      std::cerr << right->node_type_name() << std::endl;
      BUG("Not implemented.");
    }
    code_block->push_back(a);
    return new IR::BlockStatement(*code_block);
  } else {
    std::cerr << right->node_type_name() << std::endl;
    BUG("not implemented");
  }
  return a;
}

const IR::Node *StatementUnroll::postorder(IR::P4Control *a) {
  auto control = getOriginal();
  return injector.inject_control(control, a);
}
const IR::Node *StatementUnroll::postorder(IR::P4Parser *a) {
  auto parser = getOriginal();
  return injector.inject_parser(parser, a);
}

bool ExpressionUnroll::preorder(const IR::Operation_Unary *u) {
  sanity(u->expr);
  visit(u->expr);
  const IR::Expression *un_expr;
  if (root) {
    if (u->to<IR::Neg>()) {
      un_expr = new IR::Neg(root);
    } else if (u->to<IR::Cmpl>()) {
      un_expr = new IR::Cmpl(root);
    } else if (u->to<IR::LNot>()) {
      un_expr = new IR::LNot(root);
    } else {
      std::cout << u->node_type_name() << std::endl;
      BUG("Not Implemented");
    }
  } else {
    un_expr = u->expr;
  }
  root = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
  stmt.push_back(new IR::AssignmentStatement(root, un_expr));
  decl.push_back(new IR::Declaration_Variable(root->path->name, u->type));
  return false;
}

bool ExpressionUnroll::preorder(const IR::Operation_Binary *bin) {
  sanity(bin->left);
  sanity(bin->right);
  visit(bin->left);
  const IR::Expression *left_root = root;
  visit(bin->right);
  const IR::Expression *right_root = root;
  if (!left_root)
    left_root = bin->left;
  if (!right_root)
    right_root = bin->right;

  root = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
  const IR::Expression *bin_expr;
  decl.push_back(new IR::Declaration_Variable(root->path->name, bin->type));
  if (bin->is<IR::Add>()) {
    bin_expr = new IR::Add(left_root, right_root);
  } else if (bin->is<IR::Sub>()) {
    bin_expr = new IR::Sub(left_root, right_root);
  } else if (bin->is<IR::Shl>()) {
    bin_expr = new IR::Shl(left_root, right_root);
  } else if (bin->is<IR::Shr>()) {
    bin_expr = new IR::Shr(left_root, right_root);
  } else if (bin->is<IR::Equ>()) {
    bin_expr = new IR::Equ(left_root, right_root);
  } else if (bin->is<IR::LAnd>()) {
    bin_expr = new IR::LAnd(left_root, right_root);
  } else if (bin->is<IR::Leq>()) {
    bin_expr = new IR::Leq(left_root, right_root);
  } else if (bin->is<IR::Lss>()) {
    bin_expr = new IR::Lss(left_root, right_root);
  } else if (bin->is<IR::Geq>()) {
    bin_expr = new IR::Geq(left_root, right_root);
  } else if (bin->is<IR::Grt>()) {
    bin_expr = new IR::Grt(left_root, right_root);
  } else if (bin->is<IR::Neq>()) {
    bin_expr = new IR::Neq(left_root, right_root);
  } else {
    std::cerr << bin->node_type_name() << std::endl;
    BUG("not implemented");
  }
  stmt.push_back(new IR::AssignmentStatement(root, bin_expr));
  return false;
}

bool ExpressionUnroll::preorder(const IR::MethodCallExpression *m) {
  auto args = new IR::Vector<IR::Argument>;
  for (auto arg : *m->arguments) {
    sanity(arg->expression);
    visit(arg->expression);
    if (!root)
      args->push_back(arg);
    else
      args->push_back(new IR::Argument(root));
  }
  root = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
  decl.push_back(new IR::Declaration_Variable(root->path->name, m->type));
  auto new_m = new IR::MethodCallExpression(m->method, args);
  stmt.push_back(new IR::AssignmentStatement(root, new_m));
  return false;
}

bool ExpressionUnroll::preorder(const IR::Member *) {
  root = nullptr;
  return false;
}

bool ExpressionUnroll::preorder(const IR::PathExpression *) {
  root = nullptr;
  return false;
}

bool ExpressionUnroll::preorder(const IR::Constant *) {
  root = nullptr;
  return false;
}
bool ExpressionUnroll::preorder(const IR::BoolLiteral *) {
  root = nullptr;
  return false;
}

const IR::Node *IfStatementUnroll::postorder(IR::IfStatement *i) {
  auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
  ExpressionUnroll::sanity(i->condition);
  auto unroller = new LogicalExpressionUnroll(collector);
  i->condition->apply(*unroller);
  for (auto i : unroller->stmt)
    code_block->push_back(i);

  auto control = findOrigCtxt<IR::P4Control>();
  auto parser = findOrigCtxt<IR::P4Parser>();

  for (auto d : unroller->decl)
    injector.collect(control, parser, d);
  if (unroller->root) {
    i->condition = unroller->root;
  }
  code_block->push_back(i);
  return new IR::BlockStatement(*code_block);
}

const IR::Node *IfStatementUnroll::postorder(IR::P4Control *a) {
  auto control = getOriginal();
  return injector.inject_control(control, a);
}
const IR::Node *IfStatementUnroll::postorder(IR::P4Parser *a) {
  auto parser = getOriginal();
  return injector.inject_parser(parser, a);
}

bool LogicalExpressionUnroll::preorder(const IR::Operation_Unary *u) {
  sanity(u->expr);
  if (!u->is<IR::Member>()) {
    BUG("%1% Not Implemented", u);
  } else {
    root = u->clone();
    return false;
  }

  visit(u->expr);
  const IR::Expression *un_expr;
  if (root) {
    if (u->to<IR::Neg>()) {
      un_expr = new IR::Neg(root);
    } else if (u->to<IR::Cmpl>()) {
      un_expr = new IR::Cmpl(root);
    } else if (u->to<IR::LNot>()) {
      un_expr = new IR::LNot(root);
    } else {
      BUG("%1% Not Implemented", u);
    }
  } else {
    un_expr = u->expr;
  }
  auto tmp = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
  root = tmp;
  stmt.push_back(new IR::AssignmentStatement(root, un_expr));
  decl.push_back(new IR::Declaration_Variable(tmp->path->name, u->type));
  return false;
}

bool LogicalExpressionUnroll::preorder(const IR::Operation_Binary *bin) {
  sanity(bin->left);
  sanity(bin->right);
  visit(bin->left);
  const IR::Expression *left_root = root;
  visit(bin->right);
  const IR::Expression *right_root = root;
  if (!left_root)
    left_root = bin->left;
  if (!right_root)
    right_root = bin->right;

  IR::Expression *bin_expr;
  if (is_logical(bin)) {
    if (bin->is<IR::LAnd>()) {
      bin_expr = new IR::LAnd(left_root, right_root);
    } else if (bin->is<IR::LOr>()) {
      bin_expr = new IR::LOr(left_root, right_root);
    } else if (bin->is<IR::Equ>()) {
      bin_expr = new IR::Equ(left_root, right_root);
    } else if (bin->is<IR::Neq>()) {
      bin_expr = new IR::Neq(left_root, right_root);
    } else if (bin->is<IR::Grt>()) {
      bin_expr = new IR::Grt(left_root, right_root);
    } else if (bin->is<IR::Lss>()) {
      bin_expr = new IR::Lss(left_root, right_root);
    } else {
        BUG("%1%: not implemented", bin);
    }
    root = bin_expr;
  } else {
    auto tmp = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
    root = tmp;
    decl.push_back(new IR::Declaration_Variable(tmp->path->name, bin->type));
    if (bin->is<IR::Add>()) {
      bin_expr = new IR::Add(left_root, right_root);
    } else if (bin->is<IR::Sub>()) {
      bin_expr = new IR::Sub(left_root, right_root);
    } else if (bin->is<IR::Shl>()) {
      bin_expr = new IR::Shl(left_root, right_root);
    } else if (bin->is<IR::Shr>()) {
      bin_expr = new IR::Shr(left_root, right_root);
    } else {
      std::cerr << bin->node_type_name() << std::endl;
      BUG("%1%: not implemented", bin);
    }
    stmt.push_back(new IR::AssignmentStatement(root, bin_expr));
  }
  return false;
}

bool LogicalExpressionUnroll::preorder(const IR::MethodCallExpression *m) {
  auto args = new IR::Vector<IR::Argument>;
  for (auto arg : *m->arguments) {
    sanity(arg->expression);
    visit(arg->expression);
    if (!root)
      args->push_back(arg);
    else
      args->push_back(new IR::Argument(root));
  }
  if (m->type->to<IR::Type_Boolean>()) {
    root = m->clone();
    return false;
  }
  auto tmp = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
  root = tmp;
  decl.push_back(new IR::Declaration_Variable(tmp->path->name, m->type));
  auto new_m = new IR::MethodCallExpression(m->method, args);
  stmt.push_back(new IR::AssignmentStatement(root, new_m));
  return false;
}

bool LogicalExpressionUnroll::preorder(const IR::Member *) {
  root = nullptr;
  return false;
}

bool LogicalExpressionUnroll::preorder(const IR::PathExpression *) {
  root = nullptr;
  return false;
}

bool LogicalExpressionUnroll::preorder(const IR::Constant *) {
  root = nullptr;
  return false;
}
bool LogicalExpressionUnroll::preorder(const IR::BoolLiteral *) {
  root = nullptr;
  return false;
}

const IR::Node *
ConvertBinaryOperationTo2Params::postorder(IR::AssignmentStatement *a) {
  auto right = a->right;
  auto left = a->left;
  if (auto r = right->to<IR::Operation_Binary>()) {
    if (!isSimpleExpression(r->right) || !isSimpleExpression(r->left))
      BUG("%1%: Statement Unroll pass failed", a);
    if (left->equiv(*r->left)) {
      return a;
    } else if (left->equiv(*r->right)) {
      if (right->is<IR::Add>()) {
        a->right = new IR::Add(r->right, r->left);
      } else if (right->is<IR::Sub>()) {
        a->right = new IR::Sub(r->right, r->left);
      } else if (right->is<IR::Shl>()) {
        a->right = new IR::Shl(r->right, r->left);
      } else if (right->is<IR::Shr>()) {
        a->right = new IR::Shr(r->right, r->left);
      } else if (right->is<IR::Equ>()) {
        a->right = new IR::Equ(r->right, r->left);
      } else {
        std::cerr << right->node_type_name() << std::endl;
        BUG("%1%: not implemented.", a);
      }
      return a;
    } else {
      IR::IndexedVector<IR::StatOrDecl> code_block;
      const IR::Expression *src1;
      const IR::Expression *src2;
      if (isNonConstantSimpleExpression(r->left)) {
        src1 = r->left;
        src2 = r->right;
      } else if (isNonConstantSimpleExpression(r->right)) {
        src1 = r->right;
        src2 = r->left;
      } else {
        std::cerr << r->right->node_type_name() << std::endl;
        std::cerr << r->left->node_type_name() << std::endl;
        BUG("Confronting a expression that can be simplified to become a "
            "constant.");
      }
      // auto tmp = new IR::PathExpression(IR::ID(collector->get_next_tmp()));
      // injector.collect(control, parser, new
      // IR::Declaration_Variable(tmp->path->name, src1->type));
      // injector.collect(control, parser, new
      // IR::Declaration_Variable(tmp->path->name, r->type));
      code_block.push_back(new IR::AssignmentStatement(left, src1));
      IR::Operation_Binary *expr;
      if (right->is<IR::Add>()) {
        expr = new IR::Add(left, src2);
      } else if (right->is<IR::Sub>()) {
        expr = new IR::Sub(left, src2);
      } else if (right->is<IR::Shl>()) {
        expr = new IR::Shl(left, src2);
      } else if (right->is<IR::Shr>()) {
        expr = new IR::Shr(left, src2);
      } else if (right->is<IR::Equ>()) {
        expr = new IR::Equ(left, src2);
      } else if (right->is<IR::LAnd>()) {
        expr = new IR::LAnd(left, src2);
      } else if (right->is<IR::Leq>()) {
        expr = new IR::Leq(left, src2);
      } else {
        BUG("%1%: not implemented.", a);
      }
      code_block.push_back(new IR::AssignmentStatement(left, expr));
      // code_block.push_back(new IR::AssignmentStatement(left, tmp));
      return new IR::BlockStatement(code_block);
    }
  }
  return a;
}

const IR::Node *ConvertBinaryOperationTo2Params::postorder(IR::P4Control *a) {
  auto control = getOriginal();
  return injector.inject_control(control, a);
}

const IR::Node *ConvertBinaryOperationTo2Params::postorder(IR::P4Parser *a) {
  auto parser = getOriginal();
  return injector.inject_parser(parser, a);
}

const IR::Node *CollectLocalVariableToMetadata::preorder(IR::P4Program *p) {
  for (auto kv : *toBlockInfo) {
    if (kv.second.pipe == "IngressParser") {
      auto parser = kv.first->to<IR::P4Parser>();
      locals_map.emplace(kv.second.pipe, parser->parserLocals);
    } else if (kv.second.pipe == "Ingress") {
      auto control = kv.first->to<IR::P4Control>();
      locals_map.emplace(kv.second.pipe, control->controlLocals);
    } else if (kv.second.pipe == "IngressParser") {
      auto control = kv.first->to<IR::P4Control>();
      locals_map.emplace(kv.second.pipe, control->controlLocals);
    } else if (kv.second.pipe == "EgressParser") {
      auto parser = kv.first->to<IR::P4Parser>();
      locals_map.emplace(kv.second.pipe, parser->parserLocals);
    } else if (kv.second.pipe == "Egress") {
      auto control = kv.first->to<IR::P4Control>();
      locals_map.emplace(kv.second.pipe, control->controlLocals);
    } else if (kv.second.pipe == "EgressDeparser") {
      auto control = kv.first->to<IR::P4Control>();
      locals_map.emplace(kv.second.pipe, control->controlLocals);
    }
  }
  return p;
}

const IR::Node *CollectLocalVariableToMetadata::postorder(IR::Type_Struct *s) {
  if (s->name.name == info->local_metadata_type) {
    for (auto kv : locals_map) {
      for (auto d : kv.second) {
        if (auto dv = d->to<IR::Declaration_Variable>()) {
          s->fields.push_back(new IR::StructField(
              IR::ID(kv.first + "_" + dv->name.name), dv->type));
        } else if (!d->is<IR::P4Action>() && !d->is<IR::P4Table>() &&
                   !d->is<IR::Declaration_Instance>()) {
          BUG("%1%: unhandled declaration type", s);
        }
      }
    }
  }
  return s;
}

const IR::Node *
CollectLocalVariableToMetadata::postorder(IR::PathExpression *p) {
  if (auto decl =
          refMap->getDeclaration(p->path)->to<IR::Declaration_Variable>()) {
    for (auto kv : locals_map) {
      for (auto d : kv.second) {
        if (d->equiv(*decl)) {
          return new IR::Member(new IR::PathExpression(IR::ID("m")),
                                IR::ID(kv.first + "_" + decl->name.name));
        }
      }
    }
    BUG("%1%: variable is not included in a control or parser block", p);
  }
  return p;
}

const IR::Node *CollectLocalVariableToMetadata::postorder(IR::P4Control *c) {
  IR::IndexedVector<IR::Declaration> decls;
  for (auto d : c->controlLocals) {
    if (d->is<IR::Declaration_Instance>() || d->is<IR::P4Action>() ||
        d->is<IR::P4Table>()) {
      decls.push_back(d);
    } else if (!d->is<IR::Declaration_Variable>()) {
      std::cerr << d->node_type_name() << std::endl;
      BUG("%1%: Unhandled declaration type in control", c);
    }
  }
  c->controlLocals = decls;
  return c;
}
const IR::Node *CollectLocalVariableToMetadata::postorder(IR::P4Parser *p) {
  IR::IndexedVector<IR::Declaration> decls;
  for (auto d : p->parserLocals) {
    if (d->is<IR::Declaration_Instance>() || d->is<IR::P4Action>() ||
        d->is<IR::P4Table>()) {
      decls.push_back(d);
    } else if (!d->is<IR::Declaration_Variable>()) {
      std::cerr << d->node_type_name() << std::endl;
      BUG("%1%: Unhandled declaration type in parser", p);
    }
  }
  p->parserLocals = decls;
  return p;
}

const IR::Node *PrependPDotToActionArgs::postorder(IR::P4Action *a) {
  if (a->parameters->size() > 0) {
    auto l = new IR::IndexedVector<IR::Parameter>;
    for (auto p : a->parameters->parameters) {
      l->push_back(p);
    }
    args_struct_map.emplace(a->name.name + "_arg_t", l);
    auto new_l = new IR::IndexedVector<IR::Parameter>;
    new_l->push_back(
        new IR::Parameter(IR::ID("t"), IR::Direction::None,
                          new IR::Type_Name(IR::ID(a->name.name + "_arg_t"))));
    a->parameters = new IR::ParameterList(*new_l);
  }
  return a;
}

const IR::Node *PrependPDotToActionArgs::postorder(IR::P4Program *program) {
  auto new_objs = new IR::Vector<IR::Node>;
  for (auto obj : program->objects) {
    if (auto control = obj->to<IR::P4Control>()) {
      for (auto kv : *toBlockInfo) {
        if (kv.second.pipe == "Ingress") {
          if (kv.first->to<IR::P4Control>()->name == control->name) {
            for (auto kv : args_struct_map) {
              auto fields = new IR::IndexedVector<IR::StructField>;
              for (auto field : *kv.second) {
                fields->push_back(
                    new IR::StructField(field->name, field->type));
              }
              new_objs->push_back(
                  new IR::Type_Struct(IR::ID(kv.first), *fields));
            }
          }
        }
      }
    }
    new_objs->push_back(obj);
  }
  program->objects = *new_objs;
  return program;
}

const IR::Node *PrependPDotToActionArgs::preorder(IR::PathExpression *path) {
  auto declaration = refMap->getDeclaration(path->path);
  if (auto action = findContext<IR::P4Action>()) {
    if (!declaration) {
      return path;
    }
    if (auto p = declaration->to<IR::Parameter>()) {
      for (auto para : action->parameters->parameters) {
        if (para->equiv(*p)) {
          prune();
          return new IR::Member(new IR::PathExpression(IR::ID("t")),
                                path->path->name);
        }
      }
    }
  }
  return path;
}

}  // namespace DPDK
