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

#include "parser_counter.h"
#include <boost/range/adaptor/reversed.hpp>
#include "ir/ir.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"

namespace BFN {
namespace V1 {

static bool isParserCounter(const IR::Member* member) {
    if (member->member == "parser_counter") {
        if (auto path = member->expr->to<IR::PathExpression>()) {
            if (path->path->name == "ig_prsr_ctrl")
                return true;
        }
    }

    return false;
}

/// get field's alignment (lo, hi) in little-endian
static std::pair<unsigned, unsigned> getAlignLoHi(const IR::Member* member) {
    auto header = member->expr->type->to<IR::Type_Header>();

    CHECK_NULL(header);

    unsigned bits = 0;

    for (auto field : boost::adaptors::reverse(header->fields)) {
        auto size = field->type->width_bits();

        if (field->name == member->member) {
            if (size > 8) {
                ::fatal_error("Parser counter load field is of width %1% bits"
                      " which is greater than what HW supports (8 bits): %2%", size, member);
            }

            return {bits % 8, (bits + size - 1) % 8};
        }

        bits += size;
    }

    BUG("%1% not found in header?", member->member);
}

void ParserCounterConverter::cannotFit(const IR::AssignmentStatement* stmt, const char* what) {
    error("Parser counter %1% amount cannot fit into 8-bit. %2%", what, stmt);
}

const IR::Node* ParserCounterConverter::postorder(IR::AssignmentStatement* ) {
    auto stmt = getOriginal<IR::AssignmentStatement>();
    auto parserCounter = new IR::PathExpression("ig_prsr_ctrl_parser_counter");
    auto right = stmt->right;

    // Remove any casts around the source of the assignment.
    if (auto cast = right->to<IR::Cast>()) {
        if (cast->destType->is<IR::Type_Bits>()) {
            right = cast->expr;
        }
    }

    IR::MethodCallStatement *methodCall = nullptr;

    if (right->to<IR::Constant>() || right->to<IR::Member>()) {
        // Load operation (immediate or field)
        methodCall = new IR::MethodCallStatement(stmt->srcInfo,
            new IR::MethodCallExpression(stmt->srcInfo,
                new IR::Member(parserCounter, "set"),
                new IR::Vector<IR::Type>({ stmt->right->type }),
                new IR::Vector<IR::Argument>({ new IR::Argument(stmt->right) })));
    } else if (auto add = right->to<IR::Add>()) {
        auto member = add->left->to<IR::Member>();

        auto counterWidth = IR::Type::Bits::get(8);
        auto maskWidth = IR::Type::Bits::get(Device::currentDevice() == Device::TOFINO ? 3 : 8);
        auto max = new IR::Constant(counterWidth, 255);  // How does user specify the max in P4-14?

        // Add operaton
        if (member && isParserCounter(member)) {
            methodCall = new IR::MethodCallStatement(
                stmt->srcInfo, new IR::Member(parserCounter, "increment"),
                { new IR::Argument(add->right) });
        } else {
            if (auto* amt = add->right->to<IR::Constant>()) {
                // Load operation (expression of field)
                if (member) {
                    auto shr = new IR::Constant(counterWidth, 0);
                    auto mask = new IR::Constant(maskWidth,
                                    Device::currentDevice() == Device::TOFINO ? 7 : 255);
                    auto add = new IR::Constant(counterWidth, amt->asUnsigned());

                    methodCall = new IR::MethodCallStatement(stmt->srcInfo,
                        new IR::MethodCallExpression(stmt->srcInfo,
                            new IR::Member(parserCounter, "set"),
                            new IR::Vector<IR::Type>({ member->type }),
                            new IR::Vector<IR::Argument>({
                                new IR::Argument(member),
                                new IR::Argument(max),
                                new IR::Argument(shr),
                                new IR::Argument(mask),
                                new IR::Argument(add) })));
                } else if (auto* shl = add->left->to<IR::Shl>()) {
                    if (auto* rot = shl->right->to<IR::Constant>()) {
                        auto* field = shl->left->to<IR::Member>();
                        if (!field) {
                            if (auto cast = shl->left->to<IR::Cast>())
                                field = cast->expr->to<IR::Member>();
                        }

                        if (field) {
                            if (!rot->fitsUint() || rot->asUnsigned() >> 8)
                                cannotFit(stmt, "multiply");

                            if (!amt->fitsUint() || amt->asUnsigned() >> 8)
                                cannotFit(stmt, "immediate");

                            unsigned lo = 0, hi = 7;
                            std::tie(lo, hi) = getAlignLoHi(field);

                            auto shr = new IR::Constant(counterWidth, 8 - rot->asUnsigned() - lo);

                            unsigned rot_hi = std::min(hi + rot->asUnsigned(), 7u);

                            auto mask = new IR::Constant(maskWidth,
                                            Device::currentDevice() == Device::TOFINO ?
                                                rot_hi : (1 << (rot_hi + 1)) - 1);

                            auto add = new IR::Constant(counterWidth, amt->asUnsigned());

                            methodCall = new IR::MethodCallStatement( stmt->srcInfo,
                                new IR::MethodCallExpression(stmt->srcInfo,
                                    new IR::Member(parserCounter, "set"),
                                    new IR::Vector<IR::Type>({ field->type }),
                                    new IR::Vector<IR::Argument>({
                                        new IR::Argument(field),
                                        new IR::Argument(max),
                                        new IR::Argument(shr),
                                        new IR::Argument(mask),
                                        new IR::Argument(add) })));
                        }
                    }
                }
            }
        }
    }

    if (!methodCall)
        error("Unsupported syntax for parser counter: %1%", stmt);

    return methodCall;
}

struct ParserCounterSelectCaseConverter : Transform {
    bool isNegative = false;
    bool needsCast = false;
    int counterIdx = -1;

    const IR::Node* preorder(IR::SelectExpression* node) {
        for (unsigned i = 0; i < node->select->components.size(); i++) {
            auto select = node->select->components[i];

            if (auto member = select->to<IR::Member>()) {
                if (isParserCounter(member)) {
                    if (counterIdx >= 0)
                        error("Multiple selects on parser counter in %1%", node);
                    counterIdx = i;
                }
            }
        }

        return node;
    }

    struct RewriteSelectCase : Transform {
        bool isNegative = false;
        bool needsCast = false;

        const IR::Expression* convert(const IR::Constant* c,
                bool toBool = true, bool negate = true, bool check = true) {
            auto val = c->asUnsigned();

            if (check) {
                if (val & 0x80) {
                    isNegative = true;
                } else if (val) {
                    error("Parser counter only supports test of value being zero or negative.");
                }
            }

            if (toBool)
                return new IR::BoolLiteral(negate ? ~val : val);
            else
                return new IR::Constant(IR::Type::Bits::get(1), (negate ? ~val : val)  & 1);
        }

        const IR::Node* preorder(IR::Mask* mask) override {
            prune();

            mask->left = convert(mask->left->to<IR::Constant>(), false);
            mask->right = convert(mask->right->to<IR::Constant>(), false, false, false);

            needsCast = true;

            return mask;
        }

        const IR::Node* preorder(IR::Constant* c) override {
            return convert(c);
        }
    };

    const IR::Node* preorder(IR::SelectCase* node) {
        RewriteSelectCase rewrite;

        if (auto list = node->keyset->to<IR::ListExpression>()) {
            auto newList = list->clone();

            for (unsigned i = 0; i < newList->components.size(); i++) {
                if (int(i) == counterIdx) {
                    newList->components[i] = newList->components[i]->apply(rewrite);
                    break;
                }
            }

            node->keyset = newList;
        } else {
            node->keyset = node->keyset->apply(rewrite);
        }

        isNegative |= rewrite.isNegative;
        needsCast |= rewrite.needsCast;

        return node;
    }
};

struct ParserCounterSelectExprConverter : Transform {
    const ParserCounterSelectCaseConverter& caseConverter;

    explicit ParserCounterSelectExprConverter(const ParserCounterSelectCaseConverter& cc)
        : caseConverter(cc) {}

    const IR::Node* postorder(IR::Member* node) {
        if (isParserCounter(node)) {
            auto parserCounter = new IR::PathExpression("ig_prsr_ctrl_parser_counter");
            auto testExpr = new IR::Member(parserCounter,
                                           caseConverter.isNegative ? "is_negative" : "is_zero");

            const IR::Expression* methodCall = new IR::MethodCallExpression(node->srcInfo, testExpr,
                new IR::Vector<IR::Argument>());

            if (caseConverter.needsCast)
                methodCall = new IR::Cast(IR::Type::Bits::get(1), methodCall);

            return methodCall;
        }

        return node;
    }
};

ParserCounterSelectionConverter::ParserCounterSelectionConverter() {
    auto convertSelectCase = new ParserCounterSelectCaseConverter;
    auto convertSelectExpr = new ParserCounterSelectExprConverter(*convertSelectCase);

    addPasses({convertSelectCase, convertSelectExpr});
}

}  // namespace V1
}  // namespace BFN
