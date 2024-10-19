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

#include "bridge_metadata.h"

#include <boost/range/adaptor/sliced.hpp>

#include "bf-p4c/arch/collect_bridged_fields.h"
#include "bf-p4c/arch/intrinsic_metadata.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace BFN {
namespace {

struct CollectSpecialPrimitives : public Inspector {
    ordered_set<const IR::ExitStatement *> exit;
    ordered_set<const IR::MethodCallStatement *> bypass_egress;
    ordered_set<const IR::MethodCallStatement *> clone;

 public:
    CollectSpecialPrimitives() {}

    bool preorder(const IR::BFN::TnaControl *control) override {
        if (control->thread != INGRESS) return false;
        return true;
    }

    void postorder(const IR::ExitStatement *e) override { exit.insert(e); }

    void postorder(const IR::MethodCallStatement *node) override {
        auto *call = node->methodCall;
        auto *pa = call->method->to<IR::PathExpression>();
        if (!pa) return;
        if (pa->path->name == "bypass_egress")
            bypass_egress.insert(node);
        else if (pa->path->name == "clone3" || pa->path->name == "clone" ||
                 pa->path->name == "clone_preserving_field_list")
            clone.insert(node);
    }

    bool has_exit() { return !exit.empty(); }
    bool has_bypass_egress() { return !bypass_egress.empty(); }
    bool has_clone() { return !clone.empty(); }
};

struct BridgeIngressToEgress : public Transform {
    BridgeIngressToEgress(const std::set<FieldRef> &fieldsToBridge,
                          const ordered_map<FieldRef, BridgedFieldInfo> &fieldInfo,
                          CollectSpecialPrimitives *prim, P4::ReferenceMap *refMap,
                          P4::TypeMap *typeMap)
        : refMap(refMap),
          typeMap(typeMap),
          fieldsToBridge(fieldsToBridge),
          fieldInfo(fieldInfo),
          special_primitives(prim) {}

    profile_t init_apply(const IR::Node *root) override {
        // Construct the bridged metadata header type.
        IR::IndexedVector<IR::StructField> fields;

        // We always need to bridge at least one byte of metadata; it's used to
        // indicate to the egress parser that it's dealing with bridged metadata
        // rather than mirrored data. (We could pack more information in there,
        // too, but we don't right now.)
        if (!fieldsToBridge.empty() || special_primitives->has_clone()) {
            fields.push_back(new IR::StructField(BRIDGED_MD_INDICATOR, IR::Type::Bits::get(8)));
        }

        IR::IndexedVector<IR::StructField> structFields;

        // The rest of the fields come from CollectBridgedFields.
        for (auto &bridgedField : fieldsToBridge) {
            cstring fieldName = bridgedField.first + bridgedField.second;
            fieldName = fieldName.replace('.', '_');
            bridgedHeaderFieldNames.emplace(bridgedField, fieldName);

            BUG_CHECK(fieldInfo.count(bridgedField), "No field info for bridged field %1%%2%?",
                      bridgedField.first, bridgedField.second);
            auto &info = fieldInfo.at(bridgedField);

            if (info.type->is<IR::Type_Stack>())
                P4C_UNIMPLEMENTED(
                    "Currently the compiler does not support bridging field %1% "
                    "of type stack.",
                    fieldName);
            structFields.push_back(new IR::StructField(fieldName, info.type));
        }
        // Only push the fields type if there are bridged fields.
        if (structFields.size() > 0) {
            if (LOGGING(3)) {
                LOG3("\tNumber of fields to bridge: " << fieldsToBridge.size());
                for (auto *f : structFields) LOG3("\t  Bridged field: " << f->name);
            }
            auto annot = new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
            auto bridgedMetaType = new IR::Type_Struct("fields", annot, structFields);

            fields.push_back(new IR::StructField(BRIDGED_MD_FIELD, annot, bridgedMetaType));
        }

        auto *layoutKind = new IR::StringLiteral(IR::ID("BRIDGED"));
        bridgedHeaderType = new IR::Type_Header(
            BRIDGED_MD_HEADER,
            new IR::Annotations({new IR::Annotation(IR::ID("layout"), {layoutKind})}), fields);
        LOG1("Bridged header type: " << bridgedHeaderType);

        // We'll inject a field containing the new header into the user metadata
        // struct. Figure out which type that is.
        forAllMatching<IR::BFN::TnaControl>(root, [&](const IR::BFN::TnaControl *control) {
            if (!metaStructName.isNullOrEmpty()) return;
            cstring p4ParamName = control->tnaParams.at("md"_cs);
            auto *params = control->type->getApplyParameters();
            auto *param = params->getParameter(p4ParamName);
            BUG_CHECK(param, "Couldn't find param %1% on control: %2%", p4ParamName, control);
            auto *paramType = typeMap->getType(param);
            BUG_CHECK(paramType, "Couldn't find type for: %1%", param);
            BUG_CHECK(paramType->is<IR::Type_StructLike>(),
                      "User metadata parameter type isn't structlike %2%: %1%", paramType,
                      typeid(*paramType).name());
            metaStructName = paramType->to<IR::Type_StructLike>()->name;
        });

        BUG_CHECK(!metaStructName.isNullOrEmpty(),
                  "Couldn't determine the P4 name of the TNA compiler generated  metadata "
                  "struct parameter 'md'");

        return Transform::init_apply(root);
    }

    IR::Type_StructLike *preorder(IR::Type_StructLike *type) override {
        prune();
        if (type->name != metaStructName) return type;

        LOG1("Will inject the new field " << BRIDGED_MD_HEADER);

        // Inject the new field. This will give us access to the bridged
        // metadata type everywhere in the program.
        type->fields.push_back(
            new IR::StructField(BRIDGED_MD, new IR::Type_Name(BRIDGED_MD_HEADER)));
        return type;
    }

    IR::ParserState *preorder(IR::ParserState *state) override {
        prune();
        if (state->name == "__ingress_metadata")
            return updateIngressMetadataState(state);
        else if (state->name == "__bridged_metadata")
            return updateBridgedMetadataState(state);
        return state;
    }

    IR::ParserState *updateIngressMetadataState(IR::ParserState *state) {
        auto *tnaContext = findContext<IR::BFN::TnaParser>();
        BUG_CHECK(tnaContext, "Parser state %1% not within translated parser?", state->name);
        if (tnaContext->thread != INGRESS) return state;

        auto metaParam = tnaContext->tnaParams.at("md"_cs);

        // Add "metadata.^bridged_metadata.^bridged_metadata_indicator = 0;".
        if (!fieldsToBridge.empty() || special_primitives->has_clone()) {
            state->components.push_back(
                createSetMetadata(metaParam, BRIDGED_MD, BRIDGED_MD_INDICATOR, 8, 0));
        }
        return state;
    }

    IR::ParserState *updateBridgedMetadataState(IR::ParserState *state) {
        auto *tnaContext = findContext<IR::BFN::TnaParser>();
        BUG_CHECK(tnaContext, "Parser state %1% not within translated parser?", state->name);
        if (tnaContext->thread != EGRESS) return state;

        auto packetInParam = tnaContext->tnaParams.at("pkt"_cs);

        if (fieldsToBridge.empty()) {
            // nothing to bridge, but clone is used, advance one byte.
            // otherwise, do not advance.
            state->components.push_back(
                createAdvanceCall(packetInParam, special_primitives->has_clone() ? 8 : 0));
            return state;
        }

        // Add "pkt.extract(md.^bridged_metadata);"
        auto metaParam = tnaContext->tnaParams.at("md"_cs);
        auto *member = new IR::Member(new IR::PathExpression(metaParam), IR::ID(BRIDGED_MD));
        auto extractCall = createExtractCall(packetInParam, BRIDGED_MD_HEADER, member);
        state->components.push_back(extractCall);

        // Copy all of the bridged fields to their final locations.
        for (auto &bridgedField : fieldsToBridge) {
            auto *member = new IR::Member(
                new IR::Member(new IR::PathExpression(metaParam), IR::ID(BRIDGED_MD)),
                BRIDGED_MD_FIELD);
            auto *fieldMember = new IR::Member(member, bridgedHeaderFieldNames.at(bridgedField));

            auto bridgedFieldParam = tnaContext->tnaParams.at(bridgedField.first);
            auto *bridgedMember = transformAllMatching<IR::PathExpression>(
                                      fieldInfo.at(bridgedField).refTemplate,
                                      [&](const IR::PathExpression *) {
                                          return new IR::PathExpression(bridgedFieldParam);
                                      })
                                      ->to<IR::Expression>();

            auto *assignment = new IR::AssignmentStatement(bridgedMember, fieldMember);
            state->components.push_back(assignment);
        }

        return state;
    }

    IR::BFN::TnaControl *postorder(IR::BFN::TnaControl *control) override {
        if (control->thread != INGRESS) return control;
        // Inject code to copy all of the bridged fields into the bridged
        // metadata header. This will run at the very end of the ingress
        // control, so it'll get the final values of the fields.
        if (special_primitives->has_bypass_egress()) {
            auto stmt = updateIngressControl(control);
            auto condExprPath = new IR::Member(
                new IR::PathExpression(new IR::Path("ig_intr_md_for_tm")), "bypass_egress"_cs);
            auto condExpr = new IR::Equ(condExprPath, new IR::Constant(IR::Type::Bits::get(1), 0));
            auto cond = new IR::IfStatement(condExpr, new IR::BlockStatement(*stmt), nullptr);

            auto *body = control->body->clone();
            body->components.push_back(cond);
            control->body = body;
        } else {
            auto *body = control->body->clone();
            auto stmt = updateIngressControl(control);
            body->components.append(*stmt);
            control->body = body;
        }
        return control;
    }

    IR::IndexedVector<IR::StatOrDecl> *updateIngressControl(const IR::BFN::TnaControl *control) {
        auto metaParam = control->tnaParams.at("md"_cs);

        // if (ig_intr_md_for_tm.egress == 1) {
        //    add_bridge_metadata;
        // }
        auto stmt = new IR::IndexedVector<IR::StatOrDecl>();
        if (!fieldsToBridge.empty() || special_primitives->has_clone()) {
            // Add "md.^bridged_metadata.setValid();"
            stmt->push_back(createSetValid(control->srcInfo, metaParam, BRIDGED_MD));
        }

        for (auto &bridgedField : fieldsToBridge) {
            auto *member = new IR::Member(
                new IR::Member(new IR::PathExpression(metaParam), IR::ID(BRIDGED_MD)),
                IR::ID(BRIDGED_MD_FIELD));
            auto *fieldMember = new IR::Member(member, bridgedHeaderFieldNames.at(bridgedField));

            BUG_CHECK(control->tnaParams.count(bridgedField.first) != 0, "nable to find %1%",
                      bridgedField.first);
            auto bridgedFieldParam = control->tnaParams.at(bridgedField.first);
            auto *bridgedMember = transformAllMatching<IR::PathExpression>(
                                      fieldInfo.at(bridgedField).refTemplate,
                                      [&](const IR::PathExpression *) {
                                          return new IR::PathExpression(bridgedFieldParam);
                                      })
                                      ->to<IR::Expression>();

            auto *assignment =
                new IR::AssignmentStatement(control->srcInfo, fieldMember, bridgedMember);
            stmt->push_back(assignment);
        }

        return stmt;
    }

    // calling 'exit' in ingress terminates ingress processing, which means
    // that if the bridge metadata header is validated at the end of the
    // ingress control block, the header will not be validated in the control
    // flow path that invoked 'exit'.
    IR::Node *postorder(IR::ExitStatement *exit) override {
        auto ctxt = findOrigCtxt<IR::BFN::TnaControl>();
        if (!ctxt) return exit;
        if (ctxt->thread != INGRESS) return exit;
        auto stmt = updateIngressControl(ctxt);
        stmt->push_back(exit);
        const IR::Node *parent = getContext()->node;
        if (parent->is<IR::BlockStatement>())
            return stmt;
        else if (parent->is<IR::IfStatement>())
            return new IR::BlockStatement(*stmt);
        else
            BUG("Unexpected parent type - %1% (expected is BlockStatement or IfStatement)",
                parent->toString());
    }

    IR::BFN::TnaDeparser *preorder(IR::BFN::TnaDeparser *deparser) override {
        prune();
        if (deparser->thread != INGRESS) return deparser;
        return updateIngressDeparser(deparser);
    }

    IR::BFN::TnaDeparser *updateIngressDeparser(IR::BFN::TnaDeparser *control) {
        // Add "pkt.emit(md.^bridged_metadata);" as the first statement in the
        // ingress deparser.
        auto packetOutParam = control->tnaParams.at("pkt"_cs);
        auto *method = new IR::Member(new IR::PathExpression(packetOutParam), IR::ID("emit"));

        auto metaParam = control->tnaParams.at("md"_cs);
        auto *member = new IR::Member(new IR::PathExpression(metaParam), IR::ID(BRIDGED_MD));
        auto *args = new IR::Vector<IR::Argument>({new IR::Argument(member)});
        auto *typeArgs = new IR::Vector<IR::Type>();
        typeArgs->push_back(new IR::Type_Name(IR::ID(BRIDGED_MD_HEADER)));
        auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);

        auto *body = control->body->clone();
        body->components.insert(body->components.begin(), new IR::MethodCallStatement(callExpr));
        control->body = body;
        return control;
    }

    IR::Node *preorder(IR::MethodCallStatement *node) override {
        auto *call = node->methodCall;
        auto *pa = call->method->to<IR::PathExpression>();
        if (!pa) return node;
        if (pa->path->name != "bypass_egress") return node;

        auto stmts = new IR::IndexedVector<IR::StatOrDecl>();
        auto control = findContext<IR::P4Control>();
        BUG_CHECK(control != nullptr, "bypass_egress() must be used in a control block");
        auto meta = new IR::PathExpression("ig_intr_md_for_tm"_cs);
        auto flag = new IR::Member(meta, "bypass_egress"_cs);
        auto ftype = IR::Type::Bits::get(1);
        auto assign = new IR::AssignmentStatement(flag, new IR::Constant(ftype, 1));
        stmts->push_back(assign);
        return stmts;
    }

    const IR::P4Program *postorder(IR::P4Program *program) override {
        LOG4("Injecting declaration for bridge metadata type: " << bridgedHeaderType);
        program->objects.insert(program->objects.begin(), bridgedHeaderType);
        return program;
    }

    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    const std::set<FieldRef> &fieldsToBridge;
    const ordered_map<FieldRef, BridgedFieldInfo> &fieldInfo;

    CollectSpecialPrimitives *special_primitives;

    ordered_map<FieldRef, cstring> bridgedHeaderFieldNames;
    const IR::Type_Header *bridgedHeaderType = nullptr;
    cstring metaStructName;
};

}  // namespace

AddTnaBridgeMetadata::AddTnaBridgeMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                           bool &use_bridge_metadata) {
    auto *collectSpecialPrimitives = new CollectSpecialPrimitives();
    auto *collectBridgedFields = new CollectBridgedFields(refMap, typeMap);
    auto *bridgeIngressToEgress = new BridgeIngressToEgress(
        collectBridgedFields->fieldsToBridge, collectBridgedFields->fieldInfo,
        collectSpecialPrimitives, refMap, typeMap);
    addPasses({
        collectSpecialPrimitives,
        collectBridgedFields,
        new VisitFunctor([&use_bridge_metadata, collectBridgedFields]() mutable {
            use_bridge_metadata = !collectBridgedFields->fieldsToBridge.empty();
        }),
        bridgeIngressToEgress,
        new P4::ClearTypeMap(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
    });
}

}  // namespace BFN
