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

#include "bf-p4c/arch/intrinsic_metadata.h"

#include "ir/ir.h"

namespace BFN {

const IR::ParserState *convertStartStateToNormalState(const IR::ParserState *state,
                                                      cstring newName) {
    auto name = IR::ID(cstring("__") + newName);
    auto annotations = state->annotations->addAnnotationIfNew(
        IR::Annotation::nameAnnotation, new IR::StringLiteral(IR::ParserState::start));
    auto newState = new IR::ParserState(state->srcInfo, name, annotations, state->components,
                                        state->selectExpression);
    return newState;
}

struct RewritePathToStartState : public Modifier {
    cstring newName;
    explicit RewritePathToStartState(cstring name) : newName(name) {}

    bool preorder(IR::Path *path) override {
        if (path->name == "start") path->name = newName;
        return false;
    }
};

/// Rename the start state of the given parser and return it. This will
/// leave the parser without a start state, so the caller must create a new
/// one.
const IR::ParserState *convertStartStateToNormalState(IR::P4Parser *parser, cstring newName) {
    auto *origStartState = parser->getDeclByName(IR::ParserState::start);
    auto origStartStateIt = std::find(parser->states.begin(), parser->states.end(), origStartState);
    BUG_CHECK(origStartStateIt != parser->states.end(), "Couldn't find the original start state?");
    parser->states.erase(origStartStateIt);

    auto *newState = convertStartStateToNormalState(origStartState->to<IR::ParserState>(), newName);
    parser->states.push_back(newState);

    parser->states = *(parser->states.apply(RewritePathToStartState(cstring("__") + newName)));

    return newState;
}

const IR::ParserState *addNewStartState(cstring name, cstring nextState) {
    auto *startState =
        new IR::ParserState(IR::ParserState::start, new IR::PathExpression(nextState));
    startState->annotations = startState->annotations->addAnnotationIfNew(
        IR::Annotation::nameAnnotation, new IR::StringLiteral(cstring(cstring("$") + name)));
    return startState;
}

/// Add a new start state to the given parser, with a potentially
/// non-'start' name applied via an `@name` annotation.
void addNewStartState(IR::P4Parser *parser, cstring name, cstring nextState) {
    auto *startState = addNewStartState(name, nextState);
    parser->states.push_back(startState);
}

/// @return a parser state with a name that's distinct from the states in
/// the P4 program and an `@name` annotation with a '$' prefix. Downstream,
/// we search for certain '$' states and replace them with more generated
/// parser code.
const IR::ParserState *createGeneratedParserState(cstring name,
                                                  IR::IndexedVector<IR::StatOrDecl> &&statements,
                                                  const IR::Expression *selectExpression) {
    auto newStateName = IR::ID(cstring("__") + name);
    auto *newState = new IR::ParserState(newStateName, statements, selectExpression);
    newState->annotations = newState->annotations->addAnnotationIfNew(
        IR::Annotation::nameAnnotation, new IR::StringLiteral(cstring(cstring("$") + name)));
    return newState;
}

const IR::ParserState *createGeneratedParserState(cstring name,
                                                  IR::IndexedVector<IR::StatOrDecl> &&statements,
                                                  cstring nextState) {
    return createGeneratedParserState(name, std::move(statements),
                                      new IR::PathExpression(nextState));
}

/// @return an `advance()` call that advances by the given number of bits.
const IR::Statement *createAdvanceCall(cstring pkt, int bits) {
    auto *method = new IR::Member(new IR::PathExpression(IR::ID(pkt)), IR::ID("advance"));
    auto *args = new IR::Vector<IR::Argument>(
        {new IR::Argument(new IR::Constant(IR::Type::Bits::get(32), bits))});
    auto *callExpr = new IR::MethodCallExpression(method, args);
    return new IR::MethodCallStatement(callExpr);
}

/// @return a SelectCase that checks for a constant value with some mask
/// applied.
const IR::SelectCase *createSelectCase(unsigned bitWidth, unsigned value, unsigned mask,
                                       const IR::ParserState *nextState) {
    auto *type = IR::Type::Bits::get(bitWidth);
    auto *valueExpr = new IR::Constant(type, value);
    auto *maskExpr = new IR::Constant(type, mask);
    auto *nextStateExpr = new IR::PathExpression(nextState->name);
    return new IR::SelectCase(new IR::Mask(valueExpr, maskExpr), nextStateExpr);
}

/// @return an assignment statement of the form `header.field = constant`.
const IR::Statement *createSetMetadata(cstring header, cstring field, int bitWidth, int constant) {
    auto *member = new IR::Member(new IR::PathExpression(header), IR::ID(field));
    auto *value = new IR::Constant(IR::Type::Bits::get(bitWidth), constant);
    return new IR::AssignmentStatement(member, value);
}

/// @return an assignment statement of the form `param.header.field = constant`.
const IR::Statement *createSetMetadata(cstring param, cstring header, cstring field, int bitWidth,
                                       int constant) {
    auto *member = new IR::Member(new IR::Member(new IR::PathExpression(param), IR::ID(header)),
                                  IR::ID(field));
    auto *value = new IR::Constant(IR::Type::Bits::get(bitWidth), constant);
    return new IR::AssignmentStatement(member, value);
}

/// @return an assignment statement of the form `dest = header.field`.
const IR::Statement *createSetMetadata(const IR::Expression *dest, cstring header, cstring field) {
    auto *member = new IR::Member(new IR::PathExpression(IR::ID(header)), IR::ID(field));
    return new IR::AssignmentStatement(dest, member);
}

/// @return an assignment statement of the form `header.setValid()`.
const IR::Statement *createSetValid(const Util::SourceInfo &si, cstring header, cstring field) {
    auto *member = new IR::Member(new IR::PathExpression(header), IR::ID(field));
    auto *method = new IR::Member(member, IR::ID("setValid"));
    auto *args = new IR::Vector<IR::Argument>;
    auto *callExpr = new IR::MethodCallExpression(method, args);
    return new IR::MethodCallStatement(si, callExpr);
}

/// @return an `extract()` call that extracts the given header.
const IR::Statement *createExtractCall(cstring pkt, cstring type, cstring hdr) {
    auto *method = new IR::Member(new IR::PathExpression(pkt), IR::ID("extract"));
    auto *typeArgs = new IR::Vector<IR::Type>({new IR::Type_Name(IR::ID(type))});
    auto *args = new IR::Vector<IR::Argument>(
        {new IR::Argument(new IR::PathExpression(new IR::Type_Name(type), new IR::Path(hdr)))});
    auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);
    return new IR::MethodCallStatement(callExpr);
}

/// @return an `extract()` call that extracts the given header.
const IR::Statement *createExtractCall(cstring pkt, IR::Expression *member) {
    auto *method = new IR::Member(new IR::PathExpression(pkt), IR::ID("extract"));
    auto *typeArgs = new IR::Vector<IR::Type>({member->type});
    auto *args = new IR::Vector<IR::Argument>({new IR::Argument(member)});
    auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);
    return new IR::MethodCallStatement(callExpr);
}

/// @return an `extract()` call that extracts the given header.
const IR::Statement *createExtractCall(cstring pkt, cstring typeName, IR::Expression *member) {
    auto *method = new IR::Member(new IR::PathExpression(pkt), IR::ID("extract"));
    auto *args = new IR::Vector<IR::Argument>({new IR::Argument(member)});
    auto *typeArgs = new IR::Vector<IR::Type>();
    typeArgs->push_back(new IR::Type_Name(IR::ID(typeName)));
    auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);
    return new IR::MethodCallStatement(callExpr);
}

/// @return a lookahead expression for the given size of `bit<>` type.
const IR::Expression *createLookaheadExpr(cstring pkt, int bits) {
    auto *method = new IR::Member(new IR::PathExpression(pkt), IR::ID("lookahead"));
    auto *typeArgs = new IR::Vector<IR::Type>({IR::Type::Bits::get(bits)});
    auto *lookaheadExpr =
        new IR::MethodCallExpression(method, typeArgs, new IR::Vector<IR::Argument>);
    return lookaheadExpr;
}

}  // namespace BFN
