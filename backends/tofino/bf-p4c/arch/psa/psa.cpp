/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <cmath>
#include <fstream>
#include "bf-p4c/device.h"
#include "psa.h"
#include "rewrite_packet_path.h"
#include "lib/bitops.h"
#include "ir/ir.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/validateProperties.h"
#include "psa_converters.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/intrinsic_metadata.h"
#include "bf-p4c/arch/rewrite_action_selector.h"
#include "bf-p4c/arch/fromv1.0/add_metadata_parser_states.h"
#include "rewrite_bridge_metadata.h"

namespace BFN {

namespace PSA {
//////////////////////////////////////////////////////////////////////////////////////////////

/** \ingroup PortableSwitchTranslation */
class PacketPathTo8Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum *type) const override {
        if (type->name != "PSA_PacketPath_t") {
            return false;
        }
        LOG3("Convert Enum to Bits " << type->name.toString());
        return true;
    }

    unsigned enumSize(unsigned /* size */) const override { return 8; }
};

/**
 * \ingroup PortableSwitchTranslation
 * L-value of MeterColor_t is not deduced during an assignment
 * E.g.:
 *
 *     struct metadata_t {
 *         PSA_MeterColor_t color_value;
 *     }
 *
 * gets converted to:
 *
 *     struct metadata_t {
 *         MeterColor_t color_value;
 *     }
 *
 * Usage is an assignment:
 *
 *     user_meta.color_value = meter0.execute(idx);
 *
 * Output of meter is bit<8> which cannot be unified to MeterColor_t
 * To overcome this issue, 'MeterColor_t' assignments occurring in Structs and
 * Headers must be explicitly converted to an 8 bit value
 */
class MeterColorTypeTo8Bits: public Transform {
    const IR::Node* postorder(IR::Type_Name *t) override {
        auto path = t->path->to<IR::Path>();
        if (path->name != "MeterColor_t") {
            return t;
        }
        if (!findContext<IR::Type_Struct>()
                && !findContext<IR::Type_Header>()
                && !findContext<IR::Declaration_Variable>())
            return t;
        LOG3("Convert L-Value of MeterColor_t to 8 Bits " << path);
        return IR::Type_Bits::get(8);
    }
};

/** \ingroup PortableSwitchTranslation */
class AnalyzeProgram : public Inspector {
    template<class P4Type, class BlockType>
    void analyzeArchBlock(cstring gressName, cstring blockName, cstring type) {
        auto main = structure->toplevel->getMain();
        CHECK_NULL(main);
        auto gress = main->findParameterValue(gressName);
        CHECK_NULL(gress);
        auto gressPackage = gress->to<IR::PackageBlock>();
        CHECK_NULL(gressPackage);
        auto block = gressPackage->findParameterValue(blockName);
        CHECK_NULL(block);
        auto blockType = block->to<BlockType>();
        CHECK_NULL(blockType);
        LOG1("find block " << blockType->container->name.toString() << " of type " << type);
        structure->blockNames.emplace(type, blockType->container->name);
    }

 public:
    explicit AnalyzeProgram(PSA::ProgramStructure* structure,
                            P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : structure(structure), refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(structure); }

    bool preorder(const IR::P4Program*) override {
        analyzeArchBlock<IR::P4Parser, IR::ParserBlock>(
            "ingress"_cs, "ip"_cs, PSA::ProgramStructure::INGRESS_PARSER);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(
            "ingress"_cs, "ig"_cs, PSA::ProgramStructure::INGRESS);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(
            "ingress"_cs, "id"_cs, PSA::ProgramStructure::INGRESS_DEPARSER);
        analyzeArchBlock<IR::P4Parser, IR::ParserBlock>(
            "egress"_cs, "ep"_cs, PSA::ProgramStructure::EGRESS_PARSER);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(
            "egress"_cs, "eg"_cs, PSA::ProgramStructure::EGRESS);
        analyzeArchBlock<IR::P4Control, IR::ControlBlock>(
            "egress"_cs, "ed"_cs, PSA::ProgramStructure::EGRESS_DEPARSER);
        return true;
    }
    void postorder(const IR::Type_Action* node) override {
        structure->action_types.push_back(node);
    }
    void postorder(const IR::Type_StructLike* node) override {
        structure->type_declarations.emplace(node->name, node);
    }
    void postorder(const IR::Type_Typedef* node) override {
        structure->type_declarations.emplace(node->name, node);
    }
    void postorder(const IR::Type_Enum* node) override {
        structure->enums.emplace(node->name, node);
    }
    void postorder(const IR::Type_SerEnum* node) override {
        LOG3("ser enum " << node);
        structure->ser_enums.emplace(node->name, node);
    }
    void postorder(const IR::P4Parser* node) override {
        structure->parsers.emplace(node->name, node);
        process_packet_path_params(node);
    }
    void postorder(const IR::P4Control* node) override {
        structure->controls.emplace(node->name, node);
        process_packet_path_params(node);
    }

    // 'compiler_generated_metadata_t' is the last struct in the program.
    void end_apply() override {
        auto* cgAnnotation = new IR::Annotations({
             new IR::Annotation(IR::ID("__compiler_generated"), { })});

        auto cgm = new IR::Type_Struct("compiler_generated_metadata_t", cgAnnotation);
        cgm->fields.push_back(
            new IR::StructField("mirror_id", IR::Type::Bits::get(10)));
        cgm->fields.push_back(
            new IR::StructField("packet_path", IR::Type::Bits::get(8)));
        // TODO: Map clone_src + clone_digest_id to packet_path
        cgm->fields.push_back(
            new IR::StructField("clone_src", IR::Type::Bits::get(4)));
        cgm->fields.push_back(
            new IR::StructField("clone_digest_id", IR::Type::Bits::get(4)));
        // add bridge metadata
        cgm->fields.push_back(new IR::StructField(BFN::BRIDGED_MD,
                                                  structure->bridge.p4Type));
        cgm->fields.push_back(new IR::StructField("__recirculate_data",
                                                  structure->recirculate.p4Type));
        cgm->fields.push_back(new IR::StructField("__resubmit_data",
                                                  structure->resubmit.p4Type));
        cgm->fields.push_back(new IR::StructField("__clone_i2e_data",
                                                  structure->clone_i2e.p4Type));
        cgm->fields.push_back(new IR::StructField("__clone_e2e_data",
                                                  structure->clone_e2e.p4Type));
        cgm->fields.push_back(new IR::StructField("drop", IR::Type::Boolean::get()));
        cgm->fields.push_back(new IR::StructField("resubmit", IR::Type::Boolean::get()));
        cgm->fields.push_back(new IR::StructField("clone_i2e", IR::Type::Boolean::get()));
        cgm->fields.push_back(new IR::StructField("clone_e2e", IR::Type::Boolean::get()));

        structure->type_declarations.emplace("compiler_generated_metadata_t"_cs, cgm);
    }

    // program structure keeps a map from architecture param name to user-provided param name
    // it also keeps the relevant information to translate resubmit/clone/recirc operation to tna.
    void process_packet_path_params(const IR::P4Parser* node) {
        if (node->name == structure->getBlockName(PSA::ProgramStructure::INGRESS_PARSER)) {
            auto param = node->getApplyParameters()->getParameter(1);
            structure->ingress_parser.psaParams.emplace("hdr"_cs, param->name);
            param = node->getApplyParameters()->getParameter(2);
            structure->ingress_parser.psaParams.emplace("metadata"_cs, param->name);
            structure->metadataType = typeMap->getTypeType(param->type, true);
            param = node->getApplyParameters()->getParameter(3);
            structure->ingress_parser.psaParams.emplace(PSA::INP_INTR_MD, param->name);
            param = node->getApplyParameters()->getParameter(4);
            structure->resubmit.paramNameInParser = param->name;
            create_metadata_header(param, "__resubmit_data"_cs, INGRESS, structure->resubmit);
            structure->ingress_parser.psaParams.emplace("resubmit_metadata"_cs, param->name);
            param = node->getApplyParameters()->getParameter(5);
            structure->recirculate.paramNameInParser = param->name;
            create_metadata_header(param, "__recirculate_data"_cs, INGRESS, structure->recirculate);
            structure->ingress_parser.psaParams.emplace("recirc_metadata"_cs, param->name);
        } else if (node->name == structure->getBlockName(PSA::ProgramStructure::EGRESS_PARSER)) {
            auto param = node->getApplyParameters()->getParameter(1);
            structure->egress_parser.psaParams.emplace("hdr"_cs, param->name);
            param = node->getApplyParameters()->getParameter(2);
            structure->egress_parser.psaParams.emplace("metadata"_cs, param->name);
            structure->metadataType = typeMap->getTypeType(param->type, true);
            param = node->getApplyParameters()->getParameter(3);
            structure->egress_parser.psaParams.emplace(PSA::INP_INTR_MD, param->name);
            param = node->getApplyParameters()->getParameter(4);
            structure->bridge.paramNameInParser = param->name;
            structure->egress_parser.psaParams.emplace("normal_metadata"_cs, param->name);
            // add translation for bridged metadata
            // In PSA, bridge structure can be a struct or header or empty. Creating
            // a new header for bridged fields no matter what the original structure is
            create_metadata_header(param, BFN::BRIDGED_MD, EGRESS, structure->bridge);
            param = node->getApplyParameters()->getParameter(5);
            structure->clone_i2e.paramNameInParser = param->name;
            create_metadata_header(param, "__clone_i2e_data"_cs, EGRESS, structure->clone_i2e);
            structure->egress_parser.psaParams.emplace("clone_i2e_metadata"_cs, param->name);
            param = node->getApplyParameters()->getParameter(6);
            structure->clone_e2e.paramNameInParser = param->name;
            create_metadata_header(param, "__clone_e2e_data"_cs, EGRESS, structure->clone_e2e);
            structure->egress_parser.psaParams.emplace("clone_e2e_metadata"_cs, param->name);
        }
    }

    void create_metadata_header(const IR::Parameter* param, cstring headername,
                                gress_t gress, PacketPathInfo& packetStructure) {
        auto md_type = typeMap->getTypeType(param->type, true);
        if (auto t = md_type->to<IR::Type_StructLike>()) {
            cstring typeName =
                headername == BFN::BRIDGED_MD ? "__bridge_metadata_t"_cs : t->name.name;
            auto header = new IR::Type_Header(typeName, t->annotations, t->fields);
            structure->type_declarations[typeName] = header;
            for (auto f : header->to<IR::Type_StructLike>()->fields) {
                    structure->addMetadata(gress,
                        MetadataField{param->name, f->name, f->type->width_bits()},
                        MetadataField{headername, f->name,
                                  f->type->width_bits(), true});
            }
            packetStructure.p4Type = new IR::Type_Name(new IR::Path(typeName));
            packetStructure.structType = header;
        }
        return;
    }

    void analyzeIfStatement(const IR::IfStatement* ifStatement) {
        CHECK_NULL(ifStatement);
        if (!ifStatement->ifTrue || ifStatement->ifFalse) {
            return;
        }
        auto mce = ifStatement->condition->to<IR::MethodCallExpression>();
        if (!mce) return;
        auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
        if (auto ef = mi->to<P4::ExternFunction>()) {
            auto name = ef->method->name;
            structure->_map.emplace(ifStatement, nullptr);
            if (name == "psa_resubmit") {
                structure->resubmit.ifStatement = ifStatement;
            } else if (name == "psa_clone_i2e") {
                structure->clone_i2e.ifStatement = ifStatement;
            } else if (name == "psa_clone_e2e") {
                structure->clone_e2e.ifStatement = ifStatement;
            } else if (name == "psa_recirculate") {
                structure->recirculate.ifStatement = ifStatement;
            } else if (name == "psa_normal") {
                structure->bridge.ifStatement = ifStatement;
            }
        }
    }

    // assume only used in deparser.
    void collectPacketPathInfo(const IR::P4Control* node) {
        for (auto stmt : node->body->components) {
            if (auto ifStatement = stmt->to<IR::IfStatement>()) {
                analyzeIfStatement(ifStatement);
            }
        }
    }

    void process_packet_path_params(const IR::P4Control* node) {
        if (node->name == structure->getBlockName(ProgramStructure::INGRESS_DEPARSER)) {
            auto param = node->getApplyParameters()->getParameter(1);
            structure->clone_i2e.paramNameInDeparser = param->name;
            create_metadata_header(param, "__clone_i2e_data"_cs, INGRESS, structure->clone_i2e);
            param = node->getApplyParameters()->getParameter(2);
            structure->resubmit.paramNameInDeparser = param->name;
            create_metadata_header(param, "__resubmit_data"_cs, INGRESS, structure->resubmit);
            param = node->getApplyParameters()->getParameter(3);
            structure->bridge.paramNameInDeparser = param->name;
            create_metadata_header(param, BFN::BRIDGED_MD, INGRESS, structure->bridge);
            param = node->getApplyParameters()->getParameter(4);
            structure->ingress_deparser.psaParams.emplace("hdr"_cs, param->name);
            param = node->getApplyParameters()->getParameter(5);
            structure->ingress_deparser.psaParams.emplace("metadata"_cs, param->name);
            structure->metadataType = typeMap->getTypeType(param->type, true);
            param = node->getApplyParameters()->getParameter(6);
            structure->ingress_deparser.psaParams.emplace(PSA::INP_INTR_MD, param->name);
            collectPacketPathInfo(node);
        } else if (node->name == structure->getBlockName(ProgramStructure::EGRESS_DEPARSER)) {
            auto param = node->getApplyParameters()->getParameter(1);
            structure->clone_e2e.paramNameInDeparser = param->name;
            create_metadata_header(param, "__clone_e2e_data"_cs, EGRESS, structure->clone_e2e);
            param = node->getApplyParameters()->getParameter(2);
            structure->recirculate.paramNameInDeparser = param->name;
            create_metadata_header(param, "__recirculate_data"_cs, EGRESS, structure->recirculate);
            param = node->getApplyParameters()->getParameter(3);
            structure->egress_deparser.psaParams.emplace("hdr"_cs, param->name);
            param = node->getApplyParameters()->getParameter(4);
            structure->egress_deparser.psaParams.emplace("metadata"_cs, param->name);
            structure->metadataType = typeMap->getTypeType(param->type, true);
            param = node->getApplyParameters()->getParameter(5);
            structure->egress_deparser.psaParams.emplace(PSA::INP_INTR_MD, param->name);
            param = node->getApplyParameters()->getParameter(6);
            structure->egress_deparser.psaParams.emplace("edstd"_cs, param->name);
            collectPacketPathInfo(node);
        } else if (node->name == structure->getBlockName(ProgramStructure::INGRESS)) {
            auto param = node->getApplyParameters()->getParameter(0);
            structure->ingress.psaParams.emplace("hdr"_cs, param->name);
            param = node->getApplyParameters()->getParameter(1);
            structure->ingress.psaParams.emplace("metadata"_cs, param->name);
            structure->metadataType = typeMap->getTypeType(param->type, true);
            param = node->getApplyParameters()->getParameter(2);
            structure->ingress.psaParams.emplace(PSA::INP_INTR_MD, param->name);
            param = node->getApplyParameters()->getParameter(3);
            structure->ingress.psaParams.emplace(PSA::OUT_INTR_MD, param->name);
        } else if (node->name == structure->getBlockName(ProgramStructure::EGRESS)) {
            auto param = node->getApplyParameters()->getParameter(0);
            structure->egress.psaParams.emplace("hdr"_cs, param->name);
            param = node->getApplyParameters()->getParameter(1);
            structure->egress.psaParams.emplace("metadata"_cs, param->name);
            structure->metadataType = typeMap->getTypeType(param->type, true);
            param = node->getApplyParameters()->getParameter(2);
            structure->egress.psaParams.emplace(PSA::INP_INTR_MD, param->name);
            param = node->getApplyParameters()->getParameter(3);
            structure->egress.psaParams.emplace(PSA::OUT_INTR_MD, param->name);
        }
    }

 private:
    PSA::ProgramStructure* structure;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
};

/** \ingroup PortableSwitchTranslation */
class TranslateProgram : public Inspector {
    PSA::ProgramStructure* structure;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    // Used for deparser checksum
    std::map<cstring, IR::Vector<IR::Expression>> fieldListMap;

 public:
    explicit TranslateProgram(PSA::ProgramStructure* structure,
                                  P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
            : structure(structure), refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(structure); setName("TranslateProgram");
    }


    void InternetChecksumTranslation(const IR::StatOrDecl* stmt,
                                     const P4::ExternMethod* em) {
        auto declName = em->object->to<IR::Declaration_Instance>()->name;
        if (em->method->name == "add" || em->method->name == "subtract") {
            auto sourceList = (*em->expr->arguments)[0]->
                                   expression->to<IR::ListExpression>();
            auto typeArgs = new IR::Vector<IR::Type>({ sourceList->type });
            auto args = new IR::Vector<IR::Argument>({ new IR::Argument(sourceList) });
            auto mce = new IR::MethodCallExpression(
                new IR::Member(new IR::PathExpression(declName), em->method->name), typeArgs, args);
            auto csumCall = new IR::MethodCallStatement(mce);
            structure->_map.emplace(stmt, csumCall);
        }
        return;
    }

    int getErrorIdx(cstring errorName) {
        if (structure->error_to_constant.count(errorName)) {
            return structure->error_to_constant.at(errorName);
        } else {
            auto error_idx = 12 + structure->error_to_constant.size();
            if (error_idx > 15) {
                error("Cannot accomodate custom error %1%", errorName);
            } else {
                structure->error_to_constant[errorName] = error_idx;
                return error_idx;
            }
        }
        return 0;
    }

    void evaluateVerifyEnd(const IR::StatOrDecl* stmt,
                           const P4::ExternFunction* em,
                           gress_t gress,
                           cstring stateName) {
        auto equ = (*em->expr->arguments)[0]->expression;
        auto err = (*em->expr->arguments)[1]->expression->to<IR::Member>();
        CHECK_NULL(equ);
        Pattern::Match<IR::Expression> member;
        Pattern::Match<IR::MethodCallExpression> expr;
        cstring metaParam = gress == INGRESS ? "ig_intr_md_from_prsr"_cs :
                                                "eg_intr_md_from_prsr"_cs;
        int error_idx = getErrorIdx(err->member);
        if ((expr == member).match(equ)) {
            auto miExpr = P4::MethodInstance::resolve(expr, refMap, typeMap);
            if (auto emExpr = miExpr->to<P4::ExternMethod>()) {
                cstring nameExpr = emExpr->actualExternType->name;
                if (nameExpr == "InternetChecksum" && emExpr->method->name == "get") {
                    auto declName = emExpr->object->to<IR::Declaration_Instance>()->name;
                    auto list = new IR::ListExpression({member});
                    auto typeArgs = new IR::Vector<IR::Type>({ list->type });
                    auto args = new IR::Vector<IR::Argument>({ new IR::Argument(list) });
                    auto mce = new IR::MethodCallExpression(
                               new IR::Member(new IR::PathExpression(declName), "add"),
                               typeArgs, args);
                    auto addCall = new IR::MethodCallStatement(mce);
                    structure->_map.emplace(stmt, addCall);
                    auto verify = new IR::MethodCallExpression(new IR::Member(
                                  new IR::PathExpression(declName), "verify"));
                    auto rhs = new IR::Cast(IR::Type::Bits::get(1), verify);
                    auto parser_err = new IR::Member(
                                          new IR::PathExpression(metaParam),
                                          "parser_err");
                    auto lhs = new IR::Slice(parser_err, error_idx, error_idx);
                    auto verifyEnd = new IR::AssignmentStatement(lhs, rhs);
                    if (gress == INGRESS) {
                        structure->ingressParserStatements[stateName].push_back(verifyEnd);
                    } else {
                        structure->egressParserStatements[stateName].push_back(verifyEnd);
                    }
                }
            }
        } else {
            structure->state_to_verify[gress][stateName] = em->expr;
            structure->_map.emplace(stmt, nullptr);
        }
    }

    void translateResidualGet(const IR::Member* member, const P4::ExternMethod* emExpr,
                              const IR::StatOrDecl* stmt,
                              gress_t gress) {
        auto declName = emExpr->object->to<IR::Declaration_Instance>()->name;
        auto mce = new IR::MethodCallExpression(
                      new IR::Member(new IR::PathExpression(declName), "get"));
        if (gress == INGRESS) {
              auto metaparam = structure->ingress_parser.psaParams.at("metadata"_cs);
              auto residual = new IR::Member(new IR::PathExpression(metaparam), member->member);
              auto get = new IR::AssignmentStatement(residual, mce);
              structure->_map.emplace(stmt, get);
        } else if (gress == EGRESS) {
             auto metaparam = structure->egress_parser.psaParams.at("metadata"_cs);
             auto residual = new IR::Member(new IR::PathExpression(metaparam), member->member);
             auto get = new IR::AssignmentStatement(residual, mce);
             structure->_map.emplace(stmt, get);
        }
        return;
    }

    void postorder(const IR::ParserState* state) {
        auto ctxt = findContext<IR::P4Parser>();
        CHECK_NULL(ctxt);
        for (auto stmt : state->components) {
            if (auto mc = stmt->to<IR::MethodCallStatement>()) {
                auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap);
                if (auto em = mi->to<P4::ExternMethod>()) {
                    cstring name = em->actualExternType->name;
                    if (name == "InternetChecksum" && em->method->name == "add") {
                        InternetChecksumTranslation(stmt, em);
                    } else if (name == "InternetChecksum" && em->method->name == "subtract") {
                        InternetChecksumTranslation(stmt, em);
                    }
                } else if (auto em = mi->to<P4::ExternFunction>()) {
                    cstring name = em->method->name;
                    if (name == "verify") {
                        if (ctxt->name ==
                                 structure->getBlockName(PSA::ProgramStructure::INGRESS_PARSER)) {
                            evaluateVerifyEnd(stmt, em, INGRESS, state->name);
                        } else if (ctxt->name ==
                                  structure->getBlockName(PSA::ProgramStructure::EGRESS_PARSER)) {
                            evaluateVerifyEnd(stmt, em, EGRESS, state->name);
                        }
                    }
                }
            } else if (auto mc = stmt->to<IR::AssignmentStatement>()) {
                auto member = mc->left->to<IR::Member>();
                auto expr = mc->right->to<IR::MethodCallExpression>();
                if (!expr || !member) continue;
                auto miExpr = P4::MethodInstance::resolve(expr, refMap, typeMap);
                if (auto emExpr = miExpr->to<P4::ExternMethod>()) {
                    cstring nameExpr = emExpr->actualExternType->name;
                    if (nameExpr == "InternetChecksum" && emExpr->method->name == "get") {
                        if (ctxt->name ==
                                structure->getBlockName(PSA::ProgramStructure::INGRESS_PARSER)) {
                            translateResidualGet(member, emExpr, stmt, INGRESS);
                        } else if (ctxt->name ==
                                structure->getBlockName(PSA::ProgramStructure::EGRESS_PARSER)) {
                            translateResidualGet(member, emExpr, stmt, EGRESS);
                        }
                    }
                }
            }
        }
    }

    void translateDeparserAdd(const IR::MethodCallStatement* addStmt) {
        auto mi = P4::MethodInstance::resolve(addStmt, refMap, typeMap);
        auto addMethod = mi->to<P4::ExternMethod>();
        auto declName = addMethod->object->to<IR::Declaration_Instance>()->name;
        auto sourceList = (*addMethod->expr->arguments)[0]->
                                           expression->to<IR::ListExpression>();
        for (auto source : sourceList->components) {
            fieldListMap[declName].push_back(source);
        }
        structure->_map.emplace(addStmt, nullptr);
    }

    IR::AssignmentStatement* translateDeparserGet(const IR::AssignmentStatement* getStmt) {
        auto dest = getStmt->left->to<IR::Member>();
        auto expr = getStmt->right->to<IR::MethodCallExpression>();
        auto mi = P4::MethodInstance::resolve(expr, refMap, typeMap);
        auto getMethod = mi->to<P4::ExternMethod>();
        auto declName = getMethod->object->to<IR::Declaration_Instance>()->name;
        if (fieldListMap.count(declName)) {
            auto decl = new IR::Declaration_Instance(declName,
                   new IR::Type_Name("Checksum"), new IR::Vector<IR::Argument>());
            auto list = new IR::ListExpression(fieldListMap.at(declName));
            auto typeArgs = new IR::Vector<IR::Type>({ list->type });
            auto args = new IR::Vector<IR::Argument>({ new IR::Argument(list) });
            structure->_map.emplace(getMethod->object->to<IR::Declaration_Instance>(),
                                                                        decl);
            auto update = new IR::MethodCallExpression(new IR::Member(
                           new IR::PathExpression(declName), "update"),
                           typeArgs, args);
            auto assignUpdate = new IR::AssignmentStatement(dest, update);
            return assignUpdate;
        }
        return nullptr;
    }

    bool isDeparserAdd(const IR::MethodCallStatement* methodStmt) {
        auto mi = P4::MethodInstance::resolve(methodStmt, refMap, typeMap);
        if (auto em = mi->to<P4::ExternMethod>()) {
            cstring name = em->actualExternType->name;
            if (name == "InternetChecksum" && em->method->name == "add") {
                return true;
            }
        }
        return false;
    }

    bool isDeparserGet(const IR::AssignmentStatement* assignStmt) {
        auto member = assignStmt->left->to<IR::Member>();
        auto expr = assignStmt->right->to<IR::MethodCallExpression>();
        if (!member || !expr) return false;
        auto mi = P4::MethodInstance::resolve(expr, refMap, typeMap);
        if (auto em = mi->to<P4::ExternMethod>()) {
            cstring name = em->actualExternType->name;
            if (name == "InternetChecksum" && em->method->name == "get") {
                return true;
            }
        }
        return false;
    }

    IR::BlockStatement*
    translateDeparserCsumIfBlock(const IR::BlockStatement* block) {
        if (!block) return nullptr;
        IR::BlockStatement* if_block = nullptr;
        for (auto stmt : block->components) {
            if (auto methodStmt = stmt->to<IR::MethodCallStatement>()) {
                if (isDeparserAdd(methodStmt)) {
                    translateDeparserAdd(methodStmt);
                }
            } else if (auto assignStmt = stmt->to<IR::AssignmentStatement>()) {
                if (isDeparserGet(assignStmt)) {
                    auto assignUpdate = translateDeparserGet(assignStmt);
                    if_block = new IR::BlockStatement({assignUpdate});
                }
            }
        }
        return if_block;
    }

    void postorder(const IR::P4Control* control) override {
        if (control->name != structure->getBlockName(ProgramStructure::INGRESS_DEPARSER) &&
            control->name != structure->getBlockName(ProgramStructure::EGRESS_DEPARSER))
            return;
        auto body = control->body->to<IR::BlockStatement>();

        for (auto comp : body->components) {
            // Find if the ifstatement is the condition for checksum
            if (auto ifStatement = comp->to<IR::IfStatement>()) {
                IR::BlockStatement* new_true_block = nullptr;
                IR::BlockStatement* new_false_block = nullptr;
                if (ifStatement->ifTrue) {
                    new_true_block = translateDeparserCsumIfBlock(
                                      ifStatement->ifTrue->to<IR::BlockStatement>());
                }
                if (ifStatement->ifFalse) {
                    new_false_block = translateDeparserCsumIfBlock(
                                       ifStatement->ifFalse->to<IR::BlockStatement>());
                }
                // Not a checksum if statement
                if (!new_true_block && !new_false_block) continue;
                auto ifStmt = new IR::IfStatement(ifStatement->condition, new_true_block,
                                                       new_false_block);
                structure->_map.emplace(ifStatement, ifStmt);
            } else if (auto methodStmt = comp->to<IR::MethodCallStatement>()) {
                if (isDeparserAdd(methodStmt)) {
                    translateDeparserAdd(methodStmt);
                }
            } else if (auto assignStmt = comp->to<IR::AssignmentStatement>()) {
                if (isDeparserGet(assignStmt)) {
                    auto assignUpdate = translateDeparserGet(assignStmt);
                    structure->_map.emplace(assignStmt, assignUpdate);
                }
            }
        }
        return;
    }

    void postorder(const IR::Member* node) override {
        ordered_set<cstring> toTranslateInControl = {PSA::INP_INTR_MD, PSA::OUT_INTR_MD};
        ordered_set<cstring> toTranslateInParser = {PSA::INP_INTR_MD, PSA::OUT_INTR_MD};
        auto gress = findOrigCtxt<IR::P4Control>();
        if (gress) {
            if (auto expr = node->expr->to<IR::PathExpression>()) {
                auto path = expr->path->to<IR::Path>();
                CHECK_NULL(path);
                auto it = toTranslateInControl.find(path->name);
                if (it != toTranslateInControl.end()) {
                    if (gress->name == structure->getBlockName(
                            PSA::ProgramStructure::INGRESS)) {
                        structure->pathsThread.emplace(node, INGRESS);
                        structure->pathsToDo.emplace(node, node);
                    } else if (gress->name == structure->getBlockName(
                            PSA::ProgramStructure::EGRESS)) {
                        structure->pathsThread.emplace(node, EGRESS);

                        structure->pathsToDo.emplace(node, node);
                    } else {
                        LOG1("WARNING: path " << node << " in "
                                        << gress->name.name << " is not translated");
                    }
                }
            } else if (auto expr = node->expr->to<IR::TypeNameExpression>()) {
                auto tn = expr->typeName->to<IR::Type_Name>();
                CHECK_NULL(tn);
                if (tn->path->name == "error") {
                    if (node->member == "NoError") {
                        structure->_map.emplace(node, new IR::Constant(IR::Type::Bits::get(16), 0));
                    } else if (node->member == "NoMatch") {
                        // no_tcam_match_err_en
                        structure->_map.emplace(node, new IR::Constant(IR::Type::Bits::get(16), 1));
                    } else if (node->member == "PacketTooShort") {
                        // partial_hdr_err_en
                        structure->_map.emplace(node, new IR::Constant(IR::Type::Bits::get(16), 2));
                    } else if (node->member == "StackOutOfBounds") {
                        // ctr_range_err_en
                        structure->_map.emplace(node, new IR::Constant(IR::Type::Bits::get(16), 4));
                    } else if (node->member == "ParserTimeout") {
                        // timeout_cycle_err_en
                        structure->_map.emplace(node, new IR::Constant(
                                                                    IR::Type::Bits::get(16), 16));
                    } else if (structure->error_to_constant.count(node->member)) {
                        // custom errors
                        structure->_map.emplace(node,
                                         new IR::Constant(IR::Type::Bits::get(16),
                                         1 << structure->error_to_constant.at(node->member)));
                   }
                }
                structure->typeNamesToDo.emplace(node, node);
            } else {
                LOG1("WARNING: Unable to translate path " << node);
            }
        }
        auto parser = findOrigCtxt<IR::P4Parser>();
        if (parser) {
            if (auto expr = node->expr->to<IR::PathExpression>()) {
                auto path = expr->path->to<IR::Path>();
                CHECK_NULL(path);
                auto it = toTranslateInParser.find(path->name);
                if (it == toTranslateInParser.end())
                    return;
                if (expr->type->is<IR::Type_Struct>()) {
                    structure->pathsToDo.emplace(node, node);
                } else {
                    LOG1("WARNING: metadata " << node << " is not converted");
                }
            } else if (auto expr = node->expr->to<IR::TypeNameExpression>()) {
                auto tn = expr->typeName->to<IR::Type_Name>();
                CHECK_NULL(tn);
                structure->typeNamesToDo.emplace(node, node);
            } else {
                LOG1("WARNING: Expression " << node << " is not converted");
            }
        }
    }

    void cvtActionSelector(const IR::Declaration_Instance* node) {
        auto declarations = new IR::IndexedVector<IR::Declaration>();

        // Create a new instance of Hash<W>
        auto typeArgs = new IR::Vector<IR::Type>();
        auto w = node->arguments->at(2)->expression->to<IR::Constant>()->asInt();
        typeArgs->push_back(IR::Type::Bits::get(w));

        auto args = new IR::Vector<IR::Argument>();
        args->push_back(node->arguments->at(0));

        auto specializedType = new IR::Type_Specialized(
            new IR::Type_Name("Hash"), typeArgs);
        auto hashName = cstring::make_unique(
            structure->unique_names, node->name + "__hash_", '_');
        structure->unique_names.insert(hashName);
        declarations->push_back(
            new IR::Declaration_Instance(hashName, specializedType, args));

        args = new IR::Vector<IR::Argument>();
        // size
        args->push_back(node->arguments->at(1));
        // hash
        args->push_back(new IR::Argument(new IR::PathExpression(hashName)));
        // selector_mode
        auto sel_mode = new IR::Member(
            new IR::TypeNameExpression("SelectorMode_t"), "FAIR");
        if (auto anno = node->annotations->getSingle("mode"_cs)) {
            auto mode = anno->expr.at(0)->to<IR::StringLiteral>();
            if (mode->value == "resilient")
                sel_mode->member = IR::ID("RESILIENT");
            else if (mode->value != "fair" && mode->value != "non_resilient")
                BUG("Selector mode provided for the selector is not supported", node);
        } else {
            LOG1("WARNING: No mode specified for the selector %s. Assuming fair" << node);
        }
        args->push_back(new IR::Argument(sel_mode));

        declarations->push_back(new IR::Declaration_Instance(
            node->srcInfo, node->name, node->annotations, node->type, args));
        structure->_map.emplace(node, declarations);
    }

    void cvtRandom(const IR::Declaration_Instance* node) {
        // TODO: check min/max of random, can only support 0 ~ 2**w-1 on tofino
        auto args = new IR::Vector<IR::Argument>();
        auto inst = new IR::Declaration_Instance(node->name, node->type, args);
        structure->_map.emplace(node, inst);
    }

    // top level PSA_Switch could instantiate anonymous instances of
    // IngressPipeline and EgressPipeline
    void process_package_instance(const IR::Type_Specialized* tp) {
        if (!tp->baseType->is<IR::Type_Name>())
            return;
        auto tn = tp->baseType->to<IR::Type_Name>();
        if (tn->path->name == "IngressPipeline") {
            structure->type_params[PSA::TYPE_IH] =
                    tp->arguments->at(0)->to<IR::Type_Name>()->path->name;
            structure->type_params[PSA::TYPE_IM] =
                    tp->arguments->at(1)->to<IR::Type_Name>()->path->name;
        } else if (tn->path->name == "EgressPipeline") {
            structure->type_params[PSA::TYPE_EH] =
                    tp->arguments->at(0)->to<IR::Type_Name>()->path->name;
            structure->type_params[PSA::TYPE_EM] =
                    tp->arguments->at(1)->to<IR::Type_Name>()->path->name;
        }
    }

    void postorder(const IR::Declaration_Instance* node) override {
        if (auto ts = node->type->to<IR::Type_Specialized>()) {
            if (!ts->baseType->is<IR::Type_Name>())
                return;
            auto name = ts->baseType->to<IR::Type_Name>();
            if (name->path->name == "PSA_Switch") {
                for (auto arg : *node->arguments) {
                    if (auto ctr = arg->expression->to<IR::ConstructorCallExpression>()) {
                        // skip PacketReplicationEngine and BufferingQueueingEngine
                        if (!ctr->constructedType->is<IR::Type_Specialized>())
                            continue;
                        auto type = ctr->constructedType->to<IR::Type_Specialized>();
                        process_package_instance(type);
                    }
                }
            } else if (name->path->name == "IngressPipeline" ||
                       name->path->name == "EgressPipeline") {
                process_package_instance(ts);
            } else if (name->path->name == "Random") {
                cvtRandom(node);
            }
        } else if (auto typeName = node->type->to<IR::Type_Name>()) {
            auto name = typeName->path->name;
            if (name == "ActionSelector") {
                cvtActionSelector(node);
            } else if (name == "InternetChecksum") {
                auto declName = node->name;
                auto decl = new IR::Declaration_Instance(declName,
                           new IR::Type_Name("Checksum"), new IR::Vector<IR::Argument>());
                structure->_map.emplace(node, decl);
            }
        }
    }

    void addExternMethodCall(cstring name, const IR::Node* node) {
        if (structure->methodcalls.count(name) == 0) {
            TranslationMap m;
            structure->methodcalls.emplace(name, m);
        }
        structure->methodcalls[name].emplace(node, node);
    }

    void postorder(const IR::MethodCallStatement *node) override {
        auto mce = node->methodCall->to<IR::MethodCallExpression>();
        CHECK_NULL(mce);
        auto mi = P4::MethodInstance::resolve(node, refMap, typeMap);
        if (auto em = mi->to<P4::ExternMethod>()) {
            cstring name = em->actualExternType->name;
            addExternMethodCall(name, node);
        } else if (mi->is<P4::ExternFunction>()) {
            LOG1("WARNING: extern function translation is not supported");
        }
    }

    void postorder(const IR::MethodCallExpression *node) override {
        auto mi = P4::MethodInstance::resolve(node, refMap, typeMap);
        if (auto em = mi->to<P4::ExternMethod>()) {
            cstring name = em->actualExternType->name;
            if (name == "Hash") {
                HashConverter conv(structure);
                structure->_map.emplace(node, node->apply(conv));
            } else if (name == "Random") {
                RandomConverter conv(structure);
                structure->_map.emplace(node, node->apply(conv));
            } else if (name == "Meter") {
                MeterConverter conv(structure);
                structure->_map.emplace(node, node->apply(conv));
            }
            addExternMethodCall(name, node);
        } else if (mi->is<P4::ExternFunction>()) {
            LOG1("WARNING: extern function translation is not supported");
        }
    }
};

/** \ingroup PortableSwitchTranslation */
class LoadTargetArchitecture : public Inspector {
    PSA::ProgramStructure* structure;

 public:
    explicit LoadTargetArchitecture(PSA::ProgramStructure* structure) : structure(structure) {
        setName("LoadTargetArchitecture");
        CHECK_NULL(structure);
    }

    void setup_metadata_map() {
        structure->addMetadata(INGRESS,
                               MetadataField{PSA::INP_INTR_MD, "ingress_port"_cs, 9},
                               MetadataField{"ig_intr_md"_cs, "ingress_port"_cs, 9});
        structure->addMetadata(EGRESS,
                               MetadataField{PSA::INP_INTR_MD, "ingress_port"_cs, 9},
                               MetadataField{"eg_intr_md"_cs, "ingress_port"_cs, 9});

        structure->addMetadata(INGRESS,
                               MetadataField{PSA::INP_INTR_MD, "ingress_timestamp"_cs, 48},
                               MetadataField{"ig_intr_md"_cs, "ingress_mac_tstamp"_cs, 48});
        structure->addMetadata(INGRESS,
                               MetadataField{PSA::INP_INTR_MD, "parser_error"_cs, 16},
                               MetadataField{"ig_intr_md_from_parser"_cs, "parser_err"_cs, 16});
        structure->addMetadata(INGRESS,
                               MetadataField{PSA::OUT_INTR_MD, "class_of_service"_cs, 3},
                               MetadataField{"ig_intr_md_for_tm"_cs, "ingress_cos"_cs, 3});
        structure->addMetadata(INGRESS,
                               MetadataField{PSA::OUT_INTR_MD, "drop"_cs, 1},
                               MetadataField{COMPILER_META, "drop"_cs, 1});
        structure->addMetadata(INGRESS, MetadataField{PSA::OUT_INTR_MD, "multicast_group"_cs, 16},
                               MetadataField{"ig_intr_md_for_tm"_cs, "mcast_grp_a"_cs, 16});
        structure->addMetadata(INGRESS,
                               MetadataField{PSA::OUT_INTR_MD, "egress_port"_cs, 9},
                               MetadataField{"ig_intr_md_for_tm"_cs, "ucast_egress_port"_cs, 9});
        structure->addMetadata(INGRESS, MetadataField{PSA::OUT_INTR_MD, "resubmit"_cs, 1},
                               MetadataField{COMPILER_META, "resubmit"_cs, 1});
        structure->addMetadata(INGRESS, MetadataField{PSA::OUT_INTR_MD, "clone"_cs, 1},
                               MetadataField{COMPILER_META, "clone_i2e"_cs, 1});
        structure->addMetadata(INGRESS,
                               MetadataField{PSA::INP_INTR_MD, "packet_path"_cs, 0},
                               MetadataField{COMPILER_META, "packet_path"_cs, 0});

        structure->addMetadata(EGRESS,
                               MetadataField{PSA::INP_INTR_MD, "egress_port"_cs, 9},
                               MetadataField{"eg_intr_md"_cs, "egress_port"_cs, 9});
        structure->addMetadata(EGRESS,
                               MetadataField{PSA::INP_INTR_MD, "egress_timestamp"_cs, 48},
                               MetadataField{"eg_intr_md_for_dprsr"_cs,
                                             "egress_global_tstamp"_cs, 48});
        structure->addMetadata(EGRESS,
                               MetadataField{PSA::INP_INTR_MD, "parser_error"_cs, 16},
                               MetadataField{"eg_intr_md_from_parser"_cs, "parser_err"_cs, 16});
        structure->addMetadata(EGRESS,
                               MetadataField{PSA::OUT_INTR_MD, "drop"_cs, 1},
                               MetadataField{COMPILER_META, "drop"_cs, 1});
        structure->addMetadata(EGRESS, MetadataField{PSA::OUT_INTR_MD, "clone"_cs, 1},
                               MetadataField{COMPILER_META, "clone_e2e"_cs, 1});
        structure->addMetadata(EGRESS,
                               MetadataField{PSA::INP_INTR_MD, "packet_path"_cs, 0},
                               MetadataField{COMPILER_META, "packet_path"_cs, 0});
        structure->addMetadata(MetadataField{PSA::OUT_INTR_MD, "clone_session_id"_cs, 10},
                               MetadataField{COMPILER_META, "mirror_id"_cs, 10});
    }

    void analyzeTofinoModel() {
        for (auto decl : structure->targetTypes) {
            if (auto v = decl->to<IR::Type_Enum>()) {
                structure->enums.emplace(v->name, v);
            } else if (auto v = decl->to<IR::Type_SerEnum>()) {
                structure->ser_enums.emplace(v->name, v);
            } else if (auto v = decl->to<IR::Type_Error>()) {
                for (auto mem : v->members) {
                    structure->errors.emplace(mem->name);
                }
            }
        }
    }

    void postorder(const IR::P4Program*) override {
        setup_metadata_map();
        char* drvP4IncludePath = getenv("P4C_16_INCLUDE_PATH");
        std::filesystem::path path(drvP4IncludePath ? drvP4IncludePath : p4includePath);
        LOG1("path " << p4includePath << " " << drvP4IncludePath);
        char tempPath[PATH_MAX];
        snprintf(tempPath, PATH_MAX-1, "/tmp/arch_XXXXXX.p4");
        std::vector<const char *>filenames;
        if (Device::currentDevice() == Device::TOFINO) {
            filenames.push_back("tofino1_specs.p4");
            filenames.push_back("tofino1_base.p4");
        }
#if HAVE_JBAY
        else {
            filenames.push_back("tofino2_specs.p4");
            filenames.push_back("tofino2_base.p4");
        }
#endif  // HAVE_JBAY
        filenames.push_back("tofino/stratum.p4");
        filenames.push_back("tofino/p4_14_prim.p4");

        // create a temporary file. The safe method is to create and open the file
        // in exclusive mode. Since we can only open a C++ stream with a hack, we'll close
        // the file and then open it again as an ofstream.
        auto fd = mkstemps(tempPath, 3);
        if (fd < 0) {
            error("Error mkstemp(%1%): %2%", tempPath, strerror(errno));
            return;
        }
        // close the file descriptor and open as stream
        close(fd);
        std::ofstream result(tempPath, std::ios::out);
        if (!result.is_open()) {
            error("Failed to open arch include file %1%", tempPath);
            return;
        }
        for (auto f : filenames) {
            std::filesystem::path fPath = path / f;
            std::ifstream inFile(fPath, std::ios::in);
            if (inFile.is_open()) {
                result << inFile.rdbuf();
                inFile.close();
            } else {
                error("Failed to open architecture include file %1%", fPath);
                result.close();
                unlink(tempPath);
                return;
            }
        }
        result.close();
        structure->include(cstring(tempPath), &structure->targetTypes);
        unlink(tempPath);

        analyzeTofinoModel();
    }
};

/**
 * \ingroup PortableSwitchTranslation
 * Remap paths, member expressions, and type names according to the mappings
 * specified in the given ProgramStructure.
 */
struct ConvertNames : public PassManager {
    explicit ConvertNames(ProgramStructure *structure, P4::ReferenceMap *refMap,
            P4::TypeMap *typeMap) {
        addPasses({new BFN::PSA::PathExpressionConverter(structure),
                   new BFN::PSA::TypeNameExpressionConverter(structure),
                   new BFN::PSA::TypeNameConverter(structure),
                   new BFN::PSA::MeterColorTypeTo8Bits(),
                   new P4::ClearTypeMap(typeMap),
                   new P4::TypeChecking(refMap, typeMap, true),
                   new P4::EliminateSerEnums(typeMap)});
    }
};

/** \ingroup PortableSwitchTranslation */
struct CreateErrorStates : public Transform {
    PSA::ProgramStructure* structure;
    std::map<gress_t, std::set<const IR::ParserState*>> newStates;

 public:
    explicit CreateErrorStates(PSA::ProgramStructure* structure)
        : structure(structure) { }

    const IR::ParserState* create_error_state(const IR::ParserState* origState,
                                              int error_idx,
                                              cstring metaParam) {
        auto statements = new IR::IndexedVector<IR::StatOrDecl>();
        auto parser_err = new IR::Member(new IR::PathExpression(metaParam),
                                         "parser_err");
        auto lhs = new IR::Slice(parser_err, error_idx, error_idx);
        auto assign = new IR::AssignmentStatement(lhs, new IR::Constant(
                                                           IR::Type::Bits::get(1), 1));
        statements->push_back(assign);
        auto newStateName = IR::ID(cstring("__" + origState->name + "_error_" +
                                           std::to_string(error_idx)));
        auto selectExpression = new IR::PathExpression(IR::ID("accept"));
        auto newState = new IR::ParserState(newStateName, *statements, selectExpression);
        return newState;
    }

    const IR::Node* preorder(IR::ParserState* state) override {
        auto parser = findContext<IR::BFN::TnaParser>();
        if (structure->state_to_verify.count(parser->thread) &&
           (structure->state_to_verify.at(parser->thread).count(state->name))) {
            auto stmt = structure->state_to_verify.at(parser->thread).at(state->name);
            auto expr = (*stmt->arguments)[0]->expression;
            auto err = (*stmt->arguments)[1]->expression->to<IR::Member>();
            Pattern::Match<IR::Member> member;
            Pattern::Match<IR::Constant> constant;
            cstring metaParam = parser->thread == INGRESS ? "ig_intr_md_from_prsr"_cs :
                                                   "eg_intr_md_from_prsr"_cs;
            int error_idx = structure->error_to_constant.at(err->member);
            if (expr->is<IR::Equ>()) {
                (member == constant).match(expr);
            } else if (expr->is<IR::Neq>()) {
                (member != constant).match(expr);
            } else {
                error("Verify statement not supported %1%", stmt);
            }
            auto errorState = create_error_state(state, error_idx, metaParam);
            newStates[parser->thread].insert(errorState);
            auto stateName = IR::ID(cstring("__" + state->name + "_non_error"));
            auto statements = new IR::IndexedVector<IR::StatOrDecl>();
            auto origTransState = new IR::ParserState(stateName, *statements,
                                                       state->selectExpression);
            newStates[parser->thread].insert(origTransState);
            auto selectCases = new IR::Vector<IR::SelectCase>();
            selectCases->push_back(new IR::SelectCase(
                                   constant, new IR::PathExpression(
                                   expr->is<IR::Equ>() ? origTransState->name :
                                                         errorState->name)));
            selectCases->push_back(new IR::SelectCase(
                               new IR::DefaultExpression(IR::Type_Dontcare::get()),
                               new IR::PathExpression(expr->is<IR::Equ>() ? errorState->name :
                                                       origTransState->name)));
            IR::Vector<IR::Expression> selectOn;
            selectOn.push_back(member);
            auto selectExpression = new IR::SelectExpression(new IR::ListExpression(selectOn),
                                                                        *selectCases);
            state->selectExpression = selectExpression;
        }
        return state;
    }
};

/** \ingroup PortableSwitchTranslation */
struct AddParserStates : public Transform {
    const CreateErrorStates* createErrorStates;
 public:
    explicit AddParserStates(const CreateErrorStates* createErrorStates) :
                             createErrorStates(createErrorStates) { }
    const IR::BFN::TnaParser* preorder(IR::BFN::TnaParser* parser) override {
        if (!createErrorStates->newStates.count(parser->thread)) return parser;
        for (auto newState : createErrorStates->newStates.at(parser->thread)) {
            parser->states.push_back(newState);
        }
        return parser;
    }
};

/** \ingroup PortableSwitchTranslation */
struct RewriteParserVerify : public PassManager {
    explicit RewriteParserVerify(PSA::ProgramStructure* structure) {
        auto createErrorStates = new BFN::PSA::CreateErrorStates(structure);
        addPasses({ createErrorStates,
                    new BFN::PSA::AddParserStates(createErrorStates),
                   });
    }
};

}  // namespace PSA

PortableSwitchTranslation::PortableSwitchTranslation(
        P4::ReferenceMap* refMap, P4::TypeMap* typeMap, BFN_Options& options) {
    setName("Translation");
    addDebugHook(options.getDebugHook());
    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto structure = new PSA::ProgramStructure;

    addPasses({
        new P4::ValidateTableProperties({"psa_implementation"_cs, "psa_direct_counter"_cs,
                                         "psa_direct_meter"_cs, "psa_idle_timeout"_cs,
                                         "psa_empty_group_action"_cs}),
        new P4::ConvertEnums(typeMap, new PSA::PacketPathTo8Bits),
        new P4::CopyStructures(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
        evaluator,
        new VisitFunctor(
            [structure, evaluator]() { structure->toplevel = evaluator->getToplevelBlock(); }),
        new TranslationFirst(),
        new PSA::LoadTargetArchitecture(structure),
        new PSA::AnalyzeProgram(structure, refMap, typeMap),
        new PSA::TranslateProgram(structure, refMap, typeMap),
        new GenerateTofinoProgram(structure),
        new PSA::ConvertNames(structure, refMap, typeMap),
        new BFN::AddIntrinsicMetadata(refMap, typeMap),
        new PSA::RewritePacketPath(refMap, typeMap, structure),
        new PSA::RewriteParserVerify(structure),
        new AddPsaBridgeMetadata(refMap, typeMap, structure),
        new BFN::RewriteActionSelector(refMap, typeMap),
        new BFN::FindArchitecture(),
        new TranslationLast(),
        new P4::ClearTypeMap(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
    });
}

}  // namespace BFN
