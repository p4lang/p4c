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

#include "resubmit.h"

#include "bf-p4c/device.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
#include "frontends/p4/methodInstance.h"
#include "bf-p4c/arch/intrinsic_metadata.h"

namespace BFN {
namespace {

/**
 * Analyze the Resubmit `emit` method within the deparser block,
 * and try to extract the source field list.
 *
 * @param statement  The `emit` method to analyze
 * @return a ResubmitSource vector containing the source fields used in the resubmit,
 * or std::nullopt if the resubmit code was invalid.
 */

std::optional<std::pair<cstring, ResubmitSources*>>
analyzeResubmitStatement(const IR::MethodCallStatement* statement) {
    auto methodCall = statement->methodCall->to<IR::MethodCallExpression>();
    if (!methodCall) {
        return std::nullopt;
    }
    auto member = methodCall->method->to<IR::Member>();
    if (!member || member->member != "emit") {
        return std::nullopt;
    }
    if (methodCall->arguments->size() != 1) {
        ::warning("Expected 1 arguments for resubmit.%1% statement: %1%", member->member);
        return std::nullopt;
    }
    const IR::Expression* expression = methodCall->arguments->at(0)->expression;
    if (expression->is<IR::StructExpression>()) {
        LOG2("resubmit emits struct initializer expression " << expression);
        const IR::StructExpression* fieldList = nullptr;
        {
            fieldList = expression->to<IR::StructExpression>();
            if (!fieldList) {
                ::warning("Expected field list: %1%", methodCall);
                return std::nullopt;
            }
        }
        auto type = methodCall->typeArguments->at(0);
        auto typeName = type->to<IR::Type_Name>();
        auto* sources = new ResubmitSources;
        for (auto* field : fieldList->components) {
            LOG2("resubmit would include field: " << field);
            if (!field->expression->is<IR::Concat>() &&
                !field->expression->is<IR::Cast>() &&
                !field->expression->is<IR::Constant>() &&
                !field->expression->is<IR::Member>()) {
                ::warning("Unexpected field: %1%", field);
                return std::nullopt; }
            sources->push_back(field->expression);
        }
        return std::make_pair(typeName->path->name.name, sources);
    }
    return std::nullopt;
}

std::optional<const IR::Constant*>
checkResubmitIfStatement(const IR::IfStatement* ifStatement) {
    if (!ifStatement)
        return std::nullopt;
    auto* equalExpr = ifStatement->condition->to<IR::Equ>();
    if (!equalExpr) {
        ::warning("Expected comparing resubmit_type with constant: %1%", ifStatement->condition);
        return std::nullopt;
    }
    auto* constant = equalExpr->right->to<IR::Constant>();
    if (!constant) {
        ::warning("Expected comparing resubmit_type with constant: %1%", equalExpr->right);
        return std::nullopt;
    }

    auto* member = equalExpr->left->to<IR::Member>();
    if (!member || member->member != "resubmit_type") {
        ::warning("Expected comparing resubmit_type with constant: %1%", ifStatement->condition);
        return std::nullopt;
    }
    if (!ifStatement->ifTrue || ifStatement->ifFalse) {
        ::warning("Expected an `if` with no `else`: %1%", ifStatement);
        return std::nullopt;
    }
    auto* method = ifStatement->ifTrue->to<IR::MethodCallStatement>();
    if (!method) {
        ::warning("Expected a single method call statement: %1%", method);
        return std::nullopt;
    }
    return constant;
}

class FindResubmit : public DeparserInspector {
    bool preorder(const IR::MethodCallStatement* node) override {
        auto mi = P4::MethodInstance::resolve(node, refMap, typeMap);
        if (auto* em = mi->to<P4::ExternMethod>()) {
            cstring externName = em->actualExternType->name;
            if (externName != "Resubmit") {
                return false;
            }
        }
        auto ifStatement = findContext<IR::IfStatement>();
        if (!ifStatement) {
            ::warning("Expected Resubmit to be used within an If statement");
        }
        auto resubmit_type = checkResubmitIfStatement(ifStatement);
        if (!resubmit_type) {
            return false;
        }

        auto resubmit = analyzeResubmitStatement(node);
        if (resubmit)
            extracts.emplace((*resubmit_type)->asInt(), *resubmit);
        return false;
    }

 public:
    FindResubmit(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) { }

    ResubmitExtracts extracts;

 private:
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
};

/// resubmit parser is only generated for p4-14 based programs
class AddResubmitParser : public Transform {
    P4::CloneExpressions cloner;

 public:
    explicit AddResubmitParser(const ResubmitExtracts* extracts)
        : extracts(extracts) { }

    const IR::Node* preorder(IR::ParserState* state) override {
        auto parser = findOrigCtxt<IR::BFN::TnaParser>();
        BUG_CHECK(parser != nullptr, "ParserState %1% is not in TnaParser ", state->name);
        prune();
        if (state->name == "__resubmit") {
            if (extracts->size() == 0)
                return createEmptyResubmitState(parser);
            return updateResubmitMetadataState(parser, state);
        }
        return state;
    }

    const IR::Node* createEmptyResubmitState(const IR::BFN::TnaParser* parser) {
        // Add a state that skips over any padding between the phase 0 data and the
        // beginning of the packet.
        const auto bitSkip = Device::pardeSpec().bitResubmitSize();
        auto packetInParam = parser->tnaParams.at("pkt"_cs);
        auto *skipToPacketState =
            createGeneratedParserState("resubmit"_cs, {
                createAdvanceCall(packetInParam, bitSkip)
            }, new IR::PathExpression(IR::ID("__skip_to_packet")));
        return skipToPacketState;
    }

    IR::Node* updateResubmitMetadataState(const IR::BFN::TnaParser* parser,
            IR::ParserState* state) {
        LOG3(" update state " << state << " with these extracts: ");
        auto states = new IR::IndexedVector<IR::ParserState>();
        auto selectCases = new IR::Vector<IR::SelectCase>();
        for (auto e : *extracts) {
            LOG3(" - " << e.first << " " << e.second.first << " " << e.second.second);
            auto newState = createResubmitState(parser, e.first,
                        e.second.first, e.second.second);
            states->push_back(newState);
            auto selectCase = createSelectCase(8, e.first, 0x07, newState);
            selectCases->push_back(selectCase);
        }

        IR::Vector<IR::Expression> selectOn = {
            createLookaheadExpr(parser->tnaParams.at("pkt"_cs), 8)
        };

        auto* resubmitState =
            createGeneratedParserState(
                "resubmit"_cs, { },
                new IR::SelectExpression(new IR::ListExpression(selectOn), *selectCases));
        states->push_back(resubmitState);
        return states;
    }

    const IR::ParserState* createResubmitState(const IR::BFN::TnaParser* parser,
            unsigned idx, cstring header, const ResubmitSources* sources) {
        auto statements = new IR::IndexedVector<IR::StatOrDecl>();

        /**
         * T tmp;
         * pkt.extract(tmp);
         */
        cstring tmp = "__resubmit_tmp_"_cs + std::to_string(idx);
        auto decl = new IR::Declaration_Variable(IR::ID(tmp), new IR::Type_Name(header));
        statements->push_back(decl);
        statements->push_back(createExtractCall(parser->tnaParams.at("pkt"_cs), header,
                    new IR::PathExpression(tmp)));

        /**
         * copy extract tmp header to metadata;
         */
        unsigned field_idx = 0;
        unsigned skip_idx = 1;
        // skip compiler generated field
        for (auto s : *sources) {
            if (field_idx < skip_idx) {
                field_idx++;
                continue; }
            auto field = "__field_" + std::to_string(field_idx++);
            statements->push_back(createSetMetadata(s->apply(cloner), tmp, field));
        }

        cstring name = "resubmit_"_cs + std::to_string(idx);
        auto select = new IR::PathExpression(IR::ID("__skip_to_packet"));
        auto newStateName = IR::ID(cstring("__") + name);
        auto *newState = new IR::ParserState(newStateName, *statements, select);
        newState->annotations = newState->annotations
            ->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                 new IR::StringLiteral(cstring(cstring("$") + name)));
        return newState;
    }

 private:
  const ResubmitExtracts* extracts;
};

}  // namespace

FixupResubmitMetadata::FixupResubmitMetadata(
        P4::ReferenceMap *refMap,
        P4::TypeMap *typeMap) {
    auto findResubmit = new FindResubmit(refMap, typeMap);
    addPasses({
        findResubmit,
        new AddResubmitParser(&findResubmit->extracts)
    });
}

}  // namespace BFN
