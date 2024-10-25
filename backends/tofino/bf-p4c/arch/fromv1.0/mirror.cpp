/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mirror.h"

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/intrinsic_metadata.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/lib/pad_alignment.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace BFN {
namespace {

/**
 * Analyze the `mirror.emit()` method within the deparser block,
 * and try to extract the field list.
 *
 * @param statement  The `emit()` method call to analyze.
 * @return a MirroredFieldList vector containing the mirrored fields, or
 * std::nullopt if the `emit()` call was invalid.
 */
std::optional<MirroredFieldList *> analyzeMirrorStatement(
    const IR::MethodCallStatement *statement) {
    auto methodCall = statement->methodCall->to<IR::MethodCallExpression>();
    if (!methodCall) {
        return std::nullopt;
    }
    auto member = methodCall->method->to<IR::Member>();
    if (!member || member->member != "emit") {
        return std::nullopt;
    }
    if (methodCall->arguments->size() != 2) {
        ::warning("Expected 2 arguments for mirror.%1% statement: %1%", member->member);
        return std::nullopt;
    }
    const IR::Expression *expression = methodCall->arguments->at(1)->expression;
    if (expression->is<IR::StructExpression>()) {
        const IR::StructExpression *fieldList = nullptr;
        {
            fieldList = expression->to<IR::StructExpression>();
            if (!fieldList) {
                ::warning("Expected field list: %1%", methodCall);
                return std::nullopt;
            }
        }
        auto *finalFieldList = new MirroredFieldList;
        for (auto *field : fieldList->components) {
            LOG2("mirror field list would include field: " << field);
            if (!field->expression->is<IR::Concat>() && !field->expression->is<IR::Cast>() &&
                !field->expression->is<IR::Constant>() && !field->expression->is<IR::Member>()) {
                ::warning("Unexpected field: %1% ", field);
                return std::nullopt;
            }
            finalFieldList->push_back(field->expression);
        }
        LOG3("found fieldList " << finalFieldList);
        return finalFieldList;
    }
    return std::nullopt;
}

std::optional<const IR::Constant *> checkMirrorIfStatement(const IR::IfStatement *ifStatement) {
    auto *equalExpr = ifStatement->condition->to<IR::Equ>();
    if (!equalExpr) {
        ::warning(
            "Expected comparison of mirror_type with constant: "
            "%1%",
            ifStatement->condition);
        return std::nullopt;
    }
    auto *constant = equalExpr->right->to<IR::Constant>();
    if (!constant) {
        ::warning(
            "Expected comparison of mirror_type with constant: "
            "%1%",
            equalExpr->right);
        return std::nullopt;
    }

    auto *member = equalExpr->left->to<IR::Member>();
    if (!member || member->member != "mirror_type") {
        ::warning(
            "Expected comparison of mirror_type with constant: "
            "%1%",
            ifStatement->condition);
        return std::nullopt;
    }
    if (!ifStatement->ifTrue || ifStatement->ifFalse) {
        ::warning("Expected an `if` with no `else`: %1%", ifStatement);
        return std::nullopt;
    }
    auto *method = ifStatement->ifTrue->to<IR::MethodCallStatement>();
    if (!method) {
        ::warning("Expected a single method call statement: %1%", method);
        return std::nullopt;
    }
    LOG3("found constant " << constant);
    return constant;
}

struct FindMirroredFieldLists : public Inspector {
    FindMirroredFieldLists(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}

    bool preorder(const IR::MethodCallStatement *call) override {
        auto deparser = findContext<IR::BFN::TnaDeparser>();
        if (!deparser) return call;

        auto gress = deparser->thread;

        auto *mi = P4::MethodInstance::resolve(call, refMap, typeMap);
        if (!mi->is<P4::ExternMethod>()) return false;

        auto *em = mi->to<P4::ExternMethod>();
        cstring externName = em->actualExternType->name;
        LOG5("externName: " << externName);
        if (externName != "Mirror") return false;

        auto *ifStatement = findContext<IR::IfStatement>();
        if (!ifStatement) {
            ::warning(
                "Expected mirror to be used within an `if` "
                "statement");
            return false;
        }
        auto mirrorIdxConstant = checkMirrorIfStatement(ifStatement);
        if (!mirrorIdxConstant) return false;

        auto fieldList = analyzeMirrorStatement(call);
        if (!fieldList) return false;

        auto mce = call->methodCall->to<IR::MethodCallExpression>();
        auto type = mce->typeArguments->at(0);
        auto typeName = type->to<IR::Type_Name>();
        cstring header = typeName->path->name.name;

        unsigned mirrorIdx = (*mirrorIdxConstant)->asInt();
        BUG_CHECK((mirrorIdx & ~0x07) == 0, "Mirror index is more than 3 bits?");
        fieldLists[std::make_tuple(gress, mirrorIdx, header)] = *fieldList;

        LOG1("found mirror list");
        return false;
    }

    MirroredFieldLists fieldLists;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
};

class AddMirroredFieldListParser : public Transform {
    P4::CloneExpressions cloner;

 public:
    explicit AddMirroredFieldListParser(const MirroredFieldLists *fieldLists,
                                        bool use_bridge_metadata)
        : fieldLists(fieldLists), use_bridge_metadata(use_bridge_metadata) {}

    const IR::BFN::TnaParser *preorder(IR::BFN::TnaParser *parser) override {
        if (parser->thread != EGRESS) prune();
        return parser;
    }

    const IR::Node *createEmptyMirrorState(const IR::BFN::TnaParser *parser, cstring nextState) {
        // Add a state that skips over compiler generated byte
        auto packetInParam = parser->tnaParams.at("pkt"_cs);
        auto *skipToPacketState = createGeneratedParserState(
            "mirrored"_cs, {createAdvanceCall(packetInParam, use_bridge_metadata ? 8 : 0)},
            new IR::PathExpression(IR::ID(nextState)));
        return skipToPacketState;
    }

    const IR::Node *preorder(IR::ParserState *state) override {
        auto parser = findOrigCtxt<IR::BFN::TnaParser>();
        if (!parser) return state;
        prune();
        if (state->name == "__mirrored") {
            // This is the '$mirrored' placeholder state. Generate code to extract
            auto selExpr = state->selectExpression;
            if (auto path = selExpr->to<IR::PathExpression>()) {
                auto nextState = path->path->name;
                if (fieldLists->size() == 0) return createEmptyMirrorState(parser, nextState);
                return addMirroredFieldListParser(parser, state, nextState);
            } else if (auto selcases = selExpr->to<IR::SelectExpression>()) {
                auto pathexpr = selcases->selectCases[0]->state;
                auto nextState = pathexpr->to<IR::PathExpression>()->path->name;
                return addMirroredFieldListParser(parser, state, nextState);
            }
        }
        return state;
    }

    const IR::ParserState *createMirrorState(const IR::BFN::TnaParser *parser, gress_t gress,
                                             unsigned cloneSrc, unsigned digestId, cstring header,
                                             const MirroredFieldList *fl, cstring nextState) {
        auto statements = new IR::IndexedVector<IR::StatOrDecl>();
        /**
         * T tmp;
         * pkt.extract(tmp);
         */
        auto tmp = cstring::to_cstring(gress) + "_mirror_tmp_" + std::to_string(digestId);
        auto decl = new IR::Declaration_Variable(IR::ID(tmp), new IR::Type_Name(header));
        auto packetInParam = parser->tnaParams.at("pkt"_cs);
        statements->push_back(decl);
        statements->push_back(createExtractCall(packetInParam, header, tmp));

        /**
         * copy extract tmp header to metadata;
         */
        unsigned field_idx = 0;
        unsigned skip_idx = 1;
        // skip compiler generated field
        for (auto s : *fl) {
            if (field_idx < skip_idx) {
                field_idx++;
                continue;
            }
            auto field = "__field_" + std::to_string(field_idx++);
            statements->push_back(createSetMetadata(s->apply(cloner), tmp, field));
        }

        statements->push_back(
            createSetMetadata(BFN::COMPILER_META, "clone_digest_id"_cs, 4, digestId));

        statements->push_back(createSetMetadata(BFN::COMPILER_META, "clone_src"_cs, 4, cloneSrc));

        // Create a state that extracts the fields in this field list.
        cstring name = "mirror_field_list_"_cs + cstring::to_cstring(gress) + "_"_cs +
                       cstring::to_cstring(digestId);

        auto select = new IR::PathExpression(IR::ID(nextState));
        auto newStateName = IR::ID(cstring("__") + name);
        auto *newState = new IR::ParserState(newStateName, *statements, select);
        newState->annotations = newState->annotations->addAnnotationIfNew(
            IR::Annotation::nameAnnotation, new IR::StringLiteral(cstring(cstring("$") + name)));
        return newState;
    }

    IR::Node *addMirroredFieldListParser(const IR::BFN::TnaParser *parser,
                                         const IR::ParserState *state, cstring nextState) {
        LOG3(" update state " << state << " with these fieldlist: ");
        auto states = new IR::IndexedVector<IR::ParserState>();
        auto selectCases = new IR::Vector<IR::SelectCase>();
        for (auto &fieldList : *fieldLists) {
            const gress_t gress = std::get<0>(fieldList.first);
            const auto digestId = std::get<1>(fieldList.first);
            const auto header = std::get<2>(fieldList.first);
            auto *fl = fieldList.second;
            LOG3(" - " << gress << " " << digestId << " " << fl);

            // Construct a `clone_src` value. The first bit indicates that the
            // packet is mirrored; the second bit indicates whether it originates
            // from ingress or egress. See `constants.p4` for the details of the
            // format.
            unsigned source = 1 << 0;
            if (gress == EGRESS) source |= 1 << 1;

            auto *newState =
                createMirrorState(parser, gress, source, digestId, header, fl, nextState);
            states->push_back(newState);

            const unsigned fieldListId = (source << 3) | digestId;
            auto selectCase = createSelectCase(8, fieldListId, 0x1f, newState);
            selectCases->push_back(selectCase);
        }

        auto packetInParam = parser->tnaParams.at("pkt"_cs);
        IR::Vector<IR::Expression> selectOn = {createLookaheadExpr(packetInParam, 8)};

        auto *mirrorState = createGeneratedParserState(
            "mirrored"_cs, {},
            new IR::SelectExpression(new IR::ListExpression(selectOn), *selectCases));
        states->push_back(mirrorState);
        return states;
    }

 private:
    const MirroredFieldLists *fieldLists;
    bool use_bridge_metadata;
};

}  // namespace

FixupMirrorMetadata::FixupMirrorMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                         bool use_bridge_metadata) {
    auto findMirror = new FindMirroredFieldLists(refMap, typeMap);
    addPasses(
        {findMirror, new AddMirroredFieldListParser(&findMirror->fieldLists, use_bridge_metadata)});
}

}  // namespace BFN
