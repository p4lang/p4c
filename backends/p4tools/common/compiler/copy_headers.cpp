#include "backends/p4tools/common/compiler/copy_headers.h"

#include <vector>

#include <boost/multiprecision/number.hpp>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/parserControlFlow.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "lib/exceptions.h"
#include "lib/gmputil.h"
#include "lib/null.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"
#include "midend/copyStructures.h"

namespace P4Tools {

DoCopyHeaders::DoCopyHeaders(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
    : typeMap(typeMap), refMap(refMap) {
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);
    setName("DoCopyHeaders");
}

const IR::Node* DoCopyHeaders::postorder(IR::AssignmentStatement* statement) {
    // Do nothing if we are assigning from a list or struct expression.
    if (statement->right->is<IR::ListExpression>() ||
        statement->right->is<IR::StructExpression>()) {
        return statement;
    }

    const auto& srcInfo = statement->srcInfo;
    const auto* ltype = typeMap->getType(statement->left, false);
    if (!ltype) {
        return statement;
    }
    // Handle case where we are assigning to a header.
    if (const auto* header = ltype->to<IR::Type_Header>()) {
        // Build a "src.isValid()" call.
        const auto* isSrcValidCall = new IR::MethodCallExpression(
            srcInfo, IR::Type::Boolean::get(),
            new IR::Member(srcInfo, statement->right, IR::Type_Header::isValid),
            new IR::Vector<IR::Argument>());
        // Build a "dst.setValid()" call.
        const auto* setDstValidCall = new IR::MethodCallExpression(
            srcInfo, IR::Type::Void::get(),
            new IR::Member(srcInfo, statement->left, IR::Type_Header::setValid),
            new IR::Vector<IR::Argument>());

        // Build a "dst.setInvalid()" call.
        const auto* setDstInvalidCall = new IR::MethodCallExpression(
            srcInfo, IR::Type::Void::get(),
            new IR::Member(srcInfo, statement->left, IR::Type_Header::setInvalid),
            new IR::Vector<IR::Argument>());
        // Build the "then" branch: set the validity bit and copy the fields.
        IR::IndexedVector<IR::StatOrDecl> thenStmts;
        thenStmts.push_back(new IR::MethodCallStatement(srcInfo, setDstValidCall));
        for (const auto* field : header->fields) {
            const auto* left = new IR::Member(statement->left, field->name);
            const auto* right = new IR::Member(statement->right, field->name);
            thenStmts.push_back(new IR::AssignmentStatement(srcInfo, left, right));
        }

        // Build the "else" branch: clear the validity bit.
        auto* elseStmt = new IR::MethodCallStatement(srcInfo, setDstInvalidCall);

        // Finish up.
        return new IR::IfStatement(srcInfo, isSrcValidCall,
                                   new IR::BlockStatement(srcInfo, thenStmts), elseStmt);
    }

    // Handle case where we are assigning to a header stack.
    if (const auto* stack = ltype->to<IR::Type_Stack>()) {
        const auto* stackSize = stack->size->to<IR::Constant>();
        BUG_CHECK(stackSize && stackSize->value > 0, "Size of stack %s is not a positive constant",
                  ltype);
        BUG_CHECK(statement->right->is<IR::PathExpression>() ||
                      statement->right->is<IR::Member>() || statement->right->is<IR::ArrayIndex>(),
                  "%1%: Unexpected operation encountered while eliminating stack copying",
                  statement->right);

        // Copy each stack element.
        auto* result = new IR::IndexedVector<IR::StatOrDecl>();
        for (int i = 0; i < stackSize->asInt(); ++i) {
            const auto* left = new IR::ArrayIndex(statement->left, new IR::Constant(i));
            const auto* right = new IR::ArrayIndex(statement->right, new IR::Constant(i));
            result->push_back(new IR::AssignmentStatement(srcInfo, left, right));
        }

        return new IR::BlockStatement(srcInfo, *result);
    }

    return statement;
}

CopyHeaders::CopyHeaders(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                         P4::TypeChecking* typeChecking)
    : PassManager({}) {
    CHECK_NULL(typeMap);
    setName("CopyHeaders");

    if (typeChecking == nullptr) {
        typeChecking = new P4::TypeChecking(refMap, typeMap);
    }

    passes.emplace_back(typeChecking);
    passes.emplace_back(new P4::RemoveAliases(refMap, typeMap));
    passes.emplace_back(typeChecking);
    // The errorOnMethodCall argument in CopyStructures defaults to true. This means methods or
    // functions returning structs will be flagged as an error. Here, we set this to false to allow
    // such scenarios.
    passes.emplace_back(new P4::DoCopyStructures(typeMap, /* errorOnMethodCall = */ false));
    passes.emplace_back(typeChecking);
    passes.emplace_back(new DoCopyHeaders(refMap, typeMap));
    passes.emplace_back(new P4::RemoveParserControlFlow(refMap, typeMap));
}

}  // namespace P4Tools
