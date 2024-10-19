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

#include "copy_header.h"

#include "boost/range/adaptor/reversed.hpp"
#include "midend/copyStructures.h"

namespace {

/**
 * \ingroup midend
 * \ingroup parde
 * \brief Pass that converts header assignments into field assignments.
 *
 * This class converts header assignment into assignment of individual
 * fields of the header and the validity bits. The purpose is to enable
 * local copy prop on the header fields which was not possible until
 * backend.
 *
 * The validity bits are represented with a special '$valid' name.
 */
class DoCopyHeaders : public Transform {
    P4::TypeMap *typeMap;
    // We expect the ParserState blocks to have already been simplified.
    // We will carry out a simple elimination of multiple assignments within a contiguous block.
    std::vector<const IR::Member *> prev_assignments;
    const IR::Node *replaceValidMethod(const IR::Member *mem, int value,
                                       const Util::SourceInfo &srcInfo);
#if ENABLE_P4C3251
    std::vector<bool> key_change;  // Keys in const entries` that need changing.
#endif

 public:
    explicit DoCopyHeaders(P4::TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoCopyHeaders");
    }
    // We clear history on entry & exit - TODO is this conservative enough?
    const IR::Node *preorder(IR::ParserState *scope) override {
        prev_assignments.clear();
        return scope;
    }
    const IR::Node *postorder(IR::ParserState *scope) override {
        prev_assignments.clear();
        return scope;
    }
    const IR::Node *preorder(IR::BlockStatement *scope) override {
        prev_assignments.clear();
        return scope;
    }
    const IR::Node *postorder(IR::BlockStatement *scope) override {
        prev_assignments.clear();
        return scope;
    }
    const IR::Node *preorder(IR::IfStatement *scope) override {
        prev_assignments.clear();
        return scope;
    }
    const IR::Node *postorder(IR::IfStatement *scope) override {
        prev_assignments.clear();
        return scope;
    }

    const IR::Node *postorder(IR::AssignmentStatement *statement) override;
    const IR::Node *postorder(IR::MethodCallStatement *mc) override;
#if ENABLE_P4C3251
    const IR::Node *postorder(IR::MethodCallExpression *mc) override;

    const IR::Node *preorder(IR::P4Table *tbl) override;
    const IR::Node *preorder(IR::Entry *ent) override;
#endif
};

const IR::Node *DoCopyHeaders::postorder(IR::AssignmentStatement *statement) {
    if (statement->right->to<IR::ListExpression>() || statement->right->to<IR::StructExpression>())
        return statement;
    auto ltype = typeMap->getType(statement->left, true);
    if (auto strct = ltype->to<IR::Type_Header>()) {
        auto retval = new IR::IndexedVector<IR::StatOrDecl>();
        // add copy valid bit
        auto validtype = IR::Type::Bits::get(1);
        auto dst = new IR::Member(statement->srcInfo, validtype, statement->left, "$valid");
        auto src = new IR::Member(statement->srcInfo, validtype, statement->right, "$valid");
        retval->push_back(new IR::AssignmentStatement(statement->srcInfo, dst, src));
        BUG_CHECK(statement->right->is<IR::PathExpression>() ||
                      statement->right->is<IR::Member>() || statement->right->is<IR::ArrayIndex>(),
                  "%1%: Unexpected operation when eliminating struct copying", statement->right);

        for (auto f : strct->fields) {
            auto right = new IR::Member(statement->right, f->name);
            auto left = new IR::Member(statement->left, f->name);
            retval->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
        }
        return new IR::BlockStatement(statement->srcInfo, *retval);
    } else if (auto stk = ltype->to<IR::Type_Stack>()) {
        auto size = stk->size->to<IR::Constant>();
        BUG_CHECK(size && size->value > 0, "stack %s size is not positive constant", ltype);
        auto retval = new IR::IndexedVector<IR::StatOrDecl>();
        BUG_CHECK(statement->right->is<IR::PathExpression>() ||
                      statement->right->is<IR::Member>() || statement->right->is<IR::ArrayIndex>(),
                  "%1%: Unexpected operation when eliminating stack copying", statement->right);
        for (int i = 0; i < size->asInt(); ++i) {
            auto right = new IR::ArrayIndex(statement->right, new IR::Constant(i));
            auto left = new IR::ArrayIndex(statement->left, new IR::Constant(i));
            retval->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
        }
        return new IR::BlockStatement(statement->srcInfo, *retval);
    }

    return statement;
}

bool isMethod_xValid(IR::ID member) {
    return member == IR::Type_Header::setValid || member == IR::Type_Header::setInvalid ||
           member == IR::Type_Header::isValid;
}

void bugcheck_xValid(const IR::MethodCallExpression *call, const IR::Expression *hdr) {
    BUG_CHECK(call->arguments->size() == 0, "Wrong number of arguments for method call: %1%", call);
    BUG_CHECK(hdr, "Method has no target: %1%", call);
    BUG_CHECK(hdr->type->is<IR::Type_Header>() || hdr->type->is<IR::Type_HeaderUnion>() ||
                  hdr->type->is<IR::Type_Unknown>(),  // TODO why has it not been set?
              "Invoking isValid() on unexpected type %1% : %2%", hdr->type, call);
}

const IR::Node *DoCopyHeaders::replaceValidMethod(const IR::Member *mem, int value,
                                                  const Util::SourceInfo &srcInfo) {
    // Find the most recent setting of this header's $valid field viz `mem->expr`.
    for (auto m : boost::adaptors::reverse(prev_assignments)) {
        if (mem->expr->equiv(*m->expr)) {
            // This is the most recent assignment to <mem->expr>.$valid.
            if (mem->member == m->member) return nullptr;  // Resetting with same value - redundant.
            break;                                         // The setting is being changed - keep.
        }
    }
    prev_assignments.push_back(mem);
    // convert setValid() and setInvalid() method call to assignment statement on $valid bit.
    auto validtype = IR::Type::Bits::get(1);
    auto src = new IR::Constant(validtype, value);
    auto dst = new IR::Member(srcInfo, validtype, mem->expr, "$valid");
    return new IR::AssignmentStatement(srcInfo, dst, src);
}

/**
 * convert setValid() and setInvalid() method call to assignment statement on $valid bit
 *
 * This conversion is only needed for P4-16 programs to implement the P4-16 semantics
 * of setValid().
 *
 * For P4-14 programs, the setValid() method call is unmodified, because the LocalCopyProp
 * pass needs the method call to implement the semantics for add_header and remove_header.
 * i.e., when validating an uninitialized header in P4-14, all fields in the header must
 * be initialized to zero.
 */
const IR::Node *DoCopyHeaders::postorder(IR::MethodCallStatement *mc) {
    auto mem = mc->methodCall->method->to<IR::Member>();
    if (!mem || !isMethod_xValid(mem->member)) return mc;

    bugcheck_xValid(mc->methodCall, mem->expr);
    if (mem->member == IR::Type_Header::setValid) return replaceValidMethod(mem, 1, mc->srcInfo);
    if (mem->member == IR::Type_Header::setInvalid) return replaceValidMethod(mem, 0, mc->srcInfo);
    if (mem->member == IR::Type_Header::isValid)
        return nullptr;  // "bare" call to isValid (ignoring return value) is a noop.
    BUG("Unreachable code");
}

#if ENABLE_P4C3251
/**
 * Convert `isValid()` method call to an expression on header's `$valid` POV bit field.
 */
const IR::Node *DoCopyHeaders::postorder(IR::MethodCallExpression *mc) {
    if (getContext()->node->is<IR::MethodCallStatement>()) return mc;
    auto mem = mc->method->to<IR::Member>();
    if (!mem || mem->member != IR::Type_Header::isValid) return mc;

    bugcheck_xValid(mc, mem->expr);

    // On Tofino, calling a header's `isValid()` method is implemented by
    // reading the header's POV bit, which is a bit<1> value.
    auto validtype = IR::Type::Bits::get(1);
    auto member = new IR::Member(mc->srcInfo, validtype, mem->expr, "$valid");

    if (findContext<IR::IfStatement>() || findContext<IR::AssignmentStatement>()) {
        // Maintain the Boolean type of the expression - a ReinterpretCast is not enough!
        // But if it is already a ReinterpretCast, don't add an expression.
        if (!getContext()->node->is<IR::BFN::ReinterpretCast>())
            return new IR::Equ(mc->srcInfo, member, new IR::Constant(validtype, 1));
    }
    return member;
}

/**
 * Discover `isValid()` values in table keys to inform replacement below...
 */
const IR::Node *DoCopyHeaders::preorder(IR::P4Table *tbl) {
    key_change.clear();
    auto keys = tbl->getKey();
    auto entryList = tbl->getEntries();
    if (!keys || keys->keyElements.empty() || !entryList || entryList->entries.empty()) return tbl;

    bool any_change = false;
    for (auto key : keys->keyElements) {
        bool change = false;
        if (auto mc = key->expression->to<IR::MethodCallExpression>())
            if (auto mem = mc->method->to<IR::Member>())
                change = mem->member == IR::Type_Header::isValid;
        key_change.push_back(change);
        any_change |= change;
    }
    if (!any_change) key_change.clear();
    return tbl;
}

/**
 * Replace BoolLiteral in `const entries` tables with bit<1> values.
 * The `isValid()` values they are filling-in are converted to bit<1> values above.
 */
const IR::Node *DoCopyHeaders::preorder(IR::Entry *entry) {
    if (key_change.empty()) return entry;
    BUG_CHECK(entry->keys->size() == key_change.size(), "Entry size != Key size");

    IR::Vector<IR::Expression> entry_keys;
    unsigned i = 0;
    for (auto expr : entry->keys->components) {
        auto boolean = expr->to<IR::BoolLiteral>();
        if (key_change[i] && boolean)
            entry_keys.push_back(new IR::Constant(IR::Type::Bits::get(1), boolean->value));
        else
            entry_keys.push_back(expr);  // A non-boolean or `default` boolean.
        ++i;
    }
    auto clone = entry->clone();
    clone->keys = new IR::ListExpression(entry->keys->srcInfo, entry_keys);
    return clone;
}
#endif  // ENABLE_P4C3251

}  // namespace

BFN::CopyHeaders::CopyHeaders(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                              P4::TypeChecking *typeChecking) {
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);
    CHECK_NULL(typeChecking);
    setName("CopyHeaders");
    // We extend P4::CopyStructures' implementation by reimplementing it here.
    passes.emplace_back(typeChecking);
    passes.emplace_back(new P4::RemoveAliases(typeMap));
    passes.emplace_back(typeChecking);
    // `errorOnMethodCall = false` viz don't flag functions returning structs as an error.
    // E.g. Phase0 extern function returns a header struct.
    passes.emplace_back(new P4::DoCopyStructures(typeMap, false));
    passes.emplace_back(typeChecking);
    passes.emplace_back(new DoCopyHeaders(typeMap));
}
