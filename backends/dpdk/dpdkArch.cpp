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
#include "dpdkHelpers.h"
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

bool isStandardMetadata(cstring name) {
    bool isStdMeta = name == "psa_ingress_parser_input_metadata_t" ||
                     name == "psa_ingress_input_metadata_t" ||
                     name == "psa_ingress_output_metadata_t" ||
                     name == "psa_egress_parser_input_metadata_t" ||
                     name == "psa_egress_input_metadata_t" ||
                     name == "psa_egress_output_metadata_t" ||
                     name == "psa_egress_deparser_input_metadata_t" ||
                     name == "pna_pre_input_metadata_t" ||
                     name == "pna_pre_output_metadata_t" ||
                     name == "pna_main_input_metadata_t" ||
                     name == "pna_main_output_metadata_t" ||
                     name == "pna_main_parser_input_metadata_t";
    return isStdMeta;
}

cstring TypeStruct2Name(const cstring s) {
    if (isStandardMetadata(s)) {
        return s.substr(0, s.size() - 2);
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

const IR::Type_Control *
ConvertToDpdkArch::rewriteControlType(const IR::Type_Control *c, cstring name) {
    auto applyParams = new IR::ParameterList();
    if (name == "Ingress" || name == "Egress") {
        auto header = c->applyParams->parameters.at(0);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    } else if (name == "PreControlT") {
        auto header = c->applyParams->parameters.at(0);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    } else if (name == "MainControlT") {
        auto header = c->applyParams->parameters.at(0);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    }
    auto tc = new IR::Type_Control(c->name, c->annotations, c->typeParameters,
                                   applyParams);
    return tc;
}

const IR::Type_Control *
ConvertToDpdkArch::rewriteDeparserType(const IR::Type_Control *c, cstring name) {
    auto applyParams = new IR::ParameterList();
    if (name == "IngressDeparser") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        auto header = c->applyParams->parameters.at(4);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(5);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    } else if (name == "EgressDeparser") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        auto header = c->applyParams->parameters.at(3);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(4);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    } else if (name == "MainDeparserT") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        auto header = c->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(2);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    }
    auto tc = new IR::Type_Control(c->name, c->annotations, c->typeParameters, applyParams);
    return tc;
}

// translate control block signature in arch.p4
const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Control *c) {
    const IR::Type_Control* t = nullptr;
    for (auto kv : structure->pipelines) {
        if (kv.second->type->name != c->name)
            continue;
        t = rewriteControlType(c, kv.first);
    }
    for (auto kv : structure->deparsers) {
        if (kv.second->type->name != c->name)
            continue;
        t = rewriteDeparserType(c, kv.first);
    }
    // Ingress, Egress, IngressDeparser, EgressDeparser are reserved name in psa.p4
    if (c->name == "Ingress" || c->name == "Egress") {
        structure->p4arch = "psa";
        t = rewriteControlType(c, c->name);
    } else if (c->name == "MainControlT") {
        structure->p4arch = "pna";
        t = rewriteControlType(c, c->name);
    } else if (c->name == "PreControlT") {
        t = rewriteControlType(c, c->name);
    } else if (c->name == "IngressDeparser" || c->name == "EgressDeparser") {
        t = rewriteDeparserType(c, c->name);
    } else if (c->name == "MainDeparserT") {
        t = rewriteDeparserType(c, c->name);
    }
    return t;
}

const IR::Type_Parser *
ConvertToDpdkArch::rewriteParserType(const IR::Type_Parser *p, cstring name) {
    auto applyParams = new IR::ParameterList();
    if (name == "IngressParser" || name == "EgressParser" ||
        name == "MainParserT") {
        applyParams->push_back(p->applyParams->parameters.at(0));
        auto header = p->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = p->applyParams->parameters.at(2);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    }
    auto tp = new IR::Type_Parser(p->name, p->annotations, p->typeParameters,
                                  applyParams);
    return tp;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Parser *p) {
    const IR::Type_Parser* t = nullptr;
    for (auto kv : structure->parsers) {
        if (kv.second->type->name != p->name)
            continue;
        t = rewriteParserType(p, kv.first);
    }
    if (p->name == "IngressParser" || p->name == "EgressParser") {
        t = rewriteParserType(p, p->name);
    } else if (p->name == "MainParserT") {
        t = rewriteParserType(p, p->name);
    }
    return t;
}

const IR::Node *ConvertToDpdkArch::preorder(IR::PathExpression *pe) {
    auto declaration = refMap->getDeclaration(pe->path);
    if (auto decl = declaration->to<IR::Parameter>()) {
        if (auto type = decl->type->to<IR::Type_Name>()) {
            LOG3("Expression: " << pe << std::endl <<
                 "  declaration: " << declaration);
            if (type->path->name == structure->header_type) {
                auto expr = new IR::PathExpression(IR::ID("h"));
                LOG3("  replaced by: " << expr);
                return expr;
            } else if (type->path->name == structure->local_metadata_type) {
                auto expr = new IR::PathExpression(IR::ID("m"));
                LOG3("  replaced by: " << expr);
                return expr;
            }
        }
    }
    return pe;
}

const IR::Node *ConvertToDpdkArch::preorder(IR::Member *m) {
    /* PathExpressions are handled in a separate preorder function
       Hence do not process them here */
    if (!m->expr->is<IR::Member>() &&
        !m->expr->is<IR::ArrayIndex>())
        prune();

    if (auto p = m->expr->to<IR::PathExpression>()) {
        auto declaration = refMap->getDeclaration(p->path);
        if (auto decl = declaration->to<IR::Parameter>()) {
            if (auto type = decl->type->to<IR::Type_Name>()) {
                LOG3("Member: " << m << std::endl <<
                      "  declaration: " << declaration);
                if (isStandardMetadata(type->path->name)) {
                    auto nm = new IR::Member(
                        new IR::PathExpression(IR::ID("m")),
                        IR::ID(TypeStruct2Name(type->path->name.name) + "_" +
                               m->member.name));
                    LOG3("  replaced by new member: " << nm);
                    return nm;
                } else if (type->path->name == structure->header_type) {
                    auto nm = new IR::Member(new IR::PathExpression(IR::ID("h")),
                                          IR::ID(m->member.name));
                    LOG3("  replaced by new member: " << nm);
                    return nm;
                } else if (type->path->name == structure->local_metadata_type) {
                    auto nm = new IR::Member(
                        new IR::PathExpression(IR::ID("m")),
                        IR::ID("local_metadata_" + m->member.name));
                    LOG3("  replaced by new member: " << nm);
                    return nm;
                }
            }
        }
    }
    return m;
}



const IR::Node *ConvertLookahead::preorder(IR::AssignmentStatement *statement) {
    if (!statement->right->is<IR::MethodCallExpression>())
        return statement;
    auto mce = statement->right->to<IR::MethodCallExpression>();

    if (mce->type->is<IR::Type_Header>())
        return statement;

    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ExternMethod>())
        return statement;
    auto em = mi->to<P4::ExternMethod>();
    if (em->originalExternType->name != P4::P4CoreLibrary::instance.packetIn.name ||
            em->method->name != P4::P4CoreLibrary::instance.packetIn.lookahead.name)
        return statement;

    IR::ID newHeaderFieldName;
    const IR::Type *newHeaderFieldType = nullptr;

    /**
     * Thanks to the fact that lookaheads are already expanded by P4::ExpandLookahead pass
     * and structures are already flattened, following cases should cover all possibilities.
     */
    if (auto left = statement->left->to<IR::PathExpression>()) {
        /**
         * Assignment to simple variable:
         * var_name = pkt.lookahead<T>();
         */
        newHeaderFieldName = left->path->name;
        newHeaderFieldType = left->type;
    } else if (auto left = statement->left->to<IR::Member>()) {
        if (auto pe = left->expr->to<IR::PathExpression>()) {
            /**
             * Assignment to a header or structure field:
             * hdr.field = pkt.lookahead<T>();
             */
            newHeaderFieldName = IR::ID(pe->path->name + "_" + left->member);
            newHeaderFieldType = left->type;
        } else if (left->expr->is<IR::Member>() &&
                left->expr->to<IR::Member>()->expr->is<IR::PathExpression>()) {
            /**
             * Assignment to a header field of a header which is a structure field:
             * hdrs.hdr_name.field = pkt.lookahead<T>();
             */
            auto leftMember = left->expr->to<IR::Member>();
            auto pe = leftMember->expr->to<IR::PathExpression>();
            newHeaderFieldName = IR::ID(
                    pe->path->name + "_" + leftMember->member + "_" + left->member);
            newHeaderFieldType = left->type;
        }
    }

    BUG_CHECK(newHeaderFieldType, "Unsupported lookahead usage: %1%", statement);

    LOG2("Converting lookahead in statement:" << std::endl << " " << statement);

    /**
     * Store new header in following format in the map:
     *
     * header var_name_header {
     *   T var_name;
     * }
     */
    auto program = findOrigCtxt<IR::P4Program>();
    IR::IndexedVector<IR::StructField> newHeaderFields;
    newHeaderFields.push_back(new IR::StructField(newHeaderFieldName, newHeaderFieldType));
    IR::ID newHeaderName(refMap->newName(newHeaderFieldName + "_header"));
    auto newHeader = new IR::Type_Header(newHeaderName, newHeaderFields);

    if (newHeaderMap.count(program)) {
        newHeaderMap.at(program).push_back(newHeader);
    } else {
        newHeaderMap.emplace(program, IR::IndexedVector<IR::Node>(newHeader));
    }

    LOG2("Adding new header:" << std::endl << " " << newHeader);

    /**
     * Store following declaration of new local variable which is of
     * new header type in the map:
     *
     * var_name_header var_name_tmp_h;
     */
    auto parser = findOrigCtxt<IR::P4Parser>();
    IR::ID newLocalVarName(refMap->newName(newHeaderFieldName + "_tmp_h"));
    auto newLocalVarType = new IR::Type_Name(newHeaderName);
    auto newLocalVar = new IR::Declaration_Variable(newLocalVarName, newLocalVarType);

    if (newLocalVarMap.count(parser)) {
        newLocalVarMap.at(parser).push_back(newLocalVar);
    } else {
        newLocalVarMap.emplace(parser, IR::IndexedVector<IR::Declaration>(newLocalVar));
    }

    LOG2("Adding new local variable:" << std::endl << " " << newLocalVar);

    /**
     * Replace current statement with 2 new statements:
     * - assignment statement with lookahead method using
     *   newly created header definiton
     * - assignment statement which assigns the field from
     *   newly created header into the original variable
     *
     * var_name_tmp_h = pkt.lookahead<var_name_header>();
     * var_name = var_name_tmp_h.var_name;
     */
    auto newStatements = new IR::IndexedVector<IR::StatOrDecl>;
    const IR::Expression *newLeft;
    const IR::Expression *newRight;
    const IR::AssignmentStatement *newStat;

    LOG2("Adding new statements:");

    newLeft = new IR::PathExpression(newHeader, new IR::Path(newLocalVarName));
    newRight = new IR::MethodCallExpression(newHeader, mce->method,
            new IR::Vector<IR::Type>(newLocalVarType));
    newStat = new IR::AssignmentStatement(newLeft, newRight);
    newStatements->push_back(newStat);
    LOG2(" " << newStat);

    newLeft = statement->left;
    newRight = new IR::Member(new IR::PathExpression(newLocalVarName), newHeaderFieldName);
    newStat = new IR::AssignmentStatement(newLeft, newRight);
    newStatements->push_back(newStat);
    LOG2(" " << newStat);

    return newStatements;
}

const IR::Node *ConvertLookahead::postorder(IR::P4Program *program) {
    auto origProgram = getOriginal()->to<IR::P4Program>();
    if (newHeaderMap.count(origProgram)) {
        IR::Vector<IR::Node>::iterator insertIter = std::find_if(program->objects.begin(),
                program->objects.end(), [](const IR::Node *n){return n->is<IR::P4Parser>();});
        auto newHeaders = newHeaderMap.at(origProgram);
        program->objects.insert(insertIter, newHeaders.begin(), newHeaders.end());

        LOG2("Following new headers inserted into program " << dbp(program) << ":");
        for (auto h : newHeaders) {
            LOG2(" " << h);
        }
    }
    return program;
}

const IR::Node *ConvertLookahead::postorder(IR::P4Parser *parser) {
    auto origParser = getOriginal()->to<IR::P4Parser>();
    if (newLocalVarMap.count(origParser)) {
        parser->parserLocals.append(newLocalVarMap.at(origParser));

        LOG2("Following new declarations inserted into parser " << dbp(parser) << ":");
        for (auto decl : newLocalVarMap.at(origParser)) {
            LOG2(" " << decl);
        }
    }
    return parser;
}



void CollectMetadataHeaderInfo::pushMetadata(const IR::ParameterList* params,
        std::list<int> indices) {
    for (auto idx : indices) {
        pushMetadata(params->getParameter(idx));
    }
}

void CollectMetadataHeaderInfo::pushMetadata(const IR::Parameter *p) {
    for (auto m : structure->used_metadata) {
        if (m->to<IR::Type_Name>()->path->name.name ==
            p->type->to<IR::Type_Name>()->path->name.name) {
            return;
        }
    }
    structure->used_metadata.push_back(p->type);
}

bool CollectMetadataHeaderInfo::preorder(const IR::P4Program *) {
    for (auto kv : structure->parsers) {
        if (kv.first == "IngressParser") {
            auto local_metadata = kv.second->getApplyParameters()->getParameter(2);
            structure->local_metadata_type = local_metadata->type->to<IR::Type_Name>()->path->name;
            auto header = kv.second->getApplyParameters()->getParameter(1);
            structure->header_type = header->type->to<IR::Type_Name>()->path->name;
            auto params = kv.second->getApplyParameters();
            pushMetadata(params, { 2, 3, 4, 5 });
        } else if (kv.first == "EgressParser") {
            auto params = kv.second->getApplyParameters();
            pushMetadata(params, { 2, 3, 4, 5, 6 });
        } else if (kv.first == "MainParserT") {
            auto local_metadata = kv.second->getApplyParameters()->getParameter(2);
            structure->local_metadata_type = local_metadata->type->to<IR::Type_Name>()->path->name;
            auto header = kv.second->getApplyParameters()->getParameter(1);
            structure->header_type = header->type->to<IR::Type_Name>()->path->name;
            auto params = kv.second->getApplyParameters();
            pushMetadata(params, { 2, 3 });
        }
    }

    for (auto kv : structure->pipelines) {
        if (kv.first == "Ingress") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        } else if (kv.first == "Egress") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        } else if (kv.first == "MainControlT") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        } else if (kv.first == "PreControlT") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        }
    }

    for (auto kv : structure->deparsers) {
        if (kv.first == "IngressDeparser") {
            auto deparser = kv.second->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, { 1, 2, 3, 5, 6 });
        } else if (kv.first == "EgressDeparser") {
            auto deparser = kv.second->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, { 1, 2, 4, 5, 6 });
        } else if (kv.first == "MainDeparser") {
            auto deparser = kv.second->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, { 1, 2, 3 });
        }
    }
    return true;
}

bool CollectMetadataHeaderInfo::preorder(const IR::Type_Struct *s) {
    for (auto m : structure->used_metadata) {
        if (m->to<IR::Type_Name>()->path->name.name == s->name.name) {
            for (auto field : s->fields) {
                auto sf = new IR::StructField(
                    IR::ID(TypeStruct2Name(s->name.name) + "_" + field->name),
                    field->type);
                LOG4("Adding metadata field: " << sf);
                structure->compiler_added_fields.push_back(sf);
            }
            return true;
        }
    }
    return true;
}

// This function collects the match key information of a table. This is later used for
// generating context JSON.
bool CollectTableInfo::preorder(const IR::Key *keys) {
    std::vector<cstring> tableKeys;
    if (!keys || keys->keyElements.size() == 0) {
        return false;
    }
    /* Push all non-selector keys to the key_map for this table.
       Selector keys become part of the selector table */
    for (auto key : keys->keyElements) {
        cstring keyTypeStr = key->expression->toString();
        if (key->matchType->toString() != "selector")
            tableKeys.push_back(keyTypeStr);
    }

    auto control = findOrigCtxt<IR::P4Control>();
    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(control);
    CHECK_NULL(table);
    structure->key_map.emplace(
               control->name.originalName + "_" + table->name.originalName, tableKeys);
    return false;
}

const IR::Node *InjectJumboStruct::preorder(IR::Type_Struct *s) {
    if (s->name == structure->local_metadata_type) {
        auto *annotations = new IR::Annotations(
            {new IR::Annotation(IR::ID("__metadata__"), {})});
        return new IR::Type_Struct(s->name, annotations, structure->compiler_added_fields);
    } else if (s->name == structure->header_type) {
        auto *annotations = new IR::Annotations(
            {new IR::Annotation(IR::ID("__packet_data__"), {})});
        return new IR::Type_Struct(s->name, annotations, s->fields);
    }
    return s;
}

const IR::Node *InjectOutputPortMetadataField::preorder(IR::Type_Struct *s) {
    if (structure->isPNA() && s->name.name == structure->local_metadata_type) {
        s->fields.push_back(new IR::StructField(
            IR::ID(PnaMainOutputMetadataOutputPortName), IR::Type_Bits::get(32)));
        LOG3("Metadata structure after injecting output port:" << std::endl << s);
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
        auto left_unroller = new ExpressionUnroll(refMap, structure);
        left_unroller->setCalledBy(this);
        auto right_unroller = new ExpressionUnroll(refMap, structure);
        right_unroller->setCalledBy(this);
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
        auto unroller = new ExpressionUnroll(refMap, structure);
        unroller->setCalledBy(this);
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
        } else if (auto c = u->to<IR::Cast>()) {
            un_expr = new IR::Cast(c->destType, root);
        } else {
            std::cout << u->node_type_name() << std::endl;
            BUG("Not Implemented");
        }
    } else {
        un_expr = u;
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

const IR::Node *IfStatementUnroll::postorder(IR::SwitchStatement *sw) {
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    expressionUnrollSanityCheck(sw->expression);
    auto unroller = new LogicalExpressionUnroll(refMap, structure);
    unroller->setCalledBy(this);
    sw->expression->apply(*unroller);
    for (auto i : unroller->stmt)
        code_block->push_back(i);

    auto control = findOrigCtxt<IR::P4Control>();

    for (auto d : unroller->decl)
        injector.collect(control, nullptr, d);
    if (unroller->root) {
        sw->expression = unroller->root;
    }
    code_block->push_back(sw);
    return new IR::BlockStatement(*code_block);
}

const IR::Node *IfStatementUnroll::postorder(IR::IfStatement *i) {
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    expressionUnrollSanityCheck(i->condition);
    auto unroller = new LogicalExpressionUnroll(refMap, structure);
    unroller->setCalledBy(this);
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
        } else if (auto c = u->to<IR::Cast>()) {
            un_expr = new IR::Cast(c->destType, root);
        } else {
            BUG("%1% Not Implemented", u);
        }
    } else {
        un_expr = u;
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

const IR::Node *CollectLocalVariables::preorder(IR::P4Program *p) {
    for (auto kv : structure->parsers) {
        locals_map.emplace(kv.first+"_parser", kv.second->parserLocals);
    }
    for (auto kv : structure->pipelines) {
        locals_map.emplace(kv.first, kv.second->controlLocals);
    }
    for (auto kv : structure->deparsers) {
        locals_map.emplace(kv.first+"_deparser", kv.second->controlLocals);
    }
    LOG4("Collecting local variables, locals_map:");
    for (auto kv : locals_map) {
        LOG4("  " << kv.first << ":");
        for (auto decl : kv.second) {
            LOG4("    " << decl);
        }
    }

    /**
     * It may happen that there are header definitions in the source P4 program
     * which are placed after the definition of main header structure, which is
     * perfectly fine if they are not used in the main header structure.
     * But if such header is used to declare local variable there is created
     * a new field of the same header type in the main header structure.
     * If the main header structure is placed before the definition of that
     * header, it is a problem for Type Checking.
     * There might also be header definitions internally created by compiler
     * which would cause the same problem.
     * Therefore here we move all structure and header definitons directly
     * before the main header structure to prevent this problem.
     */

    /* Find main header structure. */
    IR::Vector<IR::Node>::iterator headerTypeIter = std::find_if(p->objects.begin(),
            p->objects.end(), [this](const IR::Node *n){
                return n->is<IR::Type_Struct>() &&
                       n->to<IR::Type_Struct>()->name == structure->header_type;});
    auto mainHeaderStructure = *headerTypeIter;

    /**
     * Store header definitions which will be moved before the main header structure
     * into temporary vector and remove them temporarily from the objects vector.
     */
    IR::IndexedVector<IR::Node> defs;
    std::copy_if(headerTypeIter, p->objects.end(), std::back_inserter(defs),
            [this](const IR::Node *n){
                return n->is<IR::Type_StructLike>() &&
                n->to<IR::Type_StructLike>()->name != structure->header_type;});
    p->objects.erase(std::remove_if(headerTypeIter, p->objects.end(),
            [this](const IR::Node *n){
                return n->is<IR::Type_StructLike>() &&
                n->to<IR::Type_StructLike>()->name != structure->header_type;}), p->objects.end());

    /* Insert header definitions before the main header structure. */
    p->objects.insert(headerTypeIter, defs.begin(), defs.end());

    LOG2("Structure and header definitions:");
    for (auto d : defs) {
        LOG2(" " << d);
    }
    LOG2("moved before main header structure:" << std::endl << " " << mainHeaderStructure);

    return p;
}

const IR::Node *CollectLocalVariables::postorder(IR::Type_Struct *s) {
    if (s->name.name == structure->local_metadata_type) {
        for (auto kv : locals_map) {
            for (auto d : kv.second) {
                if (auto dv = d->to<IR::Declaration_Variable>()) {
                    auto type = typeMap->getType(dv, true);
                    if (type->is<IR::Type_Header>()) {
                        LOG3("Variable: " << dv << std::endl <<
                             " type: " << type << std::endl <<
                             " already added to: " << structure->header_type);
                    } else {
                        auto sf = new IR::StructField(
                            IR::ID(kv.first + "_" + dv->name.name), dv->type);
                        LOG2("New field: " << sf << std::endl <<
                             " type: " << type << std::endl <<
                             " added to: " << s->name.name);
                        s->fields.push_back(sf);
                    }
                } else if (!d->is<IR::P4Action>() && !d->is<IR::P4Table>() &&
                           !d->is<IR::Declaration_Instance>()) {
                    BUG("%1%: Unhandled declaration type", s);
                }
            }
        }
    } else if (s->name.name == structure->header_type) {
        for (auto kv : locals_map) {
            for (auto d : kv.second) {
                if (auto dv = d->to<IR::Declaration_Variable>()) {
                    auto type = typeMap->getType(dv, true);
                    if (type->is<IR::Type_Header>()) {
                        auto sf = new IR::StructField(
                            IR::ID(kv.first + "_" + dv->name.name), dv->type);
                        LOG2("New field: " << sf << std::endl <<
                             " type: " << type << std::endl <<
                             " added to: " << s->name.name);
                        s->fields.push_back(sf);
                    } else {
                        LOG3("Variable: " << dv << std::endl <<
                             " type: " << type << std::endl <<
                             " already added to: " << structure->local_metadata_type);
                    }
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
CollectLocalVariables::postorder(IR::PathExpression *p) {
    if (auto decl =
            refMap->getDeclaration(p->path)->to<IR::Declaration_Variable>()) {
        for (auto kv : locals_map) {
            for (auto d : kv.second) {
                if (d->equiv(*decl)) {
                    IR::ID name(kv.first + "_" + decl->name.name);
                    IR::Member *member;
                    if (typeMap->getType(decl, true)->is<IR::Type_Header>()) {
                        member = new IR::Member(new IR::PathExpression(IR::ID("h")), name);
                    } else {
                        member = new IR::Member(new IR::PathExpression(IR::ID("m")), name);
                    }
                    LOG2("Expression: " << p << " replaced by: " << member);
                    return member;
                }
            }
        }
        BUG("%1%: variable is not included in a control or parser block", p);
    }
    return p;
}

const IR::Node *CollectLocalVariables::postorder(IR::P4Control *c) {
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

const IR::Node *CollectLocalVariables::postorder(IR::P4Parser *p) {
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
        structure->args_struct_map.emplace(a->name.toString() + "_arg_t", l);
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
    for (auto kv : structure->pipelines) {
        if (kv.first == "Ingress" ||
            kv.first == "MainControlT") {
            for (auto kv : structure->args_struct_map) {
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
    for (auto obj : program->objects) {
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
       bit<16> tbl_ipv4_totalLen;     // in CollectLocalVariables pass.
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

    if (!keys || keys->keyElements.size() == 0) {
        prune();
        return keys;
    }

    cstring firstKeyStr = "";
    bool firstKeyHdr = false;

    if (auto firstKeyField = keys->keyElements.at(0)->expression->to<IR::Member>()) {
        firstKeyStr = firstKeyField->expr->toString();
        /* ReplaceMetadataHeaderName pass converts all header fields to the form
          "h.<header_name>.<field_name> and similarly metadata fields are prefixed with "m.".
           Check if the match key is part of a header by checking the "h" prefix. */
        if (firstKeyStr.startsWith("h"))
            firstKeyHdr = true;
    }

    /* Key fields should be part of same header/metadata struct */
    for (auto key : keys->keyElements) {
        cstring keyTypeStr = "";
        if (auto keyField = key->expression->to<IR::Member>()) {
            keyTypeStr =keyField->expr->toString();
        } else if (auto m = key->expression->to<IR::MethodCallExpression>()) {
            /* When isValid is present as table key, it should be moved to metadata */
            auto mi = P4::MethodInstance::resolve(m, refMap, typeMap);
            if (auto b = mi->to<P4::BuiltInMethod>())
                if (b->name == "isValid") {
                    copyNeeded = true;
                    break;
                }
        }
        if (firstKeyStr != keyTypeStr) {
            if (firstKeyHdr || keyTypeStr.startsWith("h")) {
                copyNeeded = true;
                break;
             }
         }
    }

    if (!copyNeeded) {
        // This prune will prevent the postorder(IR::KeyElement*) below from executing
        prune();
    } else {
        ::warning(ErrorType::WARN_MISMATCH, "Mismatched header/metadata struct for key "
                  "elements in table %1%. Copying all match fields to metadata",
                  findOrigCtxt<IR::P4Table>()->name.toString());
        LOG3("Will pull out " << keys);
    }
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
const IR::Node* CopyMatchKeysToSingleStruct::doStatement(const IR::Statement* statement,
                                           const IR::Expression *expression) {
    LOG3("Visiting " << getOriginal());
    P4::HasTableApply hta(refMap, typeMap);
    hta.setCalledBy(this);
    (void)expression->apply(hta);
    if (hta.table == nullptr)
        return statement;
    auto insertions = get(toInsert, hta.table);
    if (insertions == nullptr)
        return statement;
    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto assign : insertions->statements)
        result->push_back(assign);
    result->push_back(statement);
    auto block = new IR::BlockStatement(*result);
    return block;
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
        actionName = refMap->newName(tbl->name.originalName + "_set_group_id");
    } else if (implementation == TableImplementation::ACTION_PROFILE) {
        actionName = refMap->newName(tbl->name.originalName + "_set_member_id");
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

const IR::P4Table* SplitP4TableCommon::create_member_table(const IR::P4Table* tbl,
                                       cstring memberTableName, cstring member_id) {
    IR::Vector<IR::KeyElement> member_keys;
    auto tableKeyEl = new IR::KeyElement(new IR::PathExpression(member_id),
            new IR::PathExpression(P4::P4CoreLibrary::instance.exactMatch.Id()));
    member_keys.push_back(tableKeyEl);
    IR::IndexedVector<IR::Property> member_properties;
    member_properties.push_back(new IR::Property("key", new IR::Key(member_keys), false));

    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));

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

    auto member_table = new IR::P4Table(memberTableName, hidden,
                                        new IR::TableProperties(member_properties));

    return member_table;
}

const IR::P4Table* SplitP4TableCommon::create_group_table(const IR::P4Table* tbl,
                                       cstring selectorTableName, cstring group_id,
                                       cstring member_id, int n_groups_max,
                                       int n_members_per_group_max) {
    IR::Vector<IR::KeyElement> selector_keys;
    for (auto key : tbl->getKey()->keyElements) {
        if (key->matchType->toString() == "selector") {
            selector_keys.push_back(key);
        }
    }
    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
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
    auto group_table = new IR::P4Table(selectorTableName, hidden,
                                       new IR::TableProperties(selector_properties));
    return group_table;
}

const IR::Node* SplitActionSelectorTable::postorder(IR::P4Table* tbl) {
    bool isConstructedInPlace = false;
    bool isAsInstanceShared = false;
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

    // Remove the control block name prefix from instance name
    cstring instance_name = *instance->name;
    instance_name = instance_name.findlast('.');
    instance_name = instance_name.trim(".\t\n\r");

    cstring member_id = instance_name + "_member_id";
    cstring group_id = instance_name + "_group_id";

    // When multiple tables share an action selector instance, they share the metadata
    // field for member id and group id.
    for (auto mid : member_ids) {
        if (mid.second == member_id)
            isAsInstanceShared = true;
    }

    member_ids.emplace(tbl->name, member_id);
    group_ids.emplace(tbl->name, group_id);

    if (!isAsInstanceShared) {
        auto member_id_decl = new IR::Declaration_Variable(member_id, IR::Type_Bits::get(32));
        auto group_id_decl = new IR::Declaration_Variable(group_id, IR::Type_Bits::get(32));
        decls->push_back(group_id_decl);
        decls->push_back(member_id_decl);
    }

    // base table matches on non-selector key and set group_id
    cstring actionName;
    const IR::P4Table* match_table;
    std::tie(match_table, actionName) = create_match_table(tbl);
    auto action = create_action(actionName, group_id, "group_id");
    decls->push_back(action);
    decls->push_back(match_table);
    cstring member_table_name = instance_name;
    cstring group_table_name = member_table_name + "_sel";

    // Create group table and member table for the first table using this action selector instance
    if (!isAsInstanceShared) {
        // group table match on group_id
        auto group_table = create_group_table(tbl, group_table_name, group_id, member_id,
                                              n_groups_max, n_members_per_group_max);
        decls->push_back(group_table);

        // member table match on member_id
        auto member_table = create_member_table(tbl, member_table_name, member_id);
        decls->push_back(member_table);

        structure->group_tables.emplace(tbl->name, group_table);
        structure->member_tables.emplace(tbl->name, member_table);
    } else {
        // Use existing member table and group table created for this action selector instance
        for (auto mt : structure->member_tables) {
             if (mt.second->name == member_table_name) {
                 auto memTable = mt.second;
                 structure->member_tables.emplace(tbl->name, memTable);
                 break;
             }
        }
        for (auto mt : structure->group_tables) {
             if (mt.second->name == group_table_name) {
                 auto groupTable = mt.second;
                 structure->group_tables.emplace(tbl->name, groupTable);
                 break;
             }
        }
    }

    match_tables.insert(tbl->name);
    member_tables.emplace(tbl->name, member_table_name);
    group_tables.emplace(tbl->name, group_table_name);

    return decls;
}

const IR::Node* SplitActionProfileTable::postorder(IR::P4Table* tbl) {
    bool isConstructedInPlace = false;
    bool isApInstanceShared = false;
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

    // Remove the control block name prefix from instance name
    cstring instance_name = *instance->name;
    instance_name = instance_name.findlast('.');
    instance_name = instance_name.trim(".\t\n\r");

    auto decls = new IR::IndexedVector<IR::Declaration>();
    cstring member_id = instance_name + "_member_id";

    // When multiple tables share an action profile instance, they share the metadata
    // field for member id.
    for (auto mid : member_ids) {
        if (mid.second == member_id)
            isApInstanceShared = true;
    }

    member_ids.emplace(tbl->name, member_id);

    if (!isApInstanceShared) {
        auto member_id_decl = new IR::Declaration_Variable(member_id, IR::Type_Bits::get(32));
        decls->push_back(member_id_decl);
    }

    cstring actionName;
    const IR::P4Table* match_table;
    std::tie(match_table, actionName) = create_match_table(tbl);
    auto action = create_action(actionName, member_id, "member_id");
    decls->push_back(action);
    decls->push_back(match_table);
    cstring member_table_name = instance_name;

    // Create member table for the first table using this action profile instance
    if (!isApInstanceShared) {
        // member table match on member_id
        auto member_table = create_member_table(tbl, member_table_name, member_id);
        decls->push_back(member_table);
        structure->member_tables.emplace(tbl->name, member_table);
    } else {
        // Use existing member table created for this action profile instance
        for (auto mt : structure->member_tables) {
             if (mt.second->name == member_table_name) {
                 auto memTable = mt.second;
                 structure->member_tables.emplace(tbl->name, memTable);
                 break;
             }
        }
    }

    match_tables.insert(tbl->name);
    member_tables.emplace(tbl->name, member_table_name);
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
        auto tableName = apply->object->getName().name;

        // member_id and/or group_id must be initialized prior to member table apply.
        if (member_ids.count(tableName) != 0) {
            auto member_id = member_ids.at(tableName);
            decls->push_back(new IR::AssignmentStatement(
                        new IR::PathExpression(member_id),
                        new IR::Constant(IR::Type_Bits::get(32), 0))); }

        if (group_ids.count(tableName) != 0) {
            auto group_id = group_ids.at(tableName);
            decls->push_back(new IR::AssignmentStatement(
                        new IR::PathExpression(group_id),
                        new IR::Constant(IR::Type_Bits::get(32), 0))); }

        decls->push_back(statement);

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
    if (!member || member->member != "action_run")
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

void CollectAddOnMissTable::postorder(const IR::P4Table* t) {
    bool use_add_on_miss = false;
    auto add_on_miss = t->properties->getProperty("add_on_miss");
    if (add_on_miss == nullptr) return;
    if (add_on_miss->value->is<IR::ExpressionValue>()) {
        auto expr = add_on_miss->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1%: expected boolean for 'add_on_miss' property", add_on_miss);
            return;
        } else {
            use_add_on_miss = expr->to<IR::BoolLiteral>()->value;
            if (use_add_on_miss)
                structure->learner_tables.insert(t->name.name);
        }
    }

    /* sanity checks */
    auto default_action = t->properties->getProperty("default_action");
    if (use_add_on_miss && default_action == nullptr) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "%1%: add_on_miss property is defined, "
                "but default_action not specificed for table %2%", default_action, t->name);
        return;
    }
    if (default_action->value->is<IR::ExpressionValue>()) {
        auto expr = default_action->value->to<IR::ExpressionValue>()->expression;
        BUG_CHECK(expr->is<IR::MethodCallExpression>(),
            "%1%: expected expression to an action", default_action);
        auto mi = P4::MethodInstance::resolve(expr->to<IR::MethodCallExpression>(),
                refMap, typeMap);
        BUG_CHECK(mi->is<P4::ActionCall>(),
            "%1%: expected action in default_action", default_action);
        if (mi->to<P4::ActionCall>()->action->parameters->parameters.size() != 0) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1%: action cannot have action argument when used with add_on_miss",
                    default_action);
        }
    }
}

void CollectAddOnMissTable::postorder(const IR::MethodCallStatement *mcs) {
    auto mce = mcs->methodCall;
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ExternFunction>()) {
        return;
    }
    auto func = mi->to<P4::ExternFunction>();
    if (func->method->name != "add_entry") {
        return;
    }
    auto ctxt = findContext<IR::P4Action>();
    BUG_CHECK(ctxt != nullptr, "%1%: add_entry extern can only be used in an action", mcs);

    // assuming checking on number of arguments is already performed in frontend.
    BUG_CHECK(mce->arguments->size() == 2, "%1%: expected two arguments in add_entry extern", mcs);
    auto action = mce->arguments->at(0);
    // assuming syntax check is already performed earlier
    auto action_name = action->expression->to<IR::StringLiteral>()->value;
    structure->learner_actions.insert(action_name);
    return;
}

}  // namespace DPDK
