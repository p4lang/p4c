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
#include "frontends/p4/coreLibrary.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/tableApply.h"

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

// This function is a sanity to check whether the component of a Expression
// falls into following classes, if not, it means we haven't implemented a
// handle for that class.
void expressionUnrollSanityCheck(const IR::Expression *e) {
    if (!e->is<IR::Operation_Unary>() &&
            !e->is<IR::MethodCallExpression>() && !e->is<IR::Member>() &&
            !e->is<IR::PathExpression>() && !e->is<IR::Operation_Binary>() &&
            !e->is<IR::Constant>() && !e->is<IR::BoolLiteral>()) {
        std::cerr << e->node_type_name() << std::endl;
        BUG("Untraversed node");
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
        auto tc = rewriteControlType(c->type, bi.pipe);
        auto cont = new IR::P4Control(c->name, tc, c->constructorParams,
                                      c->controlLocals, c->body);
        LOG3(cont);
        return cont;
    }
    return c;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Parser *p) {
    auto orig = getOriginal();
    if (block_info->count(orig) != 0) {
        auto bi = block_info->at(orig);
        auto tp = rewriteParserType(p->type, bi.pipe);
        auto prsr = new IR::P4Parser(p->name, tp, p->constructorParams,
                                     p->parserLocals, p->states);
        LOG3(prsr);
        return prsr;
    }
    return p;
}

void ParsePsa::parseIngressPipeline(const IR::PackageBlock *block) {
    auto ingress_parser = block->getParameterValue("ip");
    if (!ingress_parser->is<IR::ParserBlock>())
        ::error(ErrorType::ERR_UNEXPECTED, "Expected ParserBlock for %1%",
                ingress_parser);
    BlockInfo ip("IngressParser", INGRESS, PARSER);
    toBlockInfo.emplace(ingress_parser->to<IR::ParserBlock>()->container, ip);

    auto ingress = block->getParameterValue("ig");
    if (!ingress->is<IR::ControlBlock>())
        ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%",
                ingress);
    BlockInfo ig("Ingress", INGRESS, PIPELINE);
    toBlockInfo.emplace(ingress->to<IR::ControlBlock>()->container, ig);

    auto ingress_deparser = block->getParameterValue("id");
    if (!ingress_deparser->is<IR::ControlBlock>())
        ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%",
                ingress_deparser);
    BlockInfo id("IngressDeparser", INGRESS, DEPARSER);
    toBlockInfo.emplace(ingress_deparser->to<IR::ControlBlock>()->container,
                        id);
}

void ParsePsa::parseEgressPipeline(const IR::PackageBlock *block) {
    auto egress_parser = block->getParameterValue("ep");
    if (!egress_parser->is<IR::ParserBlock>())
        ::error(ErrorType::ERR_UNEXPECTED, "Expected ParserBlock for %1%",
                egress_parser);
    BlockInfo ep("EgressParser", EGRESS, PARSER);
    toBlockInfo.emplace(egress_parser->to<IR::ParserBlock>()->container, ep);

    auto egress = block->getParameterValue("eg");
    if (!egress->is<IR::ControlBlock>())
        ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%",
                egress);
    BlockInfo eg("Egress", EGRESS, PIPELINE);
    toBlockInfo.emplace(egress->to<IR::ControlBlock>()->container, eg);

    auto egress_deparser = block->getParameterValue("ed");
    if (!egress_deparser->is<IR::ControlBlock>())
        ::error(ErrorType::ERR_UNEXPECTED, "Expected ControlBlock for %1%",
                egress_deparser);
    BlockInfo ed("EgressDeparser", EGRESS, DEPARSER);
    toBlockInfo.emplace(egress_deparser->to<IR::ControlBlock>()->container, ed);
}

bool ParsePsa::preorder(const IR::PackageBlock *block) {
    auto decl = block->node->to<IR::Declaration_Instance>();
    // If no declaration found (anonymous instantiation) get the pipe name from
    // arch definition
    cstring pipe = decl->Name();
    BUG_CHECK(!pipe.isNullOrEmpty(),
              "Cannot determine pipe name for pipe block");

    auto ingress = block->getParameterValue("ingress");
    if (auto block = ingress->to<IR::PackageBlock>())
        parseIngressPipeline(block);
    auto egress = block->getParameterValue("egress");
    if (auto block = egress->to<IR::PackageBlock>())
        parseEgressPipeline(block);

    return false;
}

void CollectMetadataHeaderInfo::pushMetadata(const IR::ParameterList* params,
        std::list<int> indices) {
    for (auto idx : indices) {
        pushMetadata(params->getParameter(idx));
    }
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
            pushMetadata(params, { 2, 3, 4, 5 });
        } else if (kv.second.pipe == "Ingress") {
            auto control = kv.first->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        } else if (kv.second.pipe == "IngressParser") {
            auto deparser = kv.first->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, { 1, 2, 3, 5, 6 });
        } else if (kv.second.pipe == "EgressParser") {
            auto parser = kv.first->to<IR::P4Parser>();
            auto params = parser->getApplyParameters();
            pushMetadata(params, { 2, 3, 4, 5, 6 });
        } else if (kv.second.pipe == "Egress") {
            auto control = kv.first->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        } else if (kv.second.pipe == "EgressDeparser") {
            auto deparser = kv.first->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, { 1, 2, 4, 5, 6 });
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

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::PathExpression *pe) {
    auto declaration = refMap->getDeclaration(pe->path);
    if (auto decl = declaration->to<IR::Parameter>()) {
        if (auto type = decl->type->to<IR::Type_Name>()) {
            if (type->path->name == info->header_type){
                    return new IR::PathExpression(IR::ID("h"));
                } else if (type->path->name == info->local_metadata_type){
                    return new IR::PathExpression(IR::ID("m"));
                }
            }
        }
    return pe;
}

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Member *m) {
    /* PathExpressions are handled in a separate preorder function
       Hence do not process them here */
    if (!m->expr->is<IR::Member>() &&
        !m->expr->is<IR::ArrayIndex>())
        prune();

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
                    type->path->name ==
                        "psa_egress_deparser_input_metadata_t") {
                    return new IR::Member(
                        new IR::PathExpression(IR::ID("m")),
                        IR::ID(TypeStruct2Name(type->path->name.name) + "_" +
                               m->member.name));
                } else if (type->path->name == info->header_type) {
                    return new IR::Member(new IR::PathExpression(IR::ID("h")),
                                          IR::ID(m->member.name));
                } else if (type->path->name == info->local_metadata_type) {
                    return new IR::Member(
                        new IR::PathExpression(IR::ID("m")),
                        IR::ID("local_metadata_" + m->member.name));
                }
            }
        }
    }
    return m;
}

/* This function replaces the header and metadata parameter names in control
 * blocks with "h" and "m" respectively.
 * It assumes that ConvertToDpdkArch pass has converted the control blocks to
 * the following form for DPDK Architecture:
 *         control ingressDeparser(packet_out buffer, header hdr, metadata md);
 *         control egressDeparser(packet_out buffer, header hdr, metadata md);
 *         control ingress(header hdr, metadata md);
 *         control egress(header hdr, metadata md);
*/
const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Type_Control *c) {
    auto applyParams = new IR::ParameterList();
    auto paramSize = c->applyParams->size();

    if (!(paramSize == 2 || paramSize == 3))
        ::error(ErrorType::ERR_MODEL,
                ("Unexpected number of arguments for %1%. Are you using an up-to-date 'psa.p4'?"),
                 c->name);

    int header_index = 0;

    /* For IngressDeparser and EgressDeparser, Header and metadata parameters are
       at index 1 and 2 respectively. */
    if (paramSize == 3) {
        header_index = 1;
        applyParams->push_back(c->getApplyParameters()->getParameter(0));
    }

    auto header = c->applyParams->parameters.at(header_index);
    auto local_metadata = c->applyParams->parameters.at(header_index + 1);
    header = new IR::Parameter(IR::ID("h"), header->direction, header->type);
    local_metadata = new IR::Parameter(IR::ID("m"), local_metadata->direction,
                                       local_metadata->type);
    applyParams->push_back(header);
    applyParams->push_back(local_metadata);

    return new IR::Type_Control(c->name, c->annotations, c->typeParameters,
                                applyParams);
}

/* This function replaces the header and metadata parameter names in parser
 * blocks with "h" and "m" respectively.
 * It assumes that ConvertToDpdkArch pass has converted Ingress/Egress
 * parser blocks to the following form for DPDK Architecture:
 *         parser ingressParser(packet_in buffer, header hdr, metadata md);
 *         parser egressParser(packet_in buffer, header hdr, metadata md);
*/
const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Type_Parser *p) {
    auto applyParams = new IR::ParameterList();
    auto paramSize = p->applyParams->size();

    if (paramSize != 3)
        ::error(ErrorType::ERR_MODEL,
                ("Unexpected number of arguments for %1%. Are you using an up-to-date 'psa.p4'?"),
                 p->name);

    auto header = p->applyParams->parameters.at(1);
    auto local_metadata = p->applyParams->parameters.at(2);
    header = new IR::Parameter(IR::ID("h"), header->direction, header->type);
    local_metadata = new IR::Parameter(IR::ID("m"), local_metadata->direction,
                                       local_metadata->type);
    applyParams->push_back(p->getApplyParameters()->getParameter(0));
    applyParams->push_back(header);
    applyParams->push_back(local_metadata);

    return new IR::Type_Parser(p->name, p->annotations, p->typeParameters,
                               applyParams);
}

const IR::Node *InjectJumboStruct::preorder(IR::Type_Struct *s) {
    if (s->name == info->local_metadata_type) {
        auto *annotations = new IR::Annotations(
            {new IR::Annotation(IR::ID("__metadata__"), {})});
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
        expressionUnrollSanityCheck(bin->right);
        expressionUnrollSanityCheck(bin->left);
        auto left_unroller = new ExpressionUnroll(refMap, collector);
        auto right_unroller = new ExpressionUnroll(refMap, collector);
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
        IR::Operation_Binary *bin_expr;
        bin_expr = bin->clone();
        bin_expr->left = left_tmp;
        bin_expr->right = right_tmp;
        a->right = bin_expr;
        code_block->push_back(a);
        return new IR::BlockStatement(*code_block);
    } else if (isSimpleExpression(right)) {
        prune();
    } else if (auto un = right->to<IR::Operation_Unary>()) {
        auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
        expressionUnrollSanityCheck(un->expr);
        auto unroller = new ExpressionUnroll(refMap, collector);
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
        BUG("%1% not implemented", a);
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
    expressionUnrollSanityCheck(u->expr);
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
    root = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
    stmt.push_back(new IR::AssignmentStatement(root, un_expr));
    decl.push_back(new IR::Declaration_Variable(root->path->name, u->type));
    return false;
}

bool ExpressionUnroll::preorder(const IR::Operation_Binary *bin) {
    expressionUnrollSanityCheck(bin->left);
    expressionUnrollSanityCheck(bin->right);
    visit(bin->left);
    const IR::Expression *left_root = root;
    visit(bin->right);
    const IR::Expression *right_root = root;
    if (!left_root)
        left_root = bin->left;
    if (!right_root)
        right_root = bin->right;

    root = new IR::PathExpression(IR::ID(refMap->newName("tmp")));

    decl.push_back(new IR::Declaration_Variable(root->path->name, bin->type));

    IR::Operation_Binary *bin_expr;
    bin_expr = bin->clone();
    bin_expr->left = left_root;
    bin_expr->right = right_root;

    stmt.push_back(new IR::AssignmentStatement(root, bin_expr));
    return false;
}

bool ExpressionUnroll::preorder(const IR::MethodCallExpression *m) {
    auto args = new IR::Vector<IR::Argument>;
    for (auto arg : *m->arguments) {
        expressionUnrollSanityCheck(arg->expression);
        visit(arg->expression);
        if (!root)
            args->push_back(arg);
        else
            args->push_back(new IR::Argument(root));
    }
    root = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
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
    expressionUnrollSanityCheck(i->condition);
    auto unroller = new LogicalExpressionUnroll(refMap, collector);
    i->condition->apply(*unroller);
    for (auto i : unroller->stmt)
        code_block->push_back(i);

    auto control = findOrigCtxt<IR::P4Control>();

    for (auto d : unroller->decl)
        injector.collect(control, nullptr, d);
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

// TODO(GordonWuCn): simplify with a postorder visitor if it is a statement,
// return the expression, else introduce a temporary for the current expression
// and return the temporary in a pathexpression.
bool LogicalExpressionUnroll::preorder(const IR::Operation_Unary *u) {
    expressionUnrollSanityCheck(u->expr);

    // If the expression is a methodcall expression, do not insert a temporary
    // variable to represent the value of the methodcall. Instead the
    // methodcall is converted to a dpdk branch instruction in a later pass.
    if (u->expr->is<IR::MethodCallExpression>())
        return false;

    // if the expression is apply().hit or apply().miss, do not insert a temporary
    // variable.
    if (auto member = u->expr->to<IR::Member>()) {
        if (member->expr->is<IR::MethodCallExpression>() &&
            (member->member == IR::Type_Table::hit ||
             member->member == IR::Type_Table::miss)) {
            return false;
        }
    }

    root = u->clone();
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

    auto tmp = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
    root = tmp;
    stmt.push_back(new IR::AssignmentStatement(root, un_expr));
    decl.push_back(new IR::Declaration_Variable(tmp->path->name, u->type));
    return false;
}

bool LogicalExpressionUnroll::preorder(const IR::Operation_Binary *bin) {
    expressionUnrollSanityCheck(bin->left);
    expressionUnrollSanityCheck(bin->right);
    visit(bin->left);
    const IR::Expression *left_root = root;
    visit(bin->right);
    const IR::Expression *right_root = root;
    if (!left_root)
        left_root = bin->left;
    if (!right_root)
        right_root = bin->right;

    if (is_logical(bin)) {
        IR::Operation_Binary *bin_expr;
        bin_expr = bin->clone();
        bin_expr->left = left_root;
        bin_expr->right = right_root;
        root = bin_expr;
    } else {
        auto tmp = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
        root = tmp;
        decl.push_back(
            new IR::Declaration_Variable(tmp->path->name, bin->type));
        IR::Operation_Binary *bin_expr;
        bin_expr = bin->clone();
        bin_expr->left = left_root;
        bin_expr->right = right_root;
        stmt.push_back(new IR::AssignmentStatement(root, bin_expr));
    }
    return false;
}

bool LogicalExpressionUnroll::preorder(const IR::MethodCallExpression *m) {
    auto args = new IR::Vector<IR::Argument>;
    for (auto arg : *m->arguments) {
        expressionUnrollSanityCheck(arg->expression);
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
    auto tmp = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
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
    // This pass does not apply to 'bool foo = (a == b)' or 'bool foo = (a > b)' etc.
    if (right->to<IR::Operation_Relation>())
        return a;
    if (auto r = right->to<IR::Operation_Binary>()) {
        if (!isSimpleExpression(r->right) || !isSimpleExpression(r->left))
            BUG("%1%: Statement Unroll pass failed", a);
        if (left->equiv(*r->left)) {
            return a;
        } else if (left->equiv(*r->right)) {
            IR::Operation_Binary *bin_expr;
            bin_expr = r->clone();
            bin_expr->left = r->right;
            bin_expr->right = r->left;
            a->right = bin_expr;
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
                BUG("Confronting a expression that can be simplified to become "
                    "a constant.");
            }
            code_block.push_back(new IR::AssignmentStatement(left, src1));
            IR::Operation_Binary *expr;
            expr = r->clone();
            expr->left = left;
            expr->right = src2;
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
        } else if (kv.second.pipe == "IngressDeparser") {
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
                    BUG("%1%: Unhandled declaration type", s);
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
                    return new IR::Member(
                        new IR::PathExpression(IR::ID("m")),
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
            BUG("%1%: Unhandled declaration type in control", d);
        }
    }
    c->controlLocals = decls;
    return c;
}

const IR::Node *CollectLocalVariableToMetadata::postorder(IR::P4Parser *p) {
    IR::IndexedVector<IR::Declaration> decls;
    for (auto d : p->parserLocals) {
        if (d->is<IR::Declaration_Instance>()) {
            decls.push_back(d);
        } else if (!d->is<IR::Declaration_Variable>()) {
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
        args_struct_map.emplace(a->name.toString() + "_arg_t", l);
        auto new_l = new IR::IndexedVector<IR::Parameter>;
        new_l->push_back(new IR::Parameter(
            IR::ID("t"), IR::Direction::None,
            new IR::Type_Name(IR::ID(a->name.toString() + "_arg_t"))));
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
                            auto fields =
                                new IR::IndexedVector<IR::StructField>;
                            for (auto field : *kv.second) {
                                fields->push_back(new IR::StructField(
                                    field->name.toString(), field->type));
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
                                          path->path->name.toString());
                }
            }
        }
    }
    return path;
}

const IR::Node* PrependPDotToActionArgs::preorder(IR::MethodCallExpression* mce) {
    auto property = findContext<IR::Property>();
    if (!property)
        return mce;
    if (property->name != "default_action" &&
        property->name != "entries")
        return mce;
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ActionCall>())
        return mce;
    // We assume all action call has been converted to take struct as input,
    // therefore, the arguments must be passed in as a list expression
    if (mce->arguments->size() == 0)
        return mce;
    IR::Vector<IR::Expression> components;
    for (auto arg : *mce->arguments) {
        components.push_back(arg->expression);
    }
    auto arguments = new IR::Vector<IR::Argument>;
    arguments->push_back(new IR::Argument(new IR::ListExpression(components)));
    return new IR::MethodCallExpression(
            mce->method,
            arguments);
}

/* This function transforms the table so that all match keys come from the same struct.
   Mirror copies of match fields are created in metadata struct and table is updated to
   use the metadata fields.

   control ingress(inout headers h, inout metadata m) {
   {
       ...
       table tbl {
           key = {
               hdr.ethernet.srcAddr : lpm;
               hdr.ipv4.totalLen : exact;
           }
           ...
       }
       apply {
           tbl.apply();
       }
   }

   gets translated to
   control ingress(inout headers h, inout metadata m) {
       bit<48> tbl_ethernet_srcAddr;  // These declarations are later copied to metadata struct
       bit<16> tbl_ipv4_totalLen;     // in CollectLocalVariableToMetadata pass.
       ...
       table tbl {
           key = {
               tbl_ethernet_srcAddr: lpm;
               tbl_ipv4_totalLen   : exact;
           }
	   ...
       }
       apply {
           tbl_ethernet_srcAddr = h.ethernet.srcAddr;
           tbl_ipv4_totalLen = h.ipv4.totalLen;
           tbl_0.apply();
       }
  }
*/

const IR::Node* CopyMatchKeysToSingleStruct::preorder(IR::Key* keys) {
    // If any key field is from different structure, put all keys in metadata
    LOG3("Visiting " << keys);
    bool copyNeeded = false;
    const IR::Expression* firstKey = nullptr;

    if (!keys || keys->keyElements.size() == 0) {
        prune();
        return keys;
    }

    firstKey = keys->keyElements.at(0)->expression;
    for (auto key : keys->keyElements) {
        /* Key fields should be part of same header/metadata struct */
        if (key->expression->toString() != firstKey->toString()) {
            ::warning(ErrorType::WARN_UNSUPPORTED, "Mismatched header/metadata struct for key "
                      "elements in table %1%. Copying all match fields to metadata",
                       findOrigCtxt<IR::P4Table>()->name.toString());
            copyNeeded = true;
            break;
        }
    }

    if (!copyNeeded)
        // This prune will prevent the postoder(IR::KeyElement*) below from executing
        prune();
    else
        LOG3("Will pull out " << keys);
    return keys;
}

const IR::Node* CopyMatchKeysToSingleStruct::postorder(IR::KeyElement* element) {
    // If we got here we need to put the key element in metadata.
    LOG3("Extracting key element " << element);
    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(table);
    P4::TableInsertions* insertions;
    auto it = toInsert.find(table);
    if (it == toInsert.end()) {
        insertions = new P4::TableInsertions();
        toInsert.emplace(table, insertions);
    } else {
        insertions = it->second;
    }

    auto keyName =  element->expression->toString();

    /* All header fields are prefixed with "h.", prefix the match field with table name */
    if (keyName.startsWith("h.")) {
        keyName = keyName.replace('.','_');
        keyName = keyName.replace("h_",table->name.toString()+"_");
        auto keyPathExpr = new IR::PathExpression(IR::ID(refMap->newName(keyName)));
        auto decl = new IR::Declaration_Variable(keyPathExpr->path->name,
                                                 element->expression->type, nullptr);
        insertions->declarations.push_back(decl);
        auto right = element->expression;
        auto assign = new IR::AssignmentStatement(element->expression->srcInfo,
                                                  keyPathExpr, right);
        insertions->statements.push_back(assign);
        element->expression = keyPathExpr;
    }
    return element;
}

namespace Helpers {

boost::optional<P4::ExternInstance>
getExternInstanceFromProperty(const IR::P4Table* table,
                              const cstring& propertyName,
                              P4::ReferenceMap* refMap,
                              P4::TypeMap* typeMap,
                              bool *isConstructedInPlace) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return boost::none;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return boost::none;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (isConstructedInPlace) *isConstructedInPlace = expr->is<IR::ConstructorCallExpression>();
    if (expr->is<IR::ConstructorCallExpression>()
        && property->getAnnotation(IR::Annotation::nameAnnotation) == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "Table '%1%' has an anonymous table property '%2%' with no name annotation, "
                "which is not supported by P4Runtime", table->controlPlaneName(), propertyName);
        return boost::none;
    }
    auto name = property->controlPlaneName();
    auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap, name);
    if (!externInstance) {
        ::error(ErrorType::ERR_INVALID,
                "Expected %1% property value for table %2% to resolve to an "
                "extern instance: %3%", propertyName, table->controlPlaneName(),
                property);
        return boost::none;
    }

    return externInstance;
}

}

// create P4Table object that represents the matching part of the original P4
// table. This table sets the internal group_id or member_id which are used
// for subsequent table lookup.
std::tuple<const IR::P4Table*, cstring>
SplitP4TableCommon::create_match_table(const IR::P4Table *tbl) {
    cstring actionName;
    if (implementation == TableImplementation::ACTION_SELECTOR) {
        actionName = refMap->newName(tbl->name + "_set_group_id");
    } else if (implementation == TableImplementation::ACTION_PROFILE) {
        actionName = refMap->newName(tbl->name + "_set_member_id");
    } else {
        BUG("Unexpected table implementation type");
    }
    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    IR::Vector<IR::KeyElement> match_keys;
    for (auto key : tbl->getKey()->keyElements) {
        if (key->matchType->toString() != "selector") {
            match_keys.push_back(key);
        }
    }
    IR::IndexedVector<IR::ActionListElement> actionsList;

    auto actionCall = new IR::MethodCallExpression(new IR::PathExpression(actionName));
    actionsList.push_back(new IR::ActionListElement(actionCall));
    actionsList.push_back(new IR::ActionListElement(tbl->getDefaultAction()));
    IR::IndexedVector<IR::Property> properties;
    properties.push_back(new IR::Property("actions", new IR::ActionList(actionsList), false));
    properties.push_back(new IR::Property("key", new IR::Key(match_keys), false));
    properties.push_back(new IR::Property("default_action",
                         new IR::ExpressionValue(tbl->getDefaultAction()), false));
    if (tbl->getSizeProperty()) {
        properties.push_back(new IR::Property("size",
                             new IR::ExpressionValue(tbl->getSizeProperty()), false)); }
    auto match_table = new IR::P4Table(tbl->name, new IR::TableProperties(properties));
    return std::make_tuple(match_table, actionName);
}

const IR::P4Action*
SplitP4TableCommon::create_action(cstring actionName, cstring group_id, cstring param) {
    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    auto set_id = new IR::AssignmentStatement(
            new IR::PathExpression(group_id), new IR::PathExpression(param));
    auto parameter = new IR::Parameter(param, IR::Direction::None, IR::Type_Bits::get(32));
    auto action = new IR::P4Action(
            actionName, hidden, new IR::ParameterList({ parameter }),
            new IR::BlockStatement({ set_id }));
    return action;
}

const IR::P4Table*
SplitP4TableCommon::create_member_table(const IR::P4Table* tbl,
        cstring member_id) {
    IR::Vector<IR::KeyElement> member_keys;
    auto tableKeyEl = new IR::KeyElement(new IR::PathExpression(member_id),
            new IR::PathExpression(P4::P4CoreLibrary::instance.exactMatch.Id()));
    member_keys.push_back(tableKeyEl);
    IR::IndexedVector<IR::Property> member_properties;
    member_properties.push_back(new IR::Property("key", new IR::Key(member_keys), false));

    IR::IndexedVector<IR::ActionListElement> memberActionList;
    for (auto action : tbl->getActionList()->actionList)
        memberActionList.push_back(action);
    member_properties.push_back(new IR::Property("actions",
                                                 new IR::ActionList(memberActionList), false));
    if (tbl->getSizeProperty()) {
        member_properties.push_back(new IR::Property("size",
                                    new IR::ExpressionValue(tbl->getSizeProperty()), false)); }
    member_properties.push_back(new IR::Property("default_action",
                                new IR::ExpressionValue(tbl->getDefaultAction()), false));

    cstring memberTableName = refMap->newName(tbl->name + "_member_table");
    auto member_table = new IR::P4Table(memberTableName,
                                        new IR::TableProperties(member_properties));

    return member_table;
}

const IR::P4Table*
SplitP4TableCommon::create_group_table(const IR::P4Table* tbl, cstring group_id, cstring member_id,
        int n_groups_max, int n_members_per_group_max) {
    IR::Vector<IR::KeyElement> selector_keys;
    for (auto key : tbl->getKey()->keyElements) {
        if (key->matchType->toString() == "selector") {
            selector_keys.push_back(key);
        }
    }
    IR::IndexedVector<IR::Property> selector_properties;
    selector_properties.push_back(new IR::Property("selector", new IR::Key(selector_keys), false));
    selector_properties.push_back(new IR::Property("group_id",
        new IR::ExpressionValue(new IR::PathExpression(group_id)), false));
    selector_properties.push_back(new IR::Property("member_id",
        new IR::ExpressionValue(new IR::PathExpression(member_id)), false));

    selector_properties.push_back(new IR::Property("n_groups_max",
        new IR::ExpressionValue(new IR::Constant(n_groups_max)), false));
    selector_properties.push_back(new IR::Property("n_members_per_group_max",
        new IR::ExpressionValue(new IR::Constant(n_members_per_group_max)), false));
    selector_properties.push_back(new IR::Property("actions", new IR::ActionList({}), false));
    cstring selectorTableName = refMap->newName(tbl->name + "_group_table");
    auto group_table = new IR::P4Table(selectorTableName,
                                       new IR::TableProperties(selector_properties));
    return group_table;
}

const IR::Node* SplitActionSelectorTable::postorder(IR::P4Table* tbl) {
    bool isConstructedInPlace = false;
    auto instance = Helpers::getExternInstanceFromProperty(tbl, "psa_implementation",
                                                           refMap, typeMap, &isConstructedInPlace);
    if (!instance)
        return tbl;
    if (instance->type->name != "ActionSelector")
        return tbl;

    if (instance->arguments->size() != 3) {
        ::error("Incorrect number of argument on action selector %1%", *instance->name);
        return tbl;
    }

    if (!instance->arguments->at(1)->expression->is<IR::Constant>()) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "The 'size' argument of ActionSelector %1% must be a constant", *instance->name);
        return tbl;
    }

    int n_groups_max = instance->arguments->at(1)->expression->to<IR::Constant>()->asInt();
    if (!instance->arguments->at(2)->expression->is<IR::Constant>()) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "The 'outputWidth' argument of ActionSelector %1% must be a constant",
                *instance->name);
        return tbl;
    }
    auto outputWidth = instance->arguments->at(2)->expression->to<IR::Constant>()->asInt();
    if (outputWidth >= 32) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "The 'outputWidth' argument of ActionSelector %1% must be smaller than 32",
                *instance->name);
        return tbl;
    }
    int n_members_per_group_max = 1 << outputWidth;

    auto decls = new IR::IndexedVector<IR::Declaration>();

    cstring group_id = refMap->newName(tbl->name + "_group_id");
    cstring member_id = refMap->newName(tbl->name + "_member_id");

    auto group_id_decl = new IR::Declaration_Variable(group_id, IR::Type_Bits::get(32));
    auto member_id_decl = new IR::Declaration_Variable(member_id, IR::Type_Bits::get(32));
    decls->push_back(group_id_decl);
    decls->push_back(member_id_decl);

    // base table matches on non-selector key and set group_id
    cstring actionName;
    const IR::P4Table* match_table;
    std::tie(match_table, actionName) = create_match_table(tbl);
    auto action = create_action(actionName, group_id, "group_id");
    decls->push_back(action);
    decls->push_back(match_table);

    // group table match on group_id
    auto group_table = create_group_table(tbl, group_id, member_id,
                                          n_groups_max, n_members_per_group_max);
    decls->push_back(group_table);

    // member table match on member_id
    auto member_table = create_member_table(tbl, member_id);
    decls->push_back(member_table);

    match_tables.insert(tbl->name);
    group_tables.emplace(tbl->name, group_table->name);
    member_tables.emplace(tbl->name, member_table->name);

    return decls;
}

const IR::Node* SplitActionProfileTable::postorder(IR::P4Table* tbl) {
    bool isConstructedInPlace = false;
    auto instance = Helpers::getExternInstanceFromProperty(tbl, "psa_implementation",
            refMap, typeMap, &isConstructedInPlace);

    if (!instance || instance->type->name != "ActionProfile")
        return tbl;

    if (instance->arguments->size() != 1) {
        ::error("Incorrect number of argument on action profile %1%", *instance->name);
        return tbl;
    }

    if (!instance->arguments->at(0)->expression->is<IR::Constant>()) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "The 'size' argument of ActionProfile %1% must be a constant", *instance->name);
        return tbl;
    }

    auto decls = new IR::IndexedVector<IR::Declaration>();
    cstring member_id = refMap->newName(tbl->name + "_member_id");

    auto member_id_decl = new IR::Declaration_Variable(member_id, IR::Type_Bits::get(32));
    decls->push_back(member_id_decl);

    cstring actionName;
    const IR::P4Table* match_table;
    std::tie(match_table, actionName) = create_match_table(tbl);
    auto action = create_action(actionName, member_id, "member_id");
    decls->push_back(action);
    decls->push_back(match_table);

    // member table match on member_id
    auto member_table = create_member_table(tbl, member_id);
    decls->push_back(member_table);

    match_tables.insert(tbl->name);
    member_tables.emplace(tbl->name, member_table->name);

    return decls;
}

const IR::Node* SplitP4TableCommon::postorder(IR::MethodCallStatement *statement) {
    auto methodCall = statement->methodCall;
    auto mi = P4::MethodInstance::resolve(methodCall, refMap, typeMap);
    auto gen_apply = [](cstring table) {
        IR::Statement *expr = new IR::MethodCallStatement(
                new IR::MethodCallExpression(
                new IR::Member(new IR::PathExpression(table),
                IR::ID(IR::IApply::applyMethodName))));
        return expr;
    };
    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply())
            return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (match_tables.count(table->name) == 0)
            return statement;
        auto decls = new IR::IndexedVector<IR::StatOrDecl>();
        decls->push_back(statement);
        auto tableName = apply->object->getName().name;
        if (implementation == TableImplementation::ACTION_SELECTOR) {
            if (group_tables.count(tableName) == 0) {
                ::error("Unable to find group table %1%", tableName);
                return statement;
            }
            auto selectorTable = group_tables.at(tableName);
            decls->push_back(gen_apply(selectorTable));
        }
        if (member_tables.count(tableName) == 0) {
            ::error("Unable to find member table %1%", tableName);
            return statement;
        }
        auto memberTable = member_tables.at(tableName);
        decls->push_back(gen_apply(memberTable));
        return new IR::BlockStatement(*decls);
    }
    return statement;
}

// assume the RemoveMiss pass is applied
const IR::Node* SplitP4TableCommon::postorder(IR::IfStatement* statement) {
    auto cond = statement->condition;
    bool negated = false;
    if (auto neg = cond->to<IR::LNot>()) {
        // We handle !hit, which may have been created by the
        // removal of miss
        negated = true;
        cond = neg->expr; }

    if (!P4::TableApplySolver::isHit(cond, refMap, typeMap))
        return statement;
    if (!cond->is<IR::Member>())
        return statement;

    auto member = cond->to<IR::Member>();
    if (!member->expr->is<IR::MethodCallExpression>())
        return statement;
    auto mce = member->expr->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);

    auto apply_hit = [](cstring table, bool negated){
        IR::Expression *expr = new IR::Member(new IR::MethodCallExpression(
                new IR::Member(new IR::PathExpression(table),
                IR::ID(IR::IApply::applyMethodName))), "hit");
        if (negated)
            expr = new IR::LNot(expr);
        return expr;
    };

    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply())
            return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (match_tables.count(table->name) == 0)
            return statement;
        // an action selector t.apply() is converted to
        // t0.apply();  // base table
        // t1.apply();  // group table
        // t2.apply();  // member table
        //
        // if (!t.apply().hit) {
        //   foo();
        // }
        // is equivalent to
        // if (t0.apply().hit) {
        //   if (t1.apply().hit) {
        //      if (t2.apply().hit) {
        //        ; }
        //      else {
        //        foo(); }
        //   else {
        //      foo(); }
        // else {
        //   foo(); } } }
        //
        // if (t.apply().hit) {
        //   foo();
        // }
        // is equivalent to
        // if (t0.apply().hit) {
        //   if (t1.apply().hit) {
        //      if (t2.apply().hit) {
        //        foo();
        //      }
        //   }
        // }
        auto tableName = apply->object->getName().name;
        if (member_tables.count(tableName) == 0)
            return statement;
        auto memberTable = member_tables.at(tableName);
        auto t2stat = apply_hit(memberTable, negated);
        IR::Expression* t1stat = nullptr;
        if (implementation == TableImplementation::ACTION_SELECTOR) {
            auto selectorTable = group_tables.at(tableName);
            t1stat = apply_hit(selectorTable, negated);
        }

        IR::IfStatement* ret = nullptr;
        if (negated) {
            if (implementation == TableImplementation::ACTION_SELECTOR) {
                ret = new IR::IfStatement(cond, statement->ifTrue,
                        new IR::IfStatement(t1stat, statement->ifTrue,
                            new IR::IfStatement(t2stat, statement->ifTrue,
                                statement->ifFalse)));
            } else if (implementation == TableImplementation::ACTION_PROFILE) {
                ret = new IR::IfStatement(cond, statement->ifTrue,
                        new IR::IfStatement(t2stat, statement->ifTrue,
                            statement->ifFalse));
            }
        } else {
            if (implementation == TableImplementation::ACTION_SELECTOR) {
                ret = new IR::IfStatement(cond,
                        new IR::IfStatement(t1stat,
                            new IR::IfStatement(t2stat, statement->ifTrue,
                                statement->ifFalse),
                            statement->ifFalse),
                        statement->ifFalse);
            } else if (implementation == TableImplementation::ACTION_PROFILE) {
                ret = new IR::IfStatement(cond,
                        new IR::IfStatement(t2stat, statement->ifTrue,
                            statement->ifFalse),
                        statement->ifFalse);
            }
        }
        return ret;
    }
    return statement;
}

const IR::Node* SplitP4TableCommon::postorder(IR::SwitchStatement* statement) {
    auto expr = statement->expression;
    auto member = expr->to<IR::Member>();
    if (member->member != "action_run")
        return statement;
    if (!member->expr->is<IR::MethodCallExpression>())
        return statement;
    auto mce = member->expr->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    auto gen_apply = [](cstring table) {
        IR::Statement *expr = new IR::MethodCallStatement(
                new IR::MethodCallExpression(
                new IR::Member(new IR::PathExpression(table),
                IR::ID(IR::IApply::applyMethodName))));
        return expr;
    };
    auto gen_action_run = [](cstring table) {
        IR::Expression *expr = new IR::Member(new IR::MethodCallExpression(
                new IR::Member(new IR::PathExpression(table),
                IR::ID(IR::IApply::applyMethodName))), "action_run");
        return expr;
    };

    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply())
            return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (match_tables.count(table->name) == 0)
            return statement;

        // switch(t0.apply()) {} is converted to
        //
        // t0.apply();
        // t1.apply();
        // switch(t2.apply()) {}
        //
        auto decls = new IR::IndexedVector<IR::StatOrDecl>();
        auto t0stat = gen_apply(table->name);
        auto tableName = apply->object->getName().name;
        decls->push_back(t0stat);

        if (implementation == TableImplementation::ACTION_SELECTOR) {
            if (group_tables.count(tableName) == 0) {
                ::error("Unable to find group table %1%", tableName);
                return statement; }
            auto selectorTable = group_tables.at(tableName);
            auto t1stat = gen_apply(selectorTable);
            decls->push_back(t1stat);
        }

        if (member_tables.count(tableName) == 0) {
            ::error("Unable to find member table %1%", tableName);
            return statement; }
        auto memberTable = member_tables.at(tableName);
        auto t2stat = gen_action_run(memberTable);
        decls->push_back(new IR::SwitchStatement(statement->srcInfo,
                    t2stat, statement->cases));

        return new IR::BlockStatement(*decls);
    }
    return statement;
}

}  // namespace DPDK

