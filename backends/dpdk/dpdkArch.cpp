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
#include "dpdkUtils.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/typeMap.h"

namespace DPDK {

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
    if (!e->is<IR::Operation_Unary>() && !e->is<IR::MethodCallExpression>() &&
        !e->is<IR::Member>() && !e->is<IR::PathExpression>() && !e->is<IR::Operation_Binary>() &&
        !e->is<IR::Constant>() && !e->is<IR::BoolLiteral>()) {
        std::cerr << e->node_type_name() << std::endl;
        BUG("Untraversed node");
    }
}

const IR::Type_Control *ConvertToDpdkArch::rewriteControlType(const IR::Type_Control *c,
                                                              cstring name) {
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
    auto tc = new IR::Type_Control(c->name, c->annotations, c->typeParameters, applyParams);
    return tc;
}

const IR::Type_Control *ConvertToDpdkArch::rewriteDeparserType(const IR::Type_Control *c,
                                                               cstring name) {
    // Dpdk requires all local variables to be collected in a structure called
    // metadata, and we perform read/write on this struct fields, so here we unify
    // direction of this metadata decl to always be inout
    auto applyParams = new IR::ParameterList();
    if (name == "IngressDeparser") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        auto header = c->applyParams->parameters.at(4);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(5);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), IR::Direction::InOut, meta->type));
    } else if (name == "EgressDeparser") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        auto header = c->applyParams->parameters.at(3);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(4);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), IR::Direction::InOut, meta->type));
    } else if (name == "MainDeparserT") {
        applyParams->push_back(c->applyParams->parameters.at(0));
        auto header = c->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = c->applyParams->parameters.at(2);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), IR::Direction::InOut, meta->type));
    }
    auto tc = new IR::Type_Control(c->name, c->annotations, c->typeParameters, applyParams);
    return tc;
}

// translate control block signature in arch.p4
const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Control *c) {
    const IR::Type_Control *t = nullptr;
    for (auto kv : structure->pipelines) {
        if (kv.second->type->name != c->name) continue;
        t = rewriteControlType(c, kv.first);
    }
    for (auto kv : structure->deparsers) {
        if (kv.second->type->name != c->name) continue;
        t = rewriteDeparserType(c, kv.first);
    }
    // Ingress, Egress, IngressDeparser, EgressDeparser are reserved name in psa.p4
    if (c->name == "Ingress" || c->name == "Egress") {
        t = rewriteControlType(c, c->name);
    } else if (c->name == "MainControlT") {
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

const IR::Type_Parser *ConvertToDpdkArch::rewriteParserType(const IR::Type_Parser *p,
                                                            cstring name) {
    auto applyParams = new IR::ParameterList();
    if (name == "IngressParser" || name == "EgressParser" || name == "MainParserT") {
        applyParams->push_back(p->applyParams->parameters.at(0));
        auto header = p->applyParams->parameters.at(1);
        applyParams->push_back(new IR::Parameter(IR::ID("h"), header->direction, header->type));
        auto meta = p->applyParams->parameters.at(2);
        applyParams->push_back(new IR::Parameter(IR::ID("m"), meta->direction, meta->type));
    }
    auto tp = new IR::Type_Parser(p->name, p->annotations, p->typeParameters, applyParams);
    return tp;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Parser *p) {
    const IR::Type_Parser *t = nullptr;
    for (auto kv : structure->parsers) {
        if (kv.second->type->name != p->name) continue;
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
            LOG3("Expression: " << pe << std::endl << "  declaration: " << declaration);
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
    if (!m->expr->is<IR::Member>() && !m->expr->is<IR::ArrayIndex>()) prune();

    if (auto p = m->expr->to<IR::PathExpression>()) {
        auto declaration = refMap->getDeclaration(p->path);
        if (auto decl = declaration->to<IR::Parameter>()) {
            if (auto type = decl->type->to<IR::Type_Name>()) {
                LOG3("Member: " << m << std::endl << "  declaration: " << declaration);
                if (isStandardMetadata(type->path->name)) {
                    auto nm = new IR::Member(
                        new IR::PathExpression(IR::ID("m")),
                        IR::ID(TypeStruct2Name(type->path->name.name) + "_" + m->member.name));
                    LOG3("  replaced by new member: " << nm);
                    return nm;
                } else if (type->path->name == structure->header_type) {
                    auto nm =
                        new IR::Member(new IR::PathExpression(IR::ID("h")), IR::ID(m->member.name));
                    LOG3("  replaced by new member: " << nm);
                    return nm;
                } else if (type->path->name == structure->local_metadata_type) {
                    auto nm = new IR::Member(new IR::PathExpression(IR::ID("m")),
                                             IR::ID("local_metadata_" + m->member.name));
                    LOG3("  replaced by new member: " << nm);
                    return nm;
                }
            }
        }
    }
    return m;
}

void ConvertLookahead::Collect::postorder(const IR::AssignmentStatement *statement) {
    if (!statement->right->is<IR::MethodCallExpression>()) return;
    auto mce = statement->right->to<IR::MethodCallExpression>();

    if (mce->type->is<IR::Type_Header>()) return;

    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ExternMethod>()) return;
    auto em = mi->to<P4::ExternMethod>();
    if (em->originalExternType->name != P4::P4CoreLibrary::instance().packetIn.name ||
        em->method->name != P4::P4CoreLibrary::instance().packetIn.lookahead.name)
        return;

    LOG2("Collecting lookahead in statement:" << std::endl << " " << statement);

    /**
     * Store new header in following format in the map:
     *
     * header lookahead_tmp_hdr {
     *   T f;
     * }
     */
    IR::ID newHeaderFieldName("f");
    auto program = findOrigCtxt<IR::P4Program>();
    IR::IndexedVector<IR::StructField> newHeaderFields;
    newHeaderFields.push_back(new IR::StructField(newHeaderFieldName, statement->left->type));
    IR::ID newHeaderName(refMap->newName("lookahead_tmp_hdr"));
    auto newHeader = new IR::Type_Header(newHeaderName, newHeaderFields);

    repl->insertHeader(program, newHeader);

    /**
     * Store following declaration of new local variable which is of
     * new header type in the map:
     *
     * lookahead_tmp_hdr lookahead_tmp;
     */
    auto parser = findOrigCtxt<IR::P4Parser>();
    IR::ID newLocalVarName(refMap->newName("lookahead_tmp"));
    auto newLocalVarType = new IR::Type_Name(newHeaderName);
    auto newLocalVar = new IR::Declaration_Variable(newLocalVarName, newLocalVarType);

    repl->insertVar(parser, newLocalVar);

    /**
     * Replace current statement with 2 new statements:
     * - assignment statement with lookahead method using
     *   newly created header definiton
     * - assignment statement which assigns the field from
     *   newly created header into the original variable
     *
     * lookahead_tmp = pkt.lookahead<lookahead_tmp_hdr>();
     * var_name = lookahead_tmp.f;
     */
    auto newStatements = new IR::IndexedVector<IR::StatOrDecl>;
    const IR::Expression *newLeft;
    const IR::Expression *newRight;
    const IR::AssignmentStatement *newStat;

    newLeft = new IR::PathExpression(newHeader, new IR::Path(newLocalVarName));
    newRight = new IR::MethodCallExpression(newHeader, mce->method,
                                            new IR::Vector<IR::Type>(newLocalVarType));
    newStat = new IR::AssignmentStatement(newLeft, newRight);
    newStatements->push_back(newStat);

    newLeft = statement->left;
    newRight = new IR::Member(new IR::PathExpression(newLocalVarName), newHeaderFieldName);
    newStat = new IR::AssignmentStatement(newLeft, newRight);
    newStatements->push_back(newStat);

    repl->insertStatements(statement, newStatements);
}

const IR::Node *ConvertLookahead::Replace::postorder(IR::AssignmentStatement *as) {
    auto result = repl->getStatements(getOriginal()->to<IR::AssignmentStatement>());
    if (result == nullptr) return as;

    LOG2("Statement:" << std::endl << " " << as);
    LOG2("replaced by statements:");
    for (auto s : *result) {
        LOG2(" " << s);
    }

    return result;
}

const IR::Node *ConvertLookahead::Replace::postorder(IR::Type_Struct *s) {
    auto program = findOrigCtxt<IR::P4Program>();

    if (s->name != structure->header_type || program == nullptr) return s;

    auto result = repl->getHeaders(program);
    if (result == nullptr) return s;

    LOG2("Following new headers inserted into program " << dbp(program) << ":");
    for (auto h : *result) {
        LOG2(" " << h);
    }

    result->push_back(s);
    return result;
}

const IR::Node *ConvertLookahead::Replace::postorder(IR::P4Parser *parser) {
    auto result = repl->getVars(getOriginal()->to<IR::P4Parser>());
    if (result != nullptr) {
        parser->parserLocals.append(*result);
        LOG2("Following new declarations inserted into parser " << dbp(parser) << ":");
        for (auto decl : *result) {
            LOG2(" " << decl);
        }
    }
    return parser;
}

void CollectMetadataHeaderInfo::pushMetadata(const IR::ParameterList *params,
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
            pushMetadata(params, {2, 3, 4, 5});
        } else if (kv.first == "EgressParser") {
            auto params = kv.second->getApplyParameters();
            pushMetadata(params, {2, 3, 4, 5, 6});
        } else if (kv.first == "MainParserT") {
            auto local_metadata = kv.second->getApplyParameters()->getParameter(2);
            structure->local_metadata_type = local_metadata->type->to<IR::Type_Name>()->path->name;
            auto header = kv.second->getApplyParameters()->getParameter(1);
            structure->header_type = header->type->to<IR::Type_Name>()->path->name;
            auto params = kv.second->getApplyParameters();
            pushMetadata(params, {2, 3});
        }
    }

    for (auto kv : structure->pipelines) {
        if (kv.first == "Ingress") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, {1, 2, 3});
        } else if (kv.first == "Egress") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, {1, 2, 3});
        } else if (kv.first == "MainControlT") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, {1, 2, 3});
        } else if (kv.first == "PreControlT") {
            auto control = kv.second->to<IR::P4Control>();
            auto params = control->getApplyParameters();
            pushMetadata(params, {1, 2, 3});
        }
    }

    for (auto kv : structure->deparsers) {
        if (kv.first == "IngressDeparser") {
            auto deparser = kv.second->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, {1, 2, 3, 5, 6});
        } else if (kv.first == "EgressDeparser") {
            auto deparser = kv.second->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, {1, 2, 4, 5, 6});
        } else if (kv.first == "MainDeparser") {
            auto deparser = kv.second->to<IR::P4Control>();
            auto params = deparser->getApplyParameters();
            pushMetadata(params, {1, 2, 3});
        }
    }
    return true;
}

bool CollectMetadataHeaderInfo::preorder(const IR::Type_Struct *s) {
    for (auto m : structure->used_metadata) {
        if (m->to<IR::Type_Name>()->path->name.name == s->name.name) {
            for (auto field : s->fields) {
                auto sf = new IR::StructField(
                    IR::ID(TypeStruct2Name(s->name.name) + "_" + field->name), field->type);
                LOG4("Adding metadata field: " << sf);
                structure->compiler_added_fields.push_back(sf);
            }
            return true;
        }
    }
    return true;
}

/* This function processes each header structure and aligns each non-aligned header
   into 8-bit aligned header.[Restricted to only contiguous non-aligned header fields]
   For example :
       header ipv4_t {
            bit<4>  version;
            bit<4>  ihl;
            bit<8>  diffserv;
            bit<32> totalLen;
            bit<16> identification;
            bit<3>  flags;
            bit<13> fragOffset;
            bit<8>  ttl;
            bit<8>  protocol;
            bit<16> hdrChecksum;
            bit<32> srcAddr;
            bit<32> dstAddr;
       }
       is converted into
       header ipv4_t {
            bit<8>  version_ihl;      // "version" and "ihl" combined for 8-bit alignment
                                         resulting into 8 bit "version_ihl" field
            bit<8>  diffserv;
            bit<32> totalLen;
            bit<16> identification;
            bit<16> flags_fragOffset; // "flag" and "fragOffset" combined for 8-bit aligment
                                         resulting into 16-bit "flags_fragOffset" field
            bit<8>  ttl;
            bit<8>  protocol;
            bit<16> hdrChecksum;
            bit<32> srcAddr;
            bit<32> dstAddr;
       }
*/
const IR::Node *AlignHdrMetaField::preorder(IR::Type_StructLike *st) {
    if (st->is<IR::Type_Header>()) {
        unsigned size_sum_so_far = 0;
        bool all_hdr_field_aligned = true;
        for (auto field : st->fields) {
            unsigned width;
            if (auto type = (*field).type->to<IR::Type_Bits>())
                width = type->width_bits();
            else if (auto type = (*field).type->to<IR::Type_Varbits>()) {
                width = type->width_bits();
            } else {
                BUG("header fields should be of type bit<> or varbit<>"
                    "found this %1%",
                    field->toString());
            }
            size_sum_so_far += width;
            if ((width & 0x7) != 0) {
                all_hdr_field_aligned = false;
            }
        }
        if ((size_sum_so_far & 0x7) != 0) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "'%1%' is not 8-bit aligned",
                    st->name.name);
            return st;
        }
        if (all_hdr_field_aligned) {
            return st;
        }
    } else {
        return st;
    }

    // fields is used to create header with modified fields
    auto fields = new IR::IndexedVector<IR::StructField>;
    unsigned size_sum_so_far = 0;
    for (auto field : st->fields) {
        unsigned width;
        if (auto type = (*field).type->to<IR::Type_Bits>())
            width = type->width_bits();
        else {
            auto type0 = (*field).type->to<IR::Type_Varbits>();
            width = type0->width_bits();
        }
        if ((width & 0x7) == 0 && size_sum_so_far == 0) {
            fields->push_back(field);
            continue;
        } else {
            if (size_sum_so_far == 0 || (size_sum_so_far & 0x7) != 0) {
                size_sum_so_far += width;
                fieldInfo obj;
                obj.fieldWidth = width;
                // Storing the non-aligned field
                field_name_list.emplace(field->name, obj);
            }
        }
        cstring modifiedName = "";
        auto size = field_name_list.size();
        unsigned i = 0;
        // Check if the sum of width of non-aligned field is divisble by 8.
        if (size_sum_so_far && (size_sum_so_far % 8 == 0)) {
            // Form the field with all non-aligned field stored in "field_name_list"
            for (auto s = field_name_list.begin(); s != field_name_list.end(); s++, i++) {
                if ((i + 1) < size)
                    modifiedName += s->first + "_";
                else
                    modifiedName += s->first;
            }
            unsigned offset = 0;
            /* Store information about each non-aligned field
                For eg : ModifiedName, header str, width, offset, and its
                        lsb and msb in modified field

                        header ipv4_t {
                            ...
                            bit<3>  flags;
                            bit<13> fragOffset;
                            ...
                        }
                        is converted into

                        header ipv4_t {
                            ...
                            bit<16>  flags_fragOffset;
                            ...
                        }
                        Here, for "flags", information saved are :
                            ModifiedName = flags_fragOffset;
                            header str   = ipv4_t; [This is used if multiple headers
                                                        have field with same name]
                            width        = 16;
                            offset       = 3;
                            lsb          = 3;
                            msb          = 15;
            */
            for (auto s = field_name_list.begin(); s != field_name_list.end(); s++) {
                hdrFieldInfo fieldObj;
                fieldObj.modifiedName = modifiedName;
                fieldObj.headerStr = st->name.name;
                fieldObj.modifiedWidth = size_sum_so_far;
                fieldObj.fieldWidth = s->second.fieldWidth;
                fieldObj.lsb = offset;
                fieldObj.msb = offset + s->second.fieldWidth - 1;
                fieldObj.offset = offset;
                structure->hdrFieldInfoList[s->first].push_back(fieldObj);
                offset += s->second.fieldWidth;
            }
            fields->push_back(
                new IR::StructField(IR::ID(modifiedName), IR::Type_Bits::get(size_sum_so_far)));
            size_sum_so_far = 0;
            modifiedName = "";
            field_name_list.clear();
        }
    }
    /* Throw error if there is non-aligned field present at the end in header */
    if (size_sum_so_far != 0) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "8-bit Alignment for Header Structure '%1%' is not possible as no more header"
                " fields available in header to combine. DPDK does not support non-aligned"
                " header fields.",
                st->name.name);
        return st;
    }
    return new IR::Type_Header(IR::ID(st->name), st->annotations, *fields);
    return st;
}

/* This function replaces field accesses to headers that have been modified and replaces
   them with equivalent expressions on the modified headers

   For eg : If below header
       header ipv4_t {
            ...
            bit<3>  flags;
            bit<13> fragOffset;
            ...
       }
       is converted into
       header ipv4_t {
            ...
            bit<16>  flags_fragOffset;
            ...
       }
       and there is a field used in P4 as mentioned below
           if (hdrs.ipv4.fragOffset == 4) ....

       Then, the above condition is converted into
           if (hdrs.ipv4.flags_fragOffset[15:3] == 4) ....
*/
const IR::Node *AlignHdrMetaField::preorder(IR::Member *m) {
    cstring hdrStrName = "";
    /* Get the member's header structure name */
    if ((m != nullptr) && (m->expr != nullptr) && (m->expr->type != nullptr) &&
        (m->expr->type->is<IR::Type_Header>())) {
        auto str_type = m->expr->type->to<IR::Type_Header>();
        hdrStrName = str_type->name.name;
    } else {
        return m;
    }
    /* Check if the member has been modified due to 8-bit header field aligment
       and its related information is stored in "hdrFieldInfoList" data structure */
    auto it = structure->hdrFieldInfoList.find(m->member.name);
    if (it != structure->hdrFieldInfoList.end()) {
        for (auto memVec : structure->hdrFieldInfoList[m->member.name]) {
            /* header structure name is matched to handle the scenario where
               two different headers have field with same name */
            if (memVec.headerStr != hdrStrName) continue;
            auto mem = new IR::Member(m->expr, IR::ID(memVec.modifiedName));
            auto sliceMem =
                new IR::Slice(mem->clone(), (memVec.offset + memVec.fieldWidth - 1), memVec.offset);
            return sliceMem;
        }
    }
    return m;
}

/* This function processes the metadata structure and modify the metadata field width
   to 32/64 bits if it is not 8-bit aligned */
const IR::Node *ReplaceHdrMetaField::postorder(IR::Type_Struct *st) {
    auto fields = new IR::IndexedVector<IR::StructField>;
    if (st->is<IR::Type_Struct>()) {
        for (auto field : st->fields) {
            if (auto t = (*field).type->to<IR::Type_Bits>()) {
                auto width = t->width_bits();
                if (width % 8 != 0) {
                    width = getMetadataFieldWidth(width);
                    fields->push_back(
                        new IR::StructField(IR::ID(field->name), IR::Type_Bits::get(width)));
                } else {
                    fields->push_back(field);
                }
            } else {
                fields->push_back(field);
            }
        }
        return new IR::Type_Struct(IR::ID(st->name), st->annotations, *fields);
    }
    return st;
}

// This function collects the match key information of a table. This is later used for
// generating context JSON.
bool CollectTableInfo::preorder(const IR::Key *keys) {
    std::vector<std::pair<cstring, cstring>> tableKeys;
    if (!keys || keys->keyElements.size() == 0) {
        return false;
    }
    /* Push all non-selector keys to the key_map for this table.
       Selector keys become part of the selector table */
    for (auto key : keys->keyElements) {
        cstring keyName = key->expression->toString();
        cstring keyNameAnnon = key->expression->toString();
        ;
        auto annon = key->getAnnotation(IR::Annotation::nameAnnotation);
        if (key->matchType->toString() != "selector") {
            if (annon != nullptr) keyNameAnnon = annon->getSingleString();
            tableKeys.push_back(std::make_pair(keyName, keyNameAnnon));
        }
    }

    auto control = findOrigCtxt<IR::P4Control>();
    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(control);
    CHECK_NULL(table);
    structure->key_map.emplace(control->name.originalName + "_" + table->name.originalName,
                               tableKeys);
    return false;
}

const IR::Node *InjectJumboStruct::preorder(IR::Type_Struct *s) {
    if (s->name == structure->local_metadata_type) {
        auto *annotations = new IR::Annotations({new IR::Annotation(IR::ID("__metadata__"), {})});
        return new IR::Type_Struct(s->name, annotations, structure->compiler_added_fields);
    } else if (s->name == structure->header_type) {
        auto *annotations =
            new IR::Annotations({new IR::Annotation(IR::ID("__packet_data__"), {})});
        return new IR::Type_Struct(s->name, annotations, s->fields);
    }
    return s;
}

const IR::Node *InjectFixedMetadataField::preorder(IR::Type_Struct *s) {
    if (s->name.name == structure->local_metadata_type) {
        if (structure->isPNA()) {
            s->fields.push_back(new IR::StructField(IR::ID(PnaMainOutputMetadataOutputPortName),
                                                    IR::Type_Bits::get(32)));
        }
        s->fields.push_back(
            new IR::StructField(IR::ID(DirectResourceTableEntryIndex), IR::Type_Bits::get(32)));
        LOG3("Metadata structure after injecting Fixed metadata fields:" << std::endl << s);
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
        if (!left_tmp) left_tmp = bin->left;
        if (!right_tmp) right_tmp = bin->right;
        for (auto s : left_unroller->stmt) code_block->push_back(s);
        for (auto d : left_unroller->decl) injector.collect(control, parser, d);
        for (auto s : right_unroller->stmt) code_block->push_back(s);
        for (auto d : right_unroller->decl) injector.collect(control, parser, d);
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
        for (auto s : unroller->stmt) code_block->push_back(s);
        for (auto d : unroller->decl) injector.collect(control, parser, d);
        if (!un_tmp) un_tmp = un->expr;
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
    if (!left_root) left_root = bin->left;
    if (!right_root) right_root = bin->right;

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
    auto unroller = new LogicalExpressionUnroll(refMap);
    unroller->setCalledBy(this);
    sw->expression->apply(*unroller);
    for (auto i : unroller->stmt) code_block->push_back(i);

    auto control = findOrigCtxt<IR::P4Control>();
    auto parser = findOrigCtxt<IR::P4Parser>();

    for (auto d : unroller->decl) injector.collect(control, parser, d);
    if (unroller->root) {
        sw->expression = unroller->root;
    }
    code_block->push_back(sw);
    return new IR::BlockStatement(*code_block);
}

const IR::Node *IfStatementUnroll::postorder(IR::IfStatement *i) {
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    expressionUnrollSanityCheck(i->condition);
    auto unroller = new LogicalExpressionUnroll(refMap);
    unroller->setCalledBy(this);
    i->condition->apply(*unroller);
    for (auto i : unroller->stmt) code_block->push_back(i);

    auto control = findOrigCtxt<IR::P4Control>();
    auto parser = findOrigCtxt<IR::P4Parser>();
    for (auto d : unroller->decl) injector.collect(control, parser, d);
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

// TODO(GordonWuCn): simplify with a postorder visitor if it is a statement,
// return the expression, else introduce a temporary for the current expression
// and return the temporary in a pathexpression.
bool LogicalExpressionUnroll::preorder(const IR::Operation_Unary *u) {
    expressionUnrollSanityCheck(u->expr);

    // If the expression is a methodcall expression, do not insert a temporary
    // variable to represent the value of the methodcall. Instead the
    // methodcall is converted to a dpdk branch instruction in a later pass.
    if (u->expr->is<IR::MethodCallExpression>()) return false;

    // if the expression is apply().hit or apply().miss, do not insert a temporary
    // variable.
    if (auto member = u->expr->to<IR::Member>()) {
        if (member->expr->is<IR::MethodCallExpression>() &&
            (member->member == IR::Type_Table::hit || member->member == IR::Type_Table::miss)) {
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
    if (!left_root) left_root = bin->left;
    if (!right_root) right_root = bin->right;

    if (is_logical(bin)) {
        IR::Operation_Binary *bin_expr;
        bin_expr = bin->clone();
        bin_expr->left = left_root;
        bin_expr->right = right_root;
        root = bin_expr;
    } else {
        auto tmp = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
        root = tmp;
        decl.push_back(new IR::Declaration_Variable(tmp->path->name, bin->type));
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

const IR::Node *ConvertBinaryOperationTo2Params::postorder(IR::AssignmentStatement *a) {
    auto right = a->right;
    auto left = a->left;
    // This pass does not apply to 'bool foo = (a == b)' or 'bool foo = (a > b)' etc.
    if (right->to<IR::Operation_Relation>()) return a;
    if (auto r = right->to<IR::Operation_Binary>()) {
        // Array Index is replaced with flattened variable for array.
        if (right->to<IR::ArrayIndex>()) return a;
        if (!isSimpleExpression(r->right) || !isSimpleExpression(r->left))
            BUG("%1%: Statement Unroll pass failed", a);
        if (left->equiv(*r->left)) {
            return a;
        } else if (left->equiv(*r->right)) {
            IR::Operation_Binary *bin_expr;
            bin_expr = r->clone();
            if (isCommutativeBinaryOperation(r)) {
                bin_expr->left = r->right;
                bin_expr->right = r->left;
                a->right = bin_expr;
                return a;
            } else {
                // Non-commutative expressions like "a = b << a" should be replaced with the
                // following block of statments
                // tmp = b;
                // tmp = tmp << a;
                // a = tmp;
                IR::IndexedVector<IR::StatOrDecl> code_block;
                auto control = findOrigCtxt<IR::P4Control>();
                auto parser = findOrigCtxt<IR::P4Parser>();
                auto tmpOp1 = new IR::PathExpression(IR::ID(refMap->newName("tmp")));
                injector.collect(control, parser,
                                 new IR::Declaration_Variable(tmpOp1->path->name, left->type));
                code_block.push_back(new IR::AssignmentStatement(tmpOp1, r->left));
                bin_expr->left = tmpOp1;
                code_block.push_back(new IR::AssignmentStatement(tmpOp1, bin_expr));
                code_block.push_back(new IR::AssignmentStatement(left, tmpOp1));
                return new IR::BlockStatement(code_block);
            }
        } else {
            IR::IndexedVector<IR::StatOrDecl> code_block;
            const IR::Expression *src1;
            const IR::Expression *src2;
            if (isNonConstantSimpleExpression(r->left)) {
                src1 = r->left;
                src2 = r->right;
            } else if (isNonConstantSimpleExpression(r->right)) {
                if (isCommutativeBinaryOperation(r)) {
                    src1 = r->right;
                    src2 = r->left;
                } else {
                    src1 = r->left;
                    src2 = r->right;
                }
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
        insert(kv.first + "_parser", &kv.second->parserLocals);
    }
    for (auto kv : structure->pipelines) {
        insert(kv.first, &kv.second->controlLocals);
    }
    for (auto kv : structure->deparsers) {
        insert(kv.first + "_deparser", &kv.second->controlLocals);
    }
    LOG4("Collecting local variables, localsMap:");
    for (auto kv : localsMap) {
        LOG4(" " << dbp(kv.first) << ": " << kv.second);
    }
    return p;
}

const IR::Node *CollectLocalVariables::postorder(IR::Type_Struct *s) {
    if (s->name.name == structure->local_metadata_type) {
        for (auto sf : structure->key_fields) {
            s->fields.push_back(sf);
        }
        for (auto kv : localsMap) {
            auto dv = kv.first;
            auto type = typeMap->getType(dv, true);
            if (type->is<IR::Type_Header>()) {
                LOG3("Variable: " << dv << std::endl
                                  << " type: " << type << std::endl
                                  << " already added to: " << structure->header_type);
            } else if (auto strct = type->to<IR::Type_Struct>()) {
                for (auto field : strct->fields) {
                    auto sf =
                        new IR::StructField(IR::ID(kv.second + "_" + field->name), field->type);
                    LOG2("New field: " << sf << std::endl
                                       << " type: " << field->type << std::endl
                                       << " added to: " << s->name.name);
                    s->fields.push_back(sf);
                }
            } else {
                auto sf = new IR::StructField(IR::ID(kv.second), dv->type);
                LOG2("New field: " << sf << std::endl
                                   << " type: " << type << std::endl
                                   << " added to: " << s->name.name);
                s->fields.push_back(sf);
            }
        }
    } else if (s->name.name == structure->header_type) {
        for (auto kv : localsMap) {
            auto dv = kv.first;
            auto type = typeMap->getType(dv, true);
            if (type->is<IR::Type_Header>()) {
                auto sf = new IR::StructField(IR::ID(kv.second), dv->type);
                LOG2("New field: " << sf << std::endl
                                   << " type: " << type << std::endl
                                   << " added to: " << s->name.name);
                s->fields.push_back(sf);
            } else {
                LOG3("Variable: " << dv << std::endl
                                  << " type: " << type << std::endl
                                  << " already added to: " << structure->local_metadata_type);
            }
        }
    }
    return s;
}

const IR::Node *CollectLocalVariables::postorder(IR::PathExpression *p) {
    if (auto decl = refMap->getDeclaration(p->path)->to<IR::Declaration_Variable>()) {
        if (localsMap.count(decl)) {
            IR::ID name(localsMap.at(decl));
            IR::Member *member;
            if (typeMap->getType(decl, true)->is<IR::Type_Header>()) {
                member = new IR::Member(new IR::PathExpression(IR::ID("h")), name);
            } else {
                member = new IR::Member(new IR::PathExpression(IR::ID("m")), name);
            }
            LOG2("Expression: " << p << " replaced by: " << member);
            return member;
        } else {
            BUG("%1%: variable is not included in a control or parser block", p);
        }
    }
    return p;
}

const IR::Node *CollectLocalVariables::postorder(IR::Member *mem) {
    // Ensure that member's expression is a member expression
    // then convert like (m.field.field_0 to m.field_field_0)
    auto expr = mem->expr->to<IR::Member>();
    if (expr && expr->expr->toString() == "m")
        return new IR::Member(new IR::PathExpression(IR::ID("m")),
                              IR::ID(expr->member.toString() + "_" + mem->member.toString()));
    else
        return mem;
}

const IR::Node *CollectLocalVariables::postorder(IR::P4Control *c) {
    IR::IndexedVector<IR::Declaration> decls;
    for (auto d : c->controlLocals) {
        if (d->is<IR::Declaration_Instance>() || d->is<IR::P4Action>() || d->is<IR::P4Table>()) {
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

/* This function stores the information about parameters of default action
   for each table */
void DefActionValue::postorder(const IR::P4Table *t) {
    auto default_action = t->properties->getProperty("default_action");
    if (default_action != nullptr && default_action->value->is<IR::ExpressionValue>()) {
        auto expr = default_action->value->to<IR::ExpressionValue>()->expression;
        auto mi =
            P4::MethodInstance::resolve(expr->to<IR::MethodCallExpression>(), refMap, typeMap);
        BUG_CHECK(mi->is<P4::ActionCall>(), "%1%: expected action in default_action",
                  default_action);
        structure->defActionParamList[t->toString()] =
            new IR::ParameterList(mi->to<P4::ActionCall>()->action->parameters->parameters);
    }
}

const IR::Node *PrependPDotToActionArgs::postorder(IR::P4Action *a) {
    if (a->parameters->size() > 0) {
        auto l = new IR::IndexedVector<IR::Parameter>;
        for (auto p : a->parameters->parameters) {
            l->push_back(p);
        }
        structure->args_struct_map.emplace(a->name.name + "_arg_t", l);
        auto new_l = new IR::IndexedVector<IR::Parameter>;
        new_l->push_back(new IR::Parameter(IR::ID("t"), IR::Direction::None,
                                           new IR::Type_Name(IR::ID(a->name.name + "_arg_t"))));
        a->parameters = new IR::ParameterList(*new_l);
    }
    return a;
}

const IR::Node *PrependPDotToActionArgs::postorder(IR::P4Program *program) {
    auto new_objs = new IR::Vector<IR::Node>;
    for (auto kv : structure->pipelines) {
        if (kv.first == "Ingress" || kv.first == "MainControlT") {
            for (auto kv : structure->args_struct_map) {
                auto fields = new IR::IndexedVector<IR::StructField>;
                for (auto field : *kv.second) {
                    fields->push_back(new IR::StructField(field->name.toString(), field->type));
                }
                new_objs->push_back(new IR::Type_Struct(IR::ID(kv.first), *fields));
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

const IR::Node *PrependPDotToActionArgs::preorder(IR::MethodCallExpression *mce) {
    auto property = findContext<IR::Property>();
    if (!property) return mce;
    if (property->name != "default_action" && property->name != "entries") return mce;
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ActionCall>()) return mce;
    // We assume all action call has been converted to take struct as input,
    // therefore, the arguments must be passed in as a list expression
    if (mce->arguments->size() == 0) return mce;
    IR::Vector<IR::Expression> components;
    for (auto arg : *mce->arguments) {
        components.push_back(arg->expression);
    }
    auto arguments = new IR::Vector<IR::Argument>;
    arguments->push_back(new IR::Argument(new IR::ListExpression(components)));
    return new IR::MethodCallExpression(mce->method, arguments);
}

const IR::Node *DismantleMuxExpressions::preorder(IR::Mux *expression) {
    // We always dismantle muxes for dpdk
    auto type = typeMap->getType(getOriginal(), true);
    visit(expression->e0);
    auto tmp = createTemporary(type);
    auto save = statements;
    statements.clear();
    visit(expression->e1);
    auto path1 = addAssignment(expression->srcInfo, tmp, expression->e1);
    typeMap->setType(path1, type);
    auto ifTrue = statements;

    statements.clear();
    visit(expression->e2);
    auto path2 = addAssignment(expression->srcInfo, tmp, expression->e2);
    auto ifFalse = statements;
    statements = save;

    auto ifStatement = new IR::IfStatement(expression->e0, new IR::BlockStatement(ifTrue),
                                           new IR::BlockStatement(ifFalse));
    statements.push_back(ifStatement);
    typeMap->setType(path2, type);
    prune();
    return path2;
}

cstring DismantleMuxExpressions::createTemporary(const IR::Type *type) {
    type = type->getP4Type();
    auto tmp = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(tmp, nullptr), type);
    toInsert.push_back(decl);
    return tmp;
}

const IR::Expression *DismantleMuxExpressions::addAssignment(Util::SourceInfo srcInfo,
                                                             cstring varName,
                                                             const IR::Expression *expression) {
    const IR::PathExpression *left;
    if (auto pe = expression->to<IR::PathExpression>())
        left = new IR::PathExpression(IR::ID(pe->srcInfo, varName, pe->path->name.originalName));
    else
        left = new IR::PathExpression(IR::ID(varName));
    auto stat = new IR::AssignmentStatement(srcInfo, left, expression);
    statements.push_back(stat);
    auto result = left->clone();
    return result;
}

const IR::Node *DismantleMuxExpressions::postorder(IR::P4Action *action) {
    if (toInsert.empty()) return action;
    auto body = new IR::BlockStatement(action->body->srcInfo);
    for (auto a : toInsert) body->push_back(a);
    for (auto s : action->body->components) body->push_back(s);
    action->body = body;
    toInsert.clear();
    return action;
}

const IR::Node *DismantleMuxExpressions::postorder(IR::Function *function) {
    if (toInsert.empty()) return function;
    auto body = new IR::BlockStatement(function->body->srcInfo);
    for (auto a : toInsert) body->push_back(a);
    for (auto s : function->body->components) body->push_back(s);
    function->body = body;
    toInsert.clear();
    return function;
}

const IR::Node *DismantleMuxExpressions::postorder(IR::P4Parser *parser) {
    if (toInsert.empty()) return parser;
    parser->parserLocals.append(toInsert);
    toInsert.clear();
    return parser;
}

const IR::Node *DismantleMuxExpressions::postorder(IR::P4Control *control) {
    if (toInsert.empty()) return control;
    control->controlLocals.append(toInsert);
    toInsert.clear();
    return control;
}

const IR::Node *DismantleMuxExpressions::postorder(IR::AssignmentStatement *statement) {
    if (statements.empty()) return statement;
    statements.push_back(statement);
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

/* This function transforms the table so that all match keys come from the same struct and satisfy
   the constraints imposed by the target.
   Mirror copies of match fields are created in metadata struct and table is updated to
   use the metadata fields.
   The decision of whether a mirror copy is to be created is based on the following conditions:
   1) If all the table key fields are part of the same structure (either header or metadata),
      copy of fields to metadata should be done when
      a) all keys have exact matchkind and are not contiguous in the underlying structure.
   2) When the table keys have a mix of header and existing metadata fields(A)
      a) copy of header fields should always be done
      b) copy of metadata fields(A) should be done when all keys have exact matchkind and the
         number of (A) keys are less than 5. These heuristics are chosen to obtain optimal
         performance.
   3) For learner table, the match kind of all keys should always be exact and hence if the keys
      are non-exact, error should be thrown and if the keys are non-contiguous, copy of the key
      fields should be done.


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
   Temporary variable holding table key copies are held in ProgramStructure key_fields.
       bit<48> tbl_ethernet_srcAddr;  // These declarations are later copied to metadata struct
       bit<16> tbl_ipv4_totalLen;     // in CollectLocalVariables pass.

   control ingress(inout headers h, inout metadata m) {
       ...
       table tbl {
           key = {
               m.ingress_tbl_ethernet_srcAddr: lpm;
               m.ingress_tbl_ipv4_totalLen   : exact;
           }
           ...
       }
       apply {
           m.ingress_tbl_ethernet_srcAddr = h.ethernet.srcAddr;
           m.ingress_tbl_ipv4_totalLen = h.ipv4.totalLen;
           tbl_0.apply();
       }
  }
*/

bool CopyMatchKeysToSingleStruct::isLearnerTable(const IR::P4Table *t) {
    bool use_add_on_miss = false;
    auto add_on_miss = t->properties->getProperty("add_on_miss");
    if (add_on_miss == nullptr) return false;
    if (add_on_miss->value->is<IR::ExpressionValue>()) {
        auto expr = add_on_miss->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error(ErrorType::ERR_UNEXPECTED, "%1%: expected boolean for 'add_on_miss' property",
                    add_on_miss);
            return false;
        } else {
            use_add_on_miss = expr->to<IR::BoolLiteral>()->value;
        }
    }
    return use_add_on_miss;
}

int CopyMatchKeysToSingleStruct::getFieldSizeBits(const IR::Type *field_type) {
    if (auto t = field_type->to<IR::Type_Bits>()) {
        return t->width_bits();
    } else if (field_type->is<IR::Type_Boolean>() || field_type->is<IR::Type_Error>()) {
        return 8;
    } else if (auto t = field_type->to<IR::Type_Name>()) {
        if (t->path->name == "error") {
            return 8;
        } else {
            return -1;
        }
    }
    return -1;
}

struct keyInfo *CopyMatchKeysToSingleStruct::getKeyInfo(IR::Key *keys) {
    auto oneKey = new struct keyInfo();
    oneKey->numElements = keys->keyElements.size();
    int size = 0;
    int numExistingMetaFields = 0;
    int nonExact = 0;
    for (auto key : keys->keyElements) {
        auto matchKind = key->matchType->toString();
        if (matchKind != "exact") {
            nonExact++;
        }

        auto keyName = key->expression->toString();
        if (keyName.startsWith("m.")) numExistingMetaFields++;

        auto elem = new struct keyElementInfo();
        elem->offsetInMetadata = -1;
        if (key->expression->is<IR::Member>()) {
            auto keyMem = key->expression->to<IR::Member>();
            auto type = keyMem->expr->type;

            if (auto baseStruct = type->to<IR::Type_StructLike>()) {
                elem->offsetInMetadata = baseStruct->getFieldBitOffset(keyMem->member.name);
            }
        }
        auto field_type = key->expression->type;
        elem->size = getFieldSizeBits(field_type);
        if (elem->size == -1) {
            BUG("Unexpected type %1%", field_type->node_type_name());
            return nullptr;
        }
        // These are complex expressions
        if (elem->offsetInMetadata == -1) numExistingMetaFields++;
        size += elem->size;
        oneKey->elements.push_back(elem);
    }

    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(table);
    oneKey->isLearner = isLearnerTable(table);
    oneKey->size = size;
    oneKey->numExistingMetaFields = numExistingMetaFields;
    oneKey->isExact = nonExact ? false : true;
    return oneKey;
}

const IR::Node *CopyMatchKeysToSingleStruct::preorder(IR::Key *keys) {
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
        if (firstKeyStr.startsWith("h")) firstKeyHdr = true;
    }

    /* Key fields should be part of same header/metadata struct */
    for (auto key : keys->keyElements) {
        cstring keyTypeStr = "";
        if (auto keyField = key->expression->to<IR::Member>()) {
            keyTypeStr = keyField->expr->toString();
        } else if (auto m = key->expression->to<IR::MethodCallExpression>()) {
            /* When isValid is present as table key, it should be moved to metadata */
            auto mi = P4::MethodInstance::resolve(m, refMap, typeMap);
            if (auto b = mi->to<P4::BuiltInMethod>()) {
                if (b->name == "isValid") {
                    copyNeeded = true;
                    break;
                }
            }
        }
        if (firstKeyStr != keyTypeStr) {
            if (firstKeyHdr || keyTypeStr.startsWith("h")) {
                copyNeeded = true;
                break;
            }
        }
    }

    auto keyInfoInstance = getKeyInfo(keys);
    BUG_CHECK(keyInfoInstance, "Failed to collect key information");
    bool contiguous = true;

    for (int i = 0; i < keyInfoInstance->numElements; i++) {
        for (int j = 0; j < keyInfoInstance->numElements; j++) {
            auto keyI = keyInfoInstance->elements.at(i);
            auto keyJ = keyInfoInstance->elements.at(j);
            int startI = keyI->offsetInMetadata;
            int endI = startI + keyI->size;
            int startJ = keyJ->offsetInMetadata;
            int endJ = startJ + keyJ->size;
            if (endJ - startI > keyInfoInstance->size) {
                contiguous = false;
                break;
            } else if (endI - startJ > keyInfoInstance->size) {
                contiguous = false;
                break;
            }
        }
        if (!contiguous) break;
    }

    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(table);

    if (keyInfoInstance->isLearner) {
        if (!keyInfoInstance->isExact) {
            ::error(ErrorType::ERR_EXPECTED, "Learner table %1% must have all exact match keys",
                    table->name);
            return keys;
        }
        structure->table_type_map.emplace(table->name.name, InternalTableType::LEARNER);
    } else if (contiguous && keyInfoInstance->isExact) {
        structure->table_type_map.emplace(table->name.name, InternalTableType::REGULAR_EXACT);
    } else {
        structure->table_type_map.emplace(table->name.name, InternalTableType::WILDCARD);
    }

    /* If copyNeeded is false at this point, it means the keys are from same struct.
     * Check remaining conditions to see if the copy is needed or not */
    metaCopyNeeded = false;
    if (copyNeeded) contiguous = false;

    if (!contiguous &&
        ((keyInfoInstance->isLearner) ||
         (keyInfoInstance->isExact && keyInfoInstance->numExistingMetaFields <= 5))) {
        metaCopyNeeded = true;
        copyNeeded = true;
    }

    if (!copyNeeded) {
        // This prune will prevent the postorder(IR::KeyElement*) below from executing
        prune();
    } else {
        ::warning(ErrorType::WARN_MISMATCH,
                  "Mismatched header/metadata struct for key "
                  "elements in table %1%. Copying all match fields to metadata",
                  findOrigCtxt<IR::P4Table>()->name.toString());
        LOG3("Will pull out " << keys);
    }
    return keys;
}

const IR::Node *CopyMatchKeysToSingleStruct::postorder(IR::KeyElement *element) {
    // If we got here we need to put the key element in metadata.
    LOG3("Extracting key element " << element);
    auto table = findOrigCtxt<IR::P4Table>();
    auto control = findOrigCtxt<IR::P4Control>();
    CHECK_NULL(table);
    P4::TableInsertions *insertions;
    auto it = toInsert.find(table);
    if (it == toInsert.end()) {
        insertions = new P4::TableInsertions();
        toInsert.emplace(table, insertions);
    } else {
        insertions = it->second;
    }

    auto keyName = element->expression->toString();
    bool isHeader = false;
    /* All header fields are prefixed with "h." and metadata fields are prefixed with "m."
     * Prefix the match field with control and table name */
    if (keyName.startsWith("h.")) {
        isHeader = true;
        keyName = keyName.replace('.', '_');
        keyName =
            keyName.replace("h_", control->name.toString() + "_" + table->name.toString() + "_");
    } else if (metaCopyNeeded) {
        if (keyName.startsWith("m.")) {
            keyName = keyName.replace('.', '_');
            keyName = keyName.replace(
                "m_", control->name.toString() + "_" + table->name.toString() + "_");
        } else {
            keyName = control->name.toString() + "_" + table->name.toString() + "_" + keyName;
        }
    }

    if (isHeader || metaCopyNeeded) {
        IR::ID keyNameId(refMap->newName(keyName));
        auto decl = new IR::Declaration_Variable(keyNameId, element->expression->type, nullptr);
        // Store the compiler generated table keys in Program structure. These will be
        // inserted to Metadata by CollectLocalVariables pass.
        structure->key_fields.push_back(new IR::StructField(decl->name.name, decl->type));
        auto right = element->expression;
        auto left = new IR::Member(new IR::PathExpression(IR::ID("m")), keyNameId);
        auto assign = new IR::AssignmentStatement(element->expression->srcInfo, left, right);
        insertions->statements.push_back(assign);
        element->expression = left;
    }
    return element;
}

const IR::Node *CopyMatchKeysToSingleStruct::doStatement(const IR::Statement *statement,
                                                         const IR::Expression *expression) {
    LOG3("Visiting " << getOriginal());
    P4::HasTableApply hta(refMap, typeMap);
    hta.setCalledBy(this);
    (void)expression->apply(hta);
    if (hta.table == nullptr) return statement;
    auto insertions = get(toInsert, hta.table);
    if (insertions == nullptr) return statement;
    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto assign : insertions->statements) result->push_back(assign);
    result->push_back(statement);
    auto block = new IR::BlockStatement(*result);
    return block;
}

namespace Helpers {

std::optional<P4::ExternInstance> getExternInstanceFromProperty(
    const IR::P4Table *table, const cstring &propertyName, P4::ReferenceMap *refMap,
    P4::TypeMap *typeMap, bool *isConstructedInPlace, cstring &externName) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return std::nullopt;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected %1% property value for table %2% to be an expression: %3%", propertyName,
                table->controlPlaneName(), property);
        return std::nullopt;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    externName = expr->to<IR::PathExpression>()->path->name.name;
    if (isConstructedInPlace) *isConstructedInPlace = expr->is<IR::ConstructorCallExpression>();
    if (expr->is<IR::ConstructorCallExpression>() &&
        property->getAnnotation(IR::Annotation::nameAnnotation) == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "Table '%1%' has an anonymous table property '%2%' with no name annotation, "
                "which is not supported by P4Runtime",
                table->controlPlaneName(), propertyName);
        return std::nullopt;
    }
    auto name = property->controlPlaneName();
    auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap, name);
    if (!externInstance) {
        ::error(ErrorType::ERR_INVALID,
                "Expected %1% property value for table %2% to resolve to an "
                "extern instance: %3%",
                propertyName, table->controlPlaneName(), property);
        return std::nullopt;
    }
    return externInstance;
}

}  // namespace Helpers

// create P4Table object that represents the matching part of the original P4
// table. This table sets the internal group_id or member_id which are used
// for subsequent table lookup.
std::tuple<const IR::P4Table *, cstring, cstring> SplitP4TableCommon::create_match_table(
    const IR::P4Table *tbl) {
    cstring grpActionName = "", memActionName;
    if (implementation == TableImplementation::ACTION_SELECTOR) {
        grpActionName = refMap->newName(tbl->name.originalName + "_set_group_id");
        memActionName = refMap->newName(tbl->name.originalName + "_set_member_id");
    } else if (implementation == TableImplementation::ACTION_PROFILE) {
        memActionName = refMap->newName(tbl->name.originalName + "_set_member_id");
    } else {
        BUG("Unexpected table implementation type");
    }
    IR::Vector<IR::KeyElement> match_keys;
    for (auto key : tbl->getKey()->keyElements) {
        if (key->matchType->toString() != "selector") {
            match_keys.push_back(key);
        }
    }
    IR::IndexedVector<IR::ActionListElement> actionsList;

    if (implementation == TableImplementation::ACTION_SELECTOR) {
        auto grpActionCall = new IR::MethodCallExpression(new IR::PathExpression(grpActionName));
        actionsList.push_back(new IR::ActionListElement(grpActionCall));
    }
    auto memActionCall = new IR::MethodCallExpression(new IR::PathExpression(memActionName));
    actionsList.push_back(new IR::ActionListElement(memActionCall));
    auto default_action = tbl->getDefaultAction();
    if (default_action) {
        if (auto mc = default_action->to<IR::MethodCallExpression>()) {
            // Ignore action params of default action by creating new MethodCallExpression
            auto defAction = new IR::MethodCallExpression(mc->method->to<IR::PathExpression>());
            actionsList.push_back(new IR::ActionListElement(defAction));
        }
    }

    auto constDefAction = tbl->properties->getProperty("default_action");
    bool isConstDefAction = constDefAction ? constDefAction->isConstant : false;

    IR::IndexedVector<IR::Property> properties;
    properties.push_back(new IR::Property("actions", new IR::ActionList(actionsList), false));
    properties.push_back(new IR::Property("key", new IR::Key(match_keys), false));
    properties.push_back(new IR::Property(
        "default_action", new IR::ExpressionValue(tbl->getDefaultAction()), isConstDefAction));
    if (tbl->getSizeProperty()) {
        properties.push_back(
            new IR::Property("size", new IR::ExpressionValue(tbl->getSizeProperty()), false));
    }
    auto match_table =
        new IR::P4Table(tbl->name, tbl->annotations, new IR::TableProperties(properties));
    return std::make_tuple(match_table, grpActionName, memActionName);
}

const IR::P4Action *SplitP4TableCommon::create_action(cstring actionName, cstring group_id,
                                                      cstring param) {
    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    auto set_id = new IR::AssignmentStatement(new IR::PathExpression(group_id),
                                              new IR::PathExpression(param));
    auto parameter = new IR::Parameter(param, IR::Direction::None, IR::Type_Bits::get(32));
    auto action = new IR::P4Action(actionName, hidden, new IR::ParameterList({parameter}),
                                   new IR::BlockStatement({set_id}));
    return action;
}

const IR::P4Table *SplitP4TableCommon::create_member_table(const IR::P4Table *tbl,
                                                           cstring memberTableName,
                                                           cstring member_id) {
    IR::Vector<IR::KeyElement> member_keys;
    auto tableKeyEl =
        new IR::KeyElement(new IR::PathExpression(member_id),
                           new IR::PathExpression(P4::P4CoreLibrary::instance().exactMatch.Id()));
    member_keys.push_back(tableKeyEl);
    IR::IndexedVector<IR::Property> member_properties;
    member_properties.push_back(new IR::Property("key", new IR::Key(member_keys), false));

    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    auto nameAnnon = tbl->getAnnotation(IR::Annotation::nameAnnotation);
    cstring nameA = nameAnnon->getSingleString();
    cstring memName = nameA.replace(nameA.findlast('.'), "." + memberTableName);
    hidden->addAnnotation(IR::Annotation::nameAnnotation, new IR::StringLiteral(memName), false);

    IR::IndexedVector<IR::ActionListElement> memberActionList;
    for (auto action : tbl->getActionList()->actionList) memberActionList.push_back(action);
    member_properties.push_back(
        new IR::Property("actions", new IR::ActionList(memberActionList), false));
    if (tbl->getSizeProperty()) {
        member_properties.push_back(
            new IR::Property("size", new IR::ExpressionValue(tbl->getSizeProperty()), false));
    }
    member_properties.push_back(new IR::Property(
        "default_action", new IR::ExpressionValue(tbl->getDefaultAction()), false));

    auto member_table =
        new IR::P4Table(memberTableName, hidden, new IR::TableProperties(member_properties));

    return member_table;
}

const IR::P4Table *SplitP4TableCommon::create_group_table(const IR::P4Table *tbl,
                                                          cstring selectorTableName,
                                                          cstring group_id, cstring member_id,
                                                          unsigned n_groups_max,
                                                          unsigned n_members_per_group_max) {
    IR::Vector<IR::KeyElement> selector_keys;
    for (auto key : tbl->getKey()->keyElements) {
        if (key->matchType->toString() == "selector") {
            selector_keys.push_back(key);
        }
    }
    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    auto nameAnnon = tbl->getAnnotation(IR::Annotation::nameAnnotation);
    cstring nameA = nameAnnon->getSingleString();
    cstring selName = nameA.replace(nameA.findlast('.'), "." + selectorTableName);
    hidden->addAnnotation(IR::Annotation::nameAnnotation, new IR::StringLiteral(selName), false);
    IR::IndexedVector<IR::Property> selector_properties;
    selector_properties.push_back(new IR::Property("selector", new IR::Key(selector_keys), false));
    selector_properties.push_back(new IR::Property(
        "group_id", new IR::ExpressionValue(new IR::PathExpression(group_id)), false));
    selector_properties.push_back(new IR::Property(
        "member_id", new IR::ExpressionValue(new IR::PathExpression(member_id)), false));
    selector_properties.push_back(new IR::Property(
        "n_groups_max",
        new IR::ExpressionValue(new IR::Constant(IR::Type_Bits::get(32), n_groups_max)), false));
    selector_properties.push_back(new IR::Property(
        "n_members_per_group_max",
        new IR::ExpressionValue(new IR::Constant(IR::Type_Bits::get(32), n_members_per_group_max)),
        false));
    selector_properties.push_back(new IR::Property("actions", new IR::ActionList({}), false));
    auto group_table =
        new IR::P4Table(selectorTableName, hidden, new IR::TableProperties(selector_properties));
    return group_table;
}

const IR::Node *SplitActionSelectorTable::postorder(IR::P4Table *tbl) {
    bool isConstructedInPlace = false;
    bool isAsInstanceShared = false;
    cstring externName = "";
    cstring prefix = "psa_";

    if (structure->isPNA()) {
        prefix = "pna_";
    }

    auto property = tbl->properties->getProperty(prefix + "implementation");
    auto counterProperty = tbl->properties->getProperty(prefix + "direct_counter");
    auto meterProperty = tbl->properties->getProperty(prefix + "direct_meter");

    if (property != nullptr && (counterProperty != nullptr || meterProperty != nullptr)) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "implementation property cannot co-exist with direct counter and direct meter "
                "property for table %1%",
                tbl->name);
        return tbl;
    }

    auto instance = Helpers::getExternInstanceFromProperty(
        tbl, prefix + "implementation", refMap, typeMap, &isConstructedInPlace, externName);
    if (!instance) return tbl;
    if (instance->type->name != "ActionSelector") return tbl;

    if (instance->arguments->size() != 3) {
        ::error(ErrorType::ERR_UNEXPECTED, "Incorrect number of argument on action selector %1%",
                *instance->name);
        return tbl;
    }

    if (!instance->arguments->at(1)->expression->is<IR::Constant>()) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "The 'size' argument of ActionSelector %1% must be a constant", *instance->name);
        return tbl;
    }

    int n_groups_max = instance->arguments->at(1)->expression->to<IR::Constant>()->asUnsigned();
    if (!instance->arguments->at(2)->expression->is<IR::Constant>()) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "The 'outputWidth' argument of ActionSelector %1% must be a constant",
                *instance->name);
        return tbl;
    }
    auto outputWidth = instance->arguments->at(2)->expression->to<IR::Constant>()->asUnsigned();
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
        if (mid.second == member_id) isAsInstanceShared = true;
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
    cstring grpActionName, memActionName;
    const IR::P4Table *match_table;
    std::tie(match_table, grpActionName, memActionName) = create_match_table(tbl);
    auto grpAction = create_action(grpActionName, group_id, "group_id");
    auto memAction = create_action(memActionName, member_id, "member_id");
    decls->push_back(grpAction);
    decls->push_back(memAction);
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

const IR::Node *SplitActionProfileTable::postorder(IR::P4Table *tbl) {
    bool isConstructedInPlace = false;
    bool isApInstanceShared = false;
    cstring externName = "";
    cstring implementation = "psa_implementation";

    if (structure->isPNA()) implementation = "pna_implementation";

    auto instance = Helpers::getExternInstanceFromProperty(tbl, implementation, refMap, typeMap,
                                                           &isConstructedInPlace, externName);

    if (!instance || instance->type->name != "ActionProfile") return tbl;

    if (instance->arguments->size() != 1) {
        ::error(ErrorType::ERR_MODEL, "Incorrect number of argument on action profile %1%",
                *instance->name);
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
        if (mid.second == member_id) isApInstanceShared = true;
    }

    member_ids.emplace(tbl->name, member_id);

    if (!isApInstanceShared) {
        auto member_id_decl = new IR::Declaration_Variable(member_id, IR::Type_Bits::get(32));
        decls->push_back(member_id_decl);
    }

    cstring actionName, ignoreGroup;
    const IR::P4Table *match_table;
    std::tie(match_table, ignoreGroup, actionName) = create_match_table(tbl);
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

// Member_id and/or group_id must be initialized prior to member table apply.
// An action selector table can either set a group_id which in turns set the member_id or it
// can directly set a member_id. Member table is always applied in either case.
// Hence, we initialize member_id with 0 as it will always have a valid value based on the base
// table match Since the values of member_id and group_id are set during run-time, to make a
// decision whether to apply the group table or not, we initialize group_id with the maximum
// possible value (this is 32-bit as per PSA specification) and compare this initial value with the
// group_id at run-time.
IR::Expression *SplitP4TableCommon::initializeMemberAndGroupId(
    cstring tableName, IR::IndexedVector<IR::StatOrDecl> *decls) {
    IR::Expression *group_id_expr = nullptr;
    if (member_ids.count(tableName) != 0) {
        auto member_id = member_ids.at(tableName);
        decls->push_back(new IR::AssignmentStatement(
            new IR::PathExpression(member_id),
            new IR::Constant(IR::Type_Bits::get(32), initial_member_id)));
    }
    if (group_ids.count(tableName) != 0) {
        auto group_id = group_ids.at(tableName);
        group_id_expr = new IR::PathExpression(group_id);
        decls->push_back(new IR::AssignmentStatement(
            group_id_expr, new IR::Constant(IR::Type_Bits::get(32), initial_group_id)));
    }
    return group_id_expr;
}

const IR::Node *SplitP4TableCommon::postorder(IR::MethodCallStatement *statement) {
    auto methodCall = statement->methodCall;
    auto mi = P4::MethodInstance::resolve(methodCall, refMap, typeMap);
    auto gen_apply = [](cstring table) {
        IR::Statement *expr = new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(table), IR::ID(IR::IApply::applyMethodName))));
        return expr;
    };

    auto apply_hit = [](cstring table) {
        IR::Expression *expr =
            new IR::Member(new IR::MethodCallExpression(new IR::Member(
                               new IR::PathExpression(table), IR::ID(IR::IApply::applyMethodName))),
                           "hit");
        return expr;
    };

    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply()) return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (match_tables.count(table->name) == 0) return statement;
        IR::Expression *group_id_expr = nullptr;
        auto decls = new IR::IndexedVector<IR::StatOrDecl>();
        auto tableName = apply->object->getName().name;
        group_id_expr = initializeMemberAndGroupId(tableName, decls);
        if (member_tables.count(tableName) == 0) {
            ::error(ErrorType::ERR_NOT_FOUND, "Unable to find member table %1%", tableName);
            return statement;
        }
        // an action selector t.apply() is converted to
        // if (t0.apply().hit) {  // base table
        //      if (group_id != initial_groupid)
        //          t1.apply();  // group table
        //      t2.apply();  // member table
        // }
        auto ifBaseTableHit = new IR::IndexedVector<IR::StatOrDecl>();
        auto memberTable = member_tables.at(tableName);
        auto t2stat = gen_apply(memberTable);
        auto cond = apply_hit(tableName);
        IR::Statement *t1stat = nullptr;
        IR::Expression *t0stat = nullptr;
        if (implementation == TableImplementation::ACTION_SELECTOR) {
            if (group_tables.count(tableName) == 0) {
                ::error(ErrorType::ERR_NOT_FOUND, "Unable to find group table %1%", tableName);
                return statement;
            }
            auto selectorTable = group_tables.at(tableName);
            BUG_CHECK(group_id_expr, "initial group id is not set");
            auto max_group_cst = new IR::Constant(IR::Type_Bits::get(32), initial_group_id);
            t0stat = new IR::Neq(statement->srcInfo, group_id_expr, max_group_cst);
            t1stat = gen_apply(selectorTable);
        }

        IR::IfStatement *ret = nullptr;
        if (implementation == TableImplementation::ACTION_SELECTOR) {
            ifBaseTableHit->push_back(new IR::IfStatement(t0stat, t1stat, nullptr));
            ifBaseTableHit->push_back(t2stat);
            ret = new IR::IfStatement(cond, new IR::BlockStatement(*ifBaseTableHit), nullptr);
        } else if (implementation == TableImplementation::ACTION_PROFILE) {
            ret = new IR::IfStatement(cond, t2stat, nullptr);
        }
        decls->push_back(ret);
        return new IR::BlockStatement(*decls);
    }
    return statement;
}

// assume the RemoveMiss and SimplifyControlFlow pass is applied
const IR::Node *SplitP4TableCommon::postorder(IR::IfStatement *statement) {
    auto cond = statement->condition;
    if (!P4::TableApplySolver::isHit(cond, refMap, typeMap)) return statement;
    if (!cond->is<IR::Member>()) return statement;
    auto member = cond->to<IR::Member>();
    if (!member->expr->is<IR::MethodCallExpression>()) return statement;
    auto mce = member->expr->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);

    auto apply_hit = [](cstring table) {
        IR::Expression *expr =
            new IR::Member(new IR::MethodCallExpression(new IR::Member(
                               new IR::PathExpression(table), IR::ID(IR::IApply::applyMethodName))),
                           "hit");
        return expr;
    };

    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply()) return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (match_tables.count(table->name) == 0) return statement;
        //
        // if (t.apply().hit) {
        //   foo();
        // }
        // is equivalent to
        // if (t0.apply().hit) {
        //    if (group_id != initial_groupid) {
        //       if (t1.apply().hit) {
        //           if (t2.apply().hit) {
        //              foo();
        //           }
        //       }
        //    } else if (as.apply().hit) {
        //        foo();
        //    }
        // }
        IR::Expression *group_id_expr = nullptr;
        auto decls = new IR::IndexedVector<IR::StatOrDecl>();
        auto tableName = apply->object->getName().name;
        group_id_expr = initializeMemberAndGroupId(tableName, decls);
        if (member_tables.count(tableName) == 0) {
            ::error(ErrorType::ERR_NOT_FOUND, "Unable to find member table %1%", tableName);
            return statement;
        }
        if (member_tables.count(tableName) == 0) return statement;
        auto memberTable = member_tables.at(tableName);
        IR::Expression *t0stat = nullptr;
        IR::Expression *t1stat = nullptr;
        if (implementation == TableImplementation::ACTION_SELECTOR) {
            auto selectorTable = group_tables.at(tableName);
            BUG_CHECK(group_id_expr, "initial group id is not set");
            auto max_group_cst = new IR::Constant(IR::Type_Bits::get(32), initial_group_id);
            t0stat = new IR::Neq(statement->srcInfo, group_id_expr, max_group_cst);
            t1stat = apply_hit(selectorTable);
        }

        IR::IfStatement *ret = nullptr;
        auto t2stat =
            new IR::IfStatement(apply_hit(memberTable), statement->ifTrue, statement->ifFalse);

        if (implementation == TableImplementation::ACTION_SELECTOR) {
            ret = new IR::IfStatement(
                cond,
                new IR::IfStatement(t0stat, new IR::IfStatement(t1stat, t2stat, statement->ifFalse),
                                    t2stat),
                statement->ifFalse);
        } else if (implementation == TableImplementation::ACTION_PROFILE) {
            ret = new IR::IfStatement(cond, t2stat, statement->ifFalse);
        }
        decls->push_back(ret);
        return new IR::BlockStatement(*decls);
    }
    return statement;
}

/* For tables using action selector/profile property, the group table and member table should
 * only be applied if the base table is a hit. Hence, we split the switch(tbl.apply().action_run)
 * into an if statement to see if all three tables should be applied or not  and a switch statement
 * which handles the side effect of table.apply(). We assign a number to each action the table can
 * invoke. Each action which can be invoked from the base table updates a common variable with its
 * action number. This common variable is used as switch expression.
 *
 * action a0{...}
 * action a1{...}
 *
 * switch(t.apply().action_run) {
 *    a0: { bar_0.apply();}
 *    a1: { foo_0.apply();}
 * }
 *
 * is converted to
 * action a0{
 *     switchExprTmp = 0;
 *     ...
 * }
 * action a1{
 *     switchExprTmp = 1;
 *     ...
 * }
 *
 * switchExprTmp = 0xFFFFFFFF;
 * if (t0.apply().hit) {
 *    if (group_id != initial_group_id) {
 *        t1.apply();
 *    }
 *    t2.apply();
 * }
 *
 * Action invoked on above table apply will set the switch expression variable
 * switch (switchExprTmp) {
 *    0: { bar_0.apply();}
 *    1: { foo_0.apply();}
 * }
 *
 */

const IR::Node *SplitP4TableCommon::postorder(IR::SwitchStatement *statement) {
    auto expr = statement->expression;
    auto member = expr->to<IR::Member>();
    if (!member || member->member != "action_run") return statement;
    if (!member->expr->is<IR::MethodCallExpression>()) return statement;
    auto mce = member->expr->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    auto gen_apply = [](cstring table) {
        IR::Statement *expr = new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(table), IR::ID(IR::IApply::applyMethodName))));
        return expr;
    };
    auto apply_hit = [](cstring table) {
        IR::Expression *expr =
            new IR::Member(new IR::MethodCallExpression(new IR::Member(
                               new IR::PathExpression(table), IR::ID(IR::IApply::applyMethodName))),
                           "hit");
        return expr;
    };

    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply()) return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (match_tables.count(table->name) == 0) return statement;
        IR::Expression *group_id_expr = nullptr;
        auto decls = new IR::IndexedVector<IR::StatOrDecl>();
        auto tableName = apply->object->getName().name;

        group_id_expr = initializeMemberAndGroupId(tableName, decls);
        if (member_tables.count(tableName) == 0) {
            ::error(ErrorType::ERR_NOT_FOUND, "Unable to find member table %1%", tableName);
            return statement;
        }
        auto ifBaseTableHit = new IR::IndexedVector<IR::StatOrDecl>();
        auto memberTable = member_tables.at(tableName);
        auto t2stat = gen_apply(memberTable);
        auto cond = apply_hit(tableName);
        IR::Statement *t1stat = nullptr;
        IR::Expression *t0stat = nullptr;

        if (implementation == TableImplementation::ACTION_SELECTOR) {
            if (group_tables.count(tableName) == 0) {
                ::error(ErrorType::ERR_NOT_FOUND, "Unable to find group table %1%", tableName);
                return statement;
            }
            auto selectorTable = group_tables.at(tableName);
            BUG_CHECK(group_id_expr, "initial group id is not set");
            auto max_group_cst = new IR::Constant(IR::Type_Bits::get(32), initial_group_id);
            t0stat = new IR::Neq(statement->srcInfo, group_id_expr, max_group_cst);
            t1stat = gen_apply(selectorTable);
        }

        IR::IfStatement *ret = nullptr;
        if (implementation == TableImplementation::ACTION_SELECTOR) {
            ifBaseTableHit->push_back(new IR::IfStatement(t0stat, t1stat, nullptr));
            ifBaseTableHit->push_back(t2stat);
            ret = new IR::IfStatement(cond, new IR::BlockStatement(*ifBaseTableHit), nullptr);
        } else if (implementation == TableImplementation::ACTION_PROFILE) {
            ret = new IR::IfStatement(cond, t2stat, nullptr);
        }

        // Generate new switch statement with a temporary variable as switch expression
        // and insert this temporary variable declaration into the enclosing control block
        auto control = findOrigCtxt<IR::P4Control>();
        switchExprTmp = refMap->newName("switchExprTmp");
        auto decl = new IR::Declaration_Variable(IR::ID(switchExprTmp), IR::Type_Bits::get(32));
        injector.collect(control, nullptr, decl);
        decls->push_back(new IR::AssignmentStatement(
            statement->srcInfo, new IR::PathExpression(IR::ID(switchExprTmp)),
            new IR::Constant(IR::Type_Bits::get(32), 0xFFFFFFFF)));
        decls->push_back(ret);

        unsigned label_value = 0;
        IR::Vector<IR::SwitchCase> cases;
        for (auto c : statement->cases) {
            if (c->label->is<IR::DefaultExpression>()) {
                cases.push_back(c);
            } else if (auto pe = c->label->to<IR::PathExpression>()) {
                auto caseLabelValue = new IR::Constant(IR::Type_Bits::get(32), label_value);
                auto currCase = new IR::SwitchCase(caseLabelValue, c->statement);
                cases.push_back(currCase);
                cstring label = pe->path->name.name;
                sw.addToSwitchMap(label, switchExprTmp, caseLabelValue);
                label_value++;
            } else {
                BUG("Unexpected case label %1%, expected action name or default", c->label);
            }
        }
        decls->push_back(new IR::SwitchStatement(
            statement->srcInfo, new IR::PathExpression(IR::ID(switchExprTmp)), cases));

        return new IR::BlockStatement(*decls);
    }
    return statement;
}

const IR::Node *SplitP4TableCommon::postorder(IR::P4Control *a) {
    auto control = getOriginal();
    return injector.inject_control(control, a);
}

/* For regular tables, the direct counter array size is same as table size
 * For learner tables, the direct counter/meter array size is 4 times the table size */
int CollectDirectCounterMeter::getTableSize(const IR::P4Table *tbl) {
    int tableSize = dpdk_default_table_size;
    auto size = tbl->getSizeProperty();
    if (size) tableSize = size->asUnsigned();
    return tableSize;
}

void CollectDirectCounterMeter::checkMethodCallInAction(const P4::ExternMethod *a) {
    cstring externName = a->originalExternType->getName().name;
    if (externName == "DirectMeter" || externName == "DirectCounter") {
        auto di = a->object->to<IR::Declaration_Instance>();
        cstring instanceName = di->name.name;
        if (a->method->getName().name == method) {
            if (instancename != "" && instanceName == instancename) {
                methodCallFound = true;
            }

            if (oneInstance == "") oneInstance = instanceName;

            // error if more than one count method found with different instance name
            if (oneInstance != instanceName) {
                ::error(ErrorType::ERR_UNEXPECTED,
                        "%1% method for different %2% "
                        "instances (%3% and %4%) called within same action",
                        method, externName, oneInstance, instanceName);
                return;
            }
        }
    }
}

bool CollectDirectCounterMeter::preorder(const IR::MethodCallStatement *mcs) {
    auto mi = P4::MethodInstance::resolve(mcs->methodCall, refMap, typeMap);
    if (auto a = mi->to<P4::ExternMethod>()) {
        checkMethodCallInAction(a);
    }
    return false;
}

bool CollectDirectCounterMeter::preorder(const IR::AssignmentStatement *assn) {
    if (auto m = assn->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(m, refMap, typeMap);
        if (auto a = mi->to<P4::ExternMethod>()) {
            checkMethodCallInAction(a);
        }
    }
    return false;
}

bool CollectDirectCounterMeter::ifMethodFound(const IR::P4Action *a, cstring methodName,
                                              cstring instance) {
    oneInstance = "";
    instancename = instance;
    method = methodName;
    methodCallFound = false;
    visit(a->body->components);
    return methodCallFound;
}

/* ifMethodFound() is called from here to make sure that an action only contains count/dpdk_execute
 * method calls for only one Direct counter/meter instance. The error for the same is emitted
 * in the ifMethodFound function itself and return value is not required to be checked here.
 */
bool CollectDirectCounterMeter::preorder(const IR::P4Action *a) {
    ifMethodFound(a, "count");
    ifMethodFound(a, "dpdk_execute");
    return false;
}

bool CollectDirectCounterMeter::preorder(const IR::P4Table *tbl) {
    bool isConstructedInPlace = false;
    cstring implementation = "psa_implementation";
    cstring counterExternName = "";
    cstring meterExternName = "";
    cstring direct_counter = "psa_direct_counter";
    cstring direct_meter = "psa_direct_meter";

    if (structure->isPNA()) {
        implementation = "pna_implementation";
        direct_counter = "pna_direct_counter";
        direct_meter = "pna_direct_meter";
    }

    auto counterInstance = Helpers::getExternInstanceFromProperty(
        tbl, direct_counter, refMap, typeMap, &isConstructedInPlace, counterExternName);
    auto meterInstance = Helpers::getExternInstanceFromProperty(
        tbl, direct_meter, refMap, typeMap, &isConstructedInPlace, meterExternName);
    int table_size = getTableSize(tbl);
    auto table_type = ::get(structure->table_type_map, tbl->name.name);

    // Direct Counter and Meter are not supported with Wildcard match tables
    if (table_type == InternalTableType::WILDCARD && (counterInstance || meterInstance)) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "Direct counters and direct meters are"
                " unsupported for wildcard match table %1%",
                tbl->name);
        return false;
    }

    if (table_type == InternalTableType::LEARNER) table_size *= 4;

    auto default_action = tbl->getDefaultAction();
    if (default_action) {
        if (auto mc = default_action->to<IR::MethodCallExpression>()) {
            default_action = mc->method;
        }

        auto path = default_action->to<IR::PathExpression>();
        BUG_CHECK(path, "Default action path %s cannot be found", default_action);
        if (auto defaultActionDecl = refMap->getDeclaration(path->path)->to<IR::P4Action>()) {
            if (defaultActionDecl->name.originalName != "NoAction") {
                if (!ifMethodFound(defaultActionDecl, "count", counterExternName)) {
                    if (counterInstance) {
                        ::error(ErrorType::ERR_EXPECTED,
                                "Expected default action %1% to have "
                                "'count' method call for DirectCounter extern instance %2%",
                                defaultActionDecl->name, *counterInstance->name);
                        return false;
                    }
                }
                if (!ifMethodFound(defaultActionDecl, "dpdk_execute", meterExternName)) {
                    if (meterInstance) {
                        ::error(ErrorType::ERR_EXPECTED,
                                "Expected default action %1% to have "
                                "'dpdk_execute' method call for DirectMeter extern instance %2%",
                                defaultActionDecl->name, *meterInstance->name);
                        return false;
                    }
                }
            }
        }
    }

    if (counterInstance && counterInstance->type->name == "DirectCounter") {
        directMeterCounterSizeMap.emplace(counterExternName, table_size + 1);
        structure->direct_resource_map.emplace(counterExternName, tbl);
    }
    if (meterInstance && meterInstance->type->name == "DirectMeter") {
        directMeterCounterSizeMap.emplace(meterExternName, table_size + 1);
        structure->direct_resource_map.emplace(meterExternName, tbl);
    }
    return false;
}

void ValidateDirectCounterMeter::validateMethodInvocation(P4::ExternMethod *a) {
    cstring externName = a->originalExternType->getName().name;
    cstring methodName = a->method->getName().name;
    if (externName != "DirectCounter" && externName != "DirectMeter") {
        return;
    }

    if ((externName == "DirectCounter" && methodName != "count") ||
        (externName == "DirectMeter" && methodName != "dpdk_execute")) {
        ::error(ErrorType::ERR_UNEXPECTED, "%1% method not supported for %2% extern", methodName,
                externName);
        return;
    }

    if (auto di = a->object->to<IR::Declaration_Instance>()) {
        auto ownerTable = ::get(structure->direct_resource_map, di->name.name);
        bool invokedFromOwnerTable = false;
        auto act = findOrigCtxt<IR::P4Action>();
        if (!act) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1% method of %2% extern "
                    "must only be called from within an action",
                    a->method->getName().name, di->name.originalName);
            return;
        }

        for (auto action : ownerTable->getActionList()->actionList) {
            auto action_decl = refMap->getDeclaration(action->getPath())->to<IR::P4Action>();
            if (act->name.name == action_decl->name.name) {
                invokedFromOwnerTable = true;
                break;
            }
        }

        if (!invokedFromOwnerTable) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1% method of %2% extern "
                    "can only be invoked from within action of ownertable",
                    a->method->getName(), di->name.originalName);
            return;
        }
    }
}

void ValidateDirectCounterMeter::postorder(const IR::AssignmentStatement *assn) {
    if (auto m = assn->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(m, refMap, typeMap);
        if (auto a = mi->to<P4::ExternMethod>()) {
            return validateMethodInvocation(a);
        }
    }
}

void ValidateDirectCounterMeter::postorder(const IR::MethodCallStatement *mcs) {
    auto mi = P4::MethodInstance::resolve(mcs->methodCall, refMap, typeMap);
    if (auto a = mi->to<P4::ExternMethod>()) {
        return validateMethodInvocation(a);
    }
}

void CollectAddOnMissTable::postorder(const IR::P4Table *t) {
    bool use_add_on_miss = false;
    auto add_on_miss = t->properties->getProperty("add_on_miss");
    cstring default_actname = "NoAction";
    if (add_on_miss == nullptr) return;
    if (add_on_miss->value->is<IR::ExpressionValue>()) {
        auto expr = add_on_miss->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error(ErrorType::ERR_UNEXPECTED, "%1%: expected boolean for 'add_on_miss' property",
                    add_on_miss);
            return;
        } else {
            use_add_on_miss = expr->to<IR::BoolLiteral>()->value;
            if (use_add_on_miss) structure->learner_tables.insert(t->name.name);
        }
    }

    /* sanity checks */
    auto default_action = t->properties->getProperty("default_action");
    if (use_add_on_miss && default_action == nullptr) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "%1%: add_on_miss property is defined, "
                "but default_action not specificed for table %2%",
                default_action, t->name);
        return;
    }
    if (default_action->value->is<IR::ExpressionValue>()) {
        auto expr = default_action->value->to<IR::ExpressionValue>()->expression;
        BUG_CHECK(expr->is<IR::MethodCallExpression>(), "%1%: expected expression to an action",
                  default_action);
        auto mi =
            P4::MethodInstance::resolve(expr->to<IR::MethodCallExpression>(), refMap, typeMap);
        BUG_CHECK(mi->is<P4::ActionCall>(), "%1%: expected action in default_action",
                  default_action);
        auto ac = mi->to<P4::ActionCall>()->action;
        if (mi->to<P4::ActionCall>()->action->parameters->parameters.size() != 0) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1%: action cannot have action argument when used with add_on_miss",
                    default_action);
        }
        default_actname = ac->name.name;
    }
    if (use_add_on_miss) {
        for (auto action : t->getActionList()->actionList) {
            auto action_decl = refMap->getDeclaration(action->getPath())->to<IR::P4Action>();
            // Map the compiler generated internal name of action (emitted in .spec file) with
            // user visible name in P4 program.
            // To get the user visible name, strip any prefixes from externalName.
            cstring userVisibleName = action_decl->externalName();
            userVisibleName = userVisibleName.findlast('.');
            userVisibleName = userVisibleName.trim(".\t\n\r");
            structure->learner_action_map.emplace(std::make_pair(userVisibleName, default_actname),
                                                  action_decl->name.name);
            structure->learner_action_table.emplace(action_decl->externalName(), t);
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
    BUG_CHECK(ctxt != nullptr, "%1% add_entry extern can only be used in an action", mcs);
    // In p4c, by design, only  backend checks args of an extern.
    BUG_CHECK(mce->arguments->size() == 3, "%1%: expected 3 arguments in add_entry extern", mcs);
    auto action = mce->arguments->at(0);
    // assuming syntax check is already performed earlier
    auto action_name = action->expression->to<IR::StringLiteral>()->value;
    structure->learner_actions.insert(action_name);
    return;
}

void ValidateAddOnMissExterns::postorder(const IR::MethodCallStatement *mcs) {
    bool isValidExternCall = false;
    cstring propName = "";
    auto mce = mcs->methodCall;
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ExternFunction>()) {
        return;
    }
    auto func = mi->to<P4::ExternFunction>();
    auto externFuncName = func->method->name;
    if (externFuncName != "restart_expire_timer" && externFuncName != "set_entry_expire_time" &&
        externFuncName != "add_entry")
        return;
    auto act = findOrigCtxt<IR::P4Action>();
    BUG_CHECK(act != nullptr, "%1%: %2% extern can only be used in an action", mcs, externFuncName);
    auto tbl = ::get(structure->learner_action_table, act->externalName());
    if (externFuncName == "restart_expire_timer" || externFuncName == "set_entry_expire_time") {
        bool use_idle_timeout_with_auto_delete = false;
        if (tbl) {
            auto idle_timeout_with_auto_delete =
                tbl->properties->getProperty("idle_timeout_with_auto_delete");
            if (idle_timeout_with_auto_delete != nullptr) {
                propName = "idle_timeout_with_auto_delete";
                if (idle_timeout_with_auto_delete->value->is<IR::ExpressionValue>()) {
                    auto expr =
                        idle_timeout_with_auto_delete->value->to<IR::ExpressionValue>()->expression;
                    if (!expr->is<IR::BoolLiteral>()) {
                        ::error(
                            ErrorType::ERR_UNEXPECTED,
                            "%1%: expected boolean for 'idle_timeout_with_auto_delete' property",
                            idle_timeout_with_auto_delete);
                        return;
                    } else {
                        use_idle_timeout_with_auto_delete = expr->to<IR::BoolLiteral>()->value;
                        if (use_idle_timeout_with_auto_delete) isValidExternCall = true;
                    }
                }
            }
        }
    } else if (externFuncName == "add_entry") {
        bool found = false;
        if (tbl) {
            auto mce = mcs->methodCall;
            auto args = mce->arguments;
            auto at = args->at(0)->expression;
            auto an = at->to<IR::StringLiteral>()->value;
            auto add_on_miss = tbl->properties->getProperty("add_on_miss");
            if (add_on_miss != nullptr) {
                propName = "add_on_miss";
                if (add_on_miss->value->is<IR::ExpressionValue>()) {
                    auto expr = add_on_miss->value->to<IR::ExpressionValue>()->expression;
                    if (!expr->is<IR::BoolLiteral>()) {
                        ::error(ErrorType::ERR_UNEXPECTED,
                                "%1%: expected boolean for 'add_on_miss' property", add_on_miss);
                        return;
                    } else {
                        auto use_add_on_miss = expr->to<IR::BoolLiteral>()->value;
                        if (use_add_on_miss) {
                            isValidExternCall = true;
                            cstring st = getDefActionName(tbl);
                            if (st != act->name.name) {  // checks caller
                                ::error(ErrorType::ERR_UNEXPECTED,
                                        "%1% is not called from a default action: %2% ", mcs,
                                        act->name.name);
                                return;
                            } else if (st == an) {  // checks arg0
                                ::error(ErrorType::ERR_UNEXPECTED,
                                        "%1% action cannot be default action: %2%:", mcs, an);
                                return;
                            }
                            for (auto action : tbl->getActionList()->actionList) {
                                if (action->getName().originalName == an) found = true;
                            }
                            if (!found) {
                                ::error(
                                    ErrorType::ERR_UNEXPECTED,
                                    "%1% first arg action name %2% is not any action in table %3%",
                                    mcs, an, tbl->name.name);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!isValidExternCall) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "%1% must only be called from within an action with '%2% %3%'"
                " property equal to true",
                mcs, propName, act->name.name);
    }
    return;
}

bool ElimHeaderCopy::isHeader(const IR::Expression *e) {
    auto type = typeMap->getType(e);
    if (type) return type->is<IR::Type_Header>() && !e->is<IR::MethodCallExpression>();
    return false;
}

const IR::Node *ElimHeaderCopy::preorder(IR::AssignmentStatement *as) {
    if (isHeader(as->left) && isHeader(as->right)) {
        if (auto path = as->left->to<IR::PathExpression>()) {
            replacementMap.insert(
                std::make_pair(path->path->name.name, as->right->to<IR::Member>()));
            return new IR::EmptyStatement();
        } else if (auto path = as->right->to<IR::PathExpression>()) {
            replacementMap.insert(
                std::make_pair(path->path->name.name, as->left->to<IR::Member>()));
            return new IR::EmptyStatement();
        } else {
            IR::ID methodName;
            IR::IndexedVector<IR::StatOrDecl> components;
            // copy validity flag
            auto isValid =
                new IR::Member(as->right->srcInfo, as->right, IR::ID(IR::Type_Header::isValid));
            auto result =
                new IR::MethodCallExpression(as->right->srcInfo, IR::Type::Boolean::get(), isValid);
            typeMap->setType(isValid,
                             new IR::Type_Method(IR::Type::Boolean::get(), new IR::ParameterList(),
                                                 IR::Type_Header::isValid));
            auto method =
                new IR::Member(as->left->srcInfo, as->left, IR::ID(IR::Type_Header::setValid));
            auto mc = new IR::MethodCallExpression(as->left->srcInfo, method,
                                                   new IR::Vector<IR::Argument>());
            typeMap->setType(method,
                             new IR::Type_Method(IR::Type_Void::get(), new IR::ParameterList(),
                                                 IR::Type_Header::setValid));
            components.push_back(new IR::MethodCallStatement(mc->srcInfo, mc));
            auto ifTrue = components;
            components.clear();
            auto method1 =
                new IR::Member(as->left->srcInfo, as->left, IR::ID(IR::Type_Header::setInvalid));
            auto mc1 = new IR::MethodCallExpression(as->left->srcInfo, method1,
                                                    new IR::Vector<IR::Argument>());
            typeMap->setType(method1,
                             new IR::Type_Method(IR::Type_Void::get(), new IR::ParameterList(),
                                                 IR::Type_Header::setInvalid));
            components.push_back(new IR::MethodCallStatement(mc1->srcInfo, mc1));
            auto ifFalse = components;
            components.clear();
            auto ifStatement = new IR::IfStatement(result, new IR::BlockStatement(ifTrue),
                                                   new IR::BlockStatement(ifFalse));
            components.push_back(ifStatement);
            // both are non temporary header instance do element wise copy
            for (auto field : typeMap->getType(as->left)->to<IR::Type_Header>()->fields) {
                components.push_back(
                    new IR::AssignmentStatement(as->srcInfo, new IR::Member(as->left, field->name),
                                                new IR::Member(as->right, field->name)));
            }
            return new IR::BlockStatement(as->srcInfo, components);
        }
    }
    return as;
}

/**
 * Replace method call expression with temporary instances
 * like eth_0.setInvalid()
 * with empty statement as all uses of eth_0 already replaced
 *
 */
const IR::Node *ElimHeaderCopy::preorder(IR::MethodCallStatement *mcs) {
    auto me = mcs->methodCall;
    auto m = me->method->to<IR::Member>();
    if (!m) return mcs;
    if (!isHeader(m->expr)) return mcs;
    auto expr = m->expr->to<IR::PathExpression>();
    if (expr && replacementMap.count(expr->path->name.name)) {
        return new IR::EmptyStatement();
    }
    return mcs;
}

const IR::Node *ElimHeaderCopy::postorder(IR::Member *m) {
    if (!isHeader(m->expr)) {
        return m;
    }
    auto expr = m->expr->to<IR::PathExpression>();
    if (expr && replacementMap.count(expr->path->name.name)) {
        return new IR::Member(replacementMap.at(expr->path->name.name), m->member);
    }
    return m;
}

const IR::Node *DpdkAddPseudoHeaderDecl::preorder(IR::P4Program *program) {
    if (is_all_args_header) return program;
    auto *annotations = new IR::Annotations({new IR::Annotation(IR::ID("__pseudo_header__"), {})});
    const IR::Type_Header *pseudo_hdr = new IR::Type_Header(pseudoHeaderTypeName, annotations);
    allTypeDecls.push_back(pseudo_hdr);
    allTypeDecls.append(program->objects);
    program->objects = allTypeDecls;
    return program;
}

// add pseudo header field to the headers struct for it be instantiated
const IR::Node *DpdkAddPseudoHeaderDecl::preorder(IR::Type_Struct *st) {
    if (is_all_args_header) return st;
    bool header_found = isHeadersStruct(st);
    if (header_found) {
        IR::IndexedVector<IR::StructField> fields;
        auto *annotations =
            new IR::Annotations({new IR::Annotation(IR::ID("__pseudo_header__"), {})});
        auto *type = new IR::Type_Name(new IR::Path(pseudoHeaderTypeName));
        const IR::StructField *new_field1 =
            new IR::StructField(pseudoHeaderInstanceName, annotations, type);
        fields = st->fields;
        fields.push_back(new_field1);
        auto *st1 =
            new IR::Type_Struct(st->srcInfo, st->name, st->annotations, st->typeParameters, fields);
        return st1;
    }
    return st;
}

std::pair<IR::AssignmentStatement *, IR::Member *>
MoveNonHeaderFieldsToPseudoHeader::addAssignmentStmt(const IR::Expression *e) {
    const IR::Type_Bits *type = nullptr;
    if (const IR::NamedExpression *ne = e->to<IR::NamedExpression>()) {
        type = typeMap->getType(ne->expression, true)->to<IR::Type_Bits>();
    } else {
        type = typeMap->getType(e, true)->to<IR::Type_Bits>();
    }
    if (e->is<IR::Constant>() && (type->width_bits() > 64)) {
        type = IR::Type_Bits::get(64);
    }
    auto name = refMap->newName("pseudo");
    auto aligned_type = getEightBitAlignedType(type);
    pseudoFieldNameType.push_back(std::pair<cstring, const IR::Type *>(name, aligned_type));
    auto mem0 = new IR::Member(new IR::PathExpression(IR::ID("h")),
                               IR::ID(DpdkAddPseudoHeaderDecl::pseudoHeaderInstanceName));
    auto mem1 = new IR::Member(mem0, IR::ID(name));
    if (auto cst = e->to<IR::Constant>()) {
        return {new IR::AssignmentStatement(mem1, new IR::Constant(aligned_type, cst->value)),
                mem1};
    }
    const IR::Expression *cast1 = e;
    if (type->width_bits() != aligned_type->width_bits()) cast1 = new IR::Cast(aligned_type, e);
    return {new IR::AssignmentStatement(mem1, cast1), mem1};
}

const IR::Node *MoveNonHeaderFieldsToPseudoHeader::postorder(IR::AssignmentStatement *assn) {
    if (is_all_args_header) return assn;
    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    if ((isLargeFieldOperand(assn->left) && !isLargeFieldOperand(assn->right) &&
         !isInsideHeader(assn->right)) ||
        (isLargeFieldOperand(assn->left) && assn->right->is<IR::Constant>())) {
        auto expr = assn->right;
        if (auto base = assn->right->to<IR::Cast>()) expr = base->expr;
        if (auto cst = assn->right->to<IR::Constant>()) {
            if (!cst->fitsUint64()) {
                ::error(ErrorType::ERR_OVERLIMIT,
                        "DPDK target supports up-to 64-bit immediate values, %1% exceeds the limit",
                        cst);
                return assn;
            }
        }
        auto stm = addAssignmentStmt(expr);
        result->push_back(stm.first);
        if (auto base = assn->right->to<IR::Cast>()) {
            auto assn1 = new IR::AssignmentStatement(assn->srcInfo, assn->left,
                                                     new IR::Cast(base->destType, stm.second));
            result->push_back(assn1);
        } else if (assn->right->is<IR::Constant>()) {
            auto assn1 = new IR::AssignmentStatement(assn->srcInfo, assn->left,
                                                     new IR::Cast(assn->left->type, stm.second));
            result->push_back(assn1);
        } else {
            assn->right = stm.second;
            result->push_back(assn);
        }
        return result;
    }
    if (!isLargeFieldOperand(assn->left) && isLargeFieldOperand(assn->right) &&
        !isInsideHeader(assn->left)) {
        auto expr = assn->right;
        auto stm = addAssignmentStmt(expr);
        result->push_back(stm.first);
        if (auto base = assn->right->to<IR::Cast>()) {
            auto assn1 = new IR::AssignmentStatement(assn->srcInfo, assn->left,
                                                     new IR::Cast(base->destType, stm.second));
            result->push_back(assn1);
        } else {
            assn->right = stm.second;
            result->push_back(assn);
        }
        return result;
    }
    return assn;
}

const IR::Node *MoveNonHeaderFieldsToPseudoHeader::postorder(IR::MethodCallStatement *statement) {
    if (is_all_args_header) return statement;
    auto mce = statement->methodCall;
    bool added_copy = false;
    IR::Type_Name *newTname = nullptr;
    const IR::Type_Bits *newTname0 = nullptr;
    const IR::Expression *newarg = nullptr;
    IR::StructExpression *struct_exp = nullptr;
    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    if (auto *m = mce->method->to<IR::Member>()) {
        if (auto *type = typeMap->getType(m->expr)->to<IR::Type_Extern>()) {
            if (type->name == "InternetChecksum") {
                if (m->member == "add" || m->member == "subtract") {
                    auto components = new IR::IndexedVector<IR::NamedExpression>();
                    for (auto arg : *mce->arguments) {
                        if (auto tmp = arg->expression->to<IR::StructExpression>()) {
                            for (auto c : tmp->components) {
                                bool is_header_field = false;
                                if (auto m = c->expression->to<IR::Member>()) {
                                    if ((is_header_field = typeMap->getType(m->expr, true)
                                                               ->is<IR::Type_Header>()))
                                        components->push_back(c);
                                }
                                if (!is_header_field) {
                                    // replace non header expression with pseudo header field,
                                    // after initializing it with existing expression
                                    added_copy = true;
                                    auto stm = addAssignmentStmt(c->expression);
                                    result->push_back(stm.first);
                                    components->push_back(
                                        new IR::NamedExpression(c->srcInfo, c->name, stm.second));
                                }
                            }
                            // Creating Byte Aligned struct, it is type of struct expression
                            IR::IndexedVector<IR::StructField> fields;
                            auto tmps0 = tmp->type->to<IR::Type_Struct>();
                            for (auto f : tmps0->fields) {
                                BUG_CHECK(f->type->is<IR::Type_Bits>(), "Unexpected type");
                                auto type = f->type->to<IR::Type_Bits>();
                                fields.push_back(
                                    new IR::StructField(f->name, getEightBitAlignedType(type)));
                            }
                            auto newName = refMap->newName(tmps0->name);
                            newTname = new IR::Type_Name(newName);
                            auto newStructType =
                                new IR::Type_Struct(tmps0->srcInfo, newName, tmps0->annotations,
                                                    tmps0->typeParameters, fields);
                            newStructTypes.push_back(newStructType);
                            struct_exp = new IR::StructExpression(tmp->srcInfo, newStructType,
                                                                  newTname, *components);
                        } else if (arg->expression->is<IR::Constant>() ||
                                   arg->expression->is<IR::Member>()) {
                            auto m = arg->expression->to<IR::Member>();
                            if (m && (typeMap->getType(m, true)->is<IR::Type_Header>() ||
                                      typeMap->getType(m->expr, true)->is<IR::Type_Header>())) {
                                break;
                            }
                            added_copy = true;
                            auto stm = addAssignmentStmt(arg->expression);
                            auto type =
                                typeMap->getType(arg->expression, true)->to<IR::Type_Bits>();
                            newTname0 = getEightBitAlignedType(type);
                            newarg = stm.second;
                            result->push_back(stm.first);
                        } else {
                            BUG("unexpected expression");
                        }
                    }
                }
            }
        }
    }
    if (added_copy) {
        auto targ = new IR::Vector<IR::Type>;
        if (struct_exp) {
            targ->push_back(newTname);
            auto expression = new IR::MethodCallExpression(
                mce->srcInfo, mce->type, mce->method, targ,
                new IR::Vector<IR::Argument>(new IR::Argument(struct_exp)));
            result->push_back(new IR::MethodCallStatement(expression));
        } else {
            targ->push_back(newTname0);
            auto expression = new IR::MethodCallExpression(
                mce->srcInfo, mce->type, mce->method, targ,
                new IR::Vector<IR::Argument>(new IR::Argument(newarg)));
            result->push_back(new IR::MethodCallStatement(expression));
        }
        return result;
    } else
        return statement;
}

const IR::Node *AddFieldsToPseudoHeader::preorder(IR::Type_Header *h) {
    if (is_all_args_header) return h;
    auto annon = h->getAnnotation("__pseudo_header__");
    if (annon == nullptr) return h;
    IR::IndexedVector<IR::StructField> fields = h->fields;
    for (auto &p : MoveNonHeaderFieldsToPseudoHeader::pseudoFieldNameType) {
        fields.push_back(new IR::StructField(p.first, p.second));
    }
    return new IR::Type_Header(h->srcInfo, h->name, h->annotations, h->typeParameters, fields);
}

std::vector<std::pair<cstring, const IR::Type *>>
    MoveNonHeaderFieldsToPseudoHeader::pseudoFieldNameType;
cstring DpdkAddPseudoHeaderDecl::pseudoHeaderInstanceName;
cstring DpdkAddPseudoHeaderDecl::pseudoHeaderTypeName;

const IR::Node *MoveCollectedStructLocalVariableToMetadata::preorder(IR::Type_Struct *s) {
    if (s->equiv(*CollectStructLocalVariables::metadataStrct)) {
        for (auto kv : CollectStructLocalVariables::fieldNameType) {
            s->fields.push_back(new IR::StructField(kv.first, kv.second));
        }
        CollectStructLocalVariables::fieldNameType.clear();
    }
    return s;
}

const IR::Node *MoveCollectedStructLocalVariableToMetadata::postorder(IR::P4Program *p) {
    auto objs = p->objects;

    IR::Vector<IR::Node> old_objs;
    for (auto obj : objs) {
        bool is_found = false;
        for (auto obj1 : CollectStructLocalVariables::fieldNameType) {
            if (obj->equiv(*obj1.second)) {
                is_found = true;
                break;
            }
        }
        if (!is_found) old_objs.push_back(obj);
    }
    IR::Vector<IR::Node> newobjs;
    for (auto obj1 : CollectStructLocalVariables::fieldNameType) newobjs.push_back(obj1.second);
    newobjs.append(old_objs);
    p->objects = newobjs;
    return p;
}

const IR::Node *MoveCollectedStructLocalVariableToMetadata::postorder(IR::P4Control *c) {
    IR::IndexedVector<IR::Declaration> decls;
    for (auto d : c->controlLocals) {
        if (d->is<IR::Declaration_Variable>()) {
            if (!typeMap->getType(d, true)->to<IR::Type_Struct>()) decls.push_back(d);
        } else
            decls.push_back(d);
    }
    c->controlLocals = decls;
    return c;
}

const IR::Node *MoveCollectedStructLocalVariableToMetadata::postorder(IR::P4Parser *p) {
    IR::IndexedVector<IR::Declaration> decls;
    for (auto d : p->parserLocals) {
        if (d->is<IR::Declaration_Variable>()) {
            if (!typeMap->getType(d, true)->to<IR::Type_Struct>()) decls.push_back(d);
        } else
            decls.push_back(d);
    }
    p->parserLocals = decls;
    return p;
}

IR::Vector<IR::Node> CollectStructLocalVariables::type_tobe_moved_at_top;
std::map<cstring, const IR::Type *> CollectStructLocalVariables::fieldNameType{};

const IR::Node *CollectStructLocalVariables::preorder(IR::PathExpression *path) {
    auto parser = findContext<IR::P4Parser>();
    auto control = findContext<IR::P4Control>();
    auto program = findContext<IR::P4Program>();
    cstring metdataParamname;
    if (parser) {
        metdataParamname = parser->type->applyParams->parameters.at(2)->name;
    } else if (control) {
        metdataParamname = control->type->applyParams->parameters.at(1)->name;
    } else if (program) {
        return path;
    } else {
        BUG(" Unexpected context");
    }

    if (refMap->getDeclaration(path->path))
        if (auto decl = refMap->getDeclaration(path->path)->to<IR::Declaration_Variable>()) {
            if (auto type = typeMap->getType(decl, true)->to<IR::Type_Struct>()) {
                type_tobe_moved_at_top.push_back(type);
                fieldNameType.insert(
                    std::pair<cstring, const IR::Type *>(path->path->name.name, type));
                return new IR::Member(new IR::PathExpression(metdataParamname),
                                      path->path->name.name);
            }
        }
    return path;
}

const IR::Node *CollectStructLocalVariables::postorder(IR::P4Parser *p) {
    if (metadataStrct) return p;
    auto parser = getOriginal<IR::P4Parser>();
    // 3rd Parameter in parser block is of metadata struct
    auto pr = parser->type->applyParams->parameters.at(2);
    if (auto tn = pr->type->to<IR::Type_Name>()) {
        auto decl = typeMap->getTypeType(tn, true);
        auto ty = decl->to<IR::Type_Struct>();
        metadataStrct = ty;
    }
    return p;
}
const IR::Type_Struct *CollectStructLocalVariables::metadataStrct = nullptr;

// create and add register declaration instance to program
IR::IndexedVector<IR::StatOrDecl> *InsertReqDeclForIPSec::addRegDeclInstance(
    std::vector<cstring> portRegs) {
    auto decls = new IR::IndexedVector<IR::StatOrDecl>;
    for (auto reg : portRegs)
        decls->push_back(createRegDeclarationInstance(reg, IPSEC_PORT_REG_SIZE,
                                                      IPSEC_PORT_REG_INDEX_BITWIDTH,
                                                      IPSEC_PORT_REG_INITVAL_BITWIDTH));
    return decls;
}

const IR::Node *InsertReqDeclForIPSec::preorder(IR::P4Program *program) {
    if (!is_ipsec_used) return program;
    cstring resName = "";
    if (!reservedNames(refMap, registerInstanceNames, resName)) {
        ::error(ErrorType::ERR_RESERVED, "%1% name is reserved for DPDK IPSec port register",
                resName);
        return program;
    }
    bool ipsecHdrAdded = false;
    bool ipsecPortRegAdded = false;
    auto new_objs = new IR::Vector<IR::Node>;

    // Create platform_hdr structure with just one field to hold the security association index
    IR::ID newHeaderFieldName("sa_id");
    IR::IndexedVector<IR::StructField> newHeaderFields;
    newHeaderFields.push_back(
        new IR::StructField(newHeaderFieldName, IR::Type_Bits::get(sa_id_width)));
    if (!reservedNames(refMap, {newHeaderName}, resName)) {
        ::error(ErrorType::ERR_RESERVED, "%1% type name is reserved for DPDK platform header",
                newHeaderName);
        return program;
    }
    ipsecHeader = new IR::Type_Header(IR::ID(newHeaderName), newHeaderFields);

    // Programs using ipsec accelerator must contain ethernet and ipv4 headers, hence a struct
    // containing headerinstances for all headers must be present
    BUG_CHECK(structure->header_type != "",
              "Encapsulating struct containing all headers used in the program is missing");
    for (auto obj : program->objects) {
        if (auto st = obj->to<IR::Type_Struct>()) {
            // Add new platform header before the encapsulating header struct
            if (st->name.name == structure->header_type && !ipsecHdrAdded) {
                ipsecHdrAdded = true;
                new_objs->push_back(ipsecHeader);
            }
        } else if (obj->is<IR::P4Parser>()) {
            if (!ipsecPortRegAdded) {
                ipsecPortRegAdded = true;
                // Add all 4 ipsec port register declarations before parser
                auto decls = addRegDeclInstance(registerInstanceNames);
                for (auto d : *decls) new_objs->push_back(d);
            }
        }
        new_objs->push_back(obj);
    }
    if (!(ipsecHdrAdded && ipsecPortRegAdded))
        BUG("Missing platform header/IPSec port Registers needed for IPSec accelerator");
    program->objects = *new_objs;
    return program;
}

// Create an instance of ipsec_hdr in the header struct
const IR::Node *InsertReqDeclForIPSec::preorder(IR::Type_Struct *s) {
    if (!is_ipsec_used) return s;
    if (s->name.name == structure->header_type) {
        s->fields.push_back(
            new IR::StructField(IR::ID("ipsec_hdr"), new IR::Type_Name(IR::ID("platform_hdr_t"))));
    }
    return s;
}

const IR::Node *InsertReqDeclForIPSec::preorder(IR::P4Control *c) {
    if (!is_ipsec_used) return c;

    auto insertIpsecHeader = [this](IR::P4Control *c) {
        IR::IndexedVector<IR::StatOrDecl> stmt;
        auto pkt_out = c->type->applyParams->parameters.at(0);
        auto args = new IR::Vector<IR::Argument>();
        structure->ipsec_header =
            new IR::Member(new IR::PathExpression(IR::ID("h")), IR::ID("ipsec_hdr"));
        args->push_back(new IR::Argument(structure->ipsec_header));
        auto typeArgs = new IR::Vector<IR::Type>();
        typeArgs->push_back(new IR::Type_Name(IR::ID(newHeaderName)));
        auto regRdArgs = new IR::Vector<IR::Argument>();

        // Register index is always 0 as the ipsec port registers are all single location registers
        regRdArgs->push_back(
            new IR::Argument(new IR::Constant(IR::Type::Bits::get(32), IPSEC_PORT_REG_INDEX)));
        /*  Set the ipsec output port based on packet direction and emit ipsec_hdr
            if (ipsec_hdr.isValid())
                if (direction == NET_TO_HOST)
                    port_out = ipsec_inbound_port_out.read(0);
                else
                    port_out = ipsec_outbound_port_out.read(0);
                emit(ipsec_hdr);
        */
        auto regRdOutInbound = new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(IR::ID("ipsec_port_out_inbound")),
                           IR::ID("read")),
            regRdArgs);
        auto regRdOutOutbound = new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(IR::ID("ipsec_port_out_outbound")),
                           IR::ID("read")),
            regRdArgs);
        auto port_out = new IR::Member(new IR::PathExpression(IR::ID("m")),
                                       IR::ID("pna_main_output_metadata_output_port"));
        auto portOutInBound =
            new IR::AssignmentStatement(c->body->srcInfo, port_out, regRdOutInbound);
        auto portOutOutBound =
            new IR::AssignmentStatement(c->body->srcInfo, port_out, regRdOutOutbound);
        auto isNetToHost = new IR::Equ(c->body->srcInfo,
                                       new IR::Member(new IR::PathExpression(IR::ID("m")),
                                                      IR::ID("pna_main_input_metadata_direction")),
                                       new IR::Constant(NET_TO_HOST));
        auto isValid =
            new IR::Member(c->body->srcInfo,
                           new IR::Member(new IR::PathExpression(IR::ID("h")), IR::ID("ipsec_hdr")),
                           IR::ID(IR::Type_Header::isValid));

        auto ipsecIsValid =
            new IR::MethodCallExpression(c->body->srcInfo, IR::Type::Boolean::get(), isValid);
        auto ifTrue =
            new IR::IfStatement(c->body->srcInfo, isNetToHost, portOutInBound, portOutOutBound);
        IR::Statement *fillIpsecPortOut = new IR::IfStatement(ipsecIsValid, ifTrue, nullptr);
        IR::Statement *ipsecEmit = new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(pkt_out->name), IR::ID("emit")), typeArgs, args));
        stmt.push_back(fillIpsecPortOut);
        stmt.push_back(ipsecEmit);
        return stmt;
    };

    for (auto kv : structure->deparsers) {
        if (kv.second->type->name == c->name) {
            auto body = new IR::BlockStatement(c->body->srcInfo);
            auto ipsecEmit = insertIpsecHeader(c);
            for (auto s : ipsecEmit) body->push_back(s);
            for (auto s : c->body->components) body->push_back(s);
            c->body = body;
            break;
        }
    }
    return c;
}

}  // namespace DPDK
