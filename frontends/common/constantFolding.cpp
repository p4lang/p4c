/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "lib/gmputil.h"
#include "constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

class CloneConstants : public Transform {
 public:
    CloneConstants() = default;
    const IR::Node* postorder(IR::Constant* constant) override {
        // We clone the constant.  This is necessary because the same
        // the type associated with the constant may participate in
        // type unification, and thus we want to have different type
        // objects for different constant instances.
        const IR::Type* type = constant->type;
        if (type->is<IR::Type_Bits>()) {
            type = constant->type->clone();
        } else if (auto ii = type->to<IR::Type_InfInt>()) {
            // You can't just clone a InfInt value, because
            // you get the same declid.  We want a new declid.
            type = new IR::Type_InfInt(ii->srcInfo);
        } else {
            BUG("unexpected type %2% for constant %2%", type, constant);
        }
        return new IR::Constant(constant->srcInfo, type, constant->value, constant->base);
    }
    static const IR::Expression* clone(const IR::Expression* expression, const Visitor* calledBy) {
        CloneConstants cc;
        cc.setCalledBy(calledBy);
        return expression->apply(cc)->to<IR::Expression>();
    }
};

const IR::Expression* DoConstantFolding::getConstant(const IR::Expression* expr) const {
    CHECK_NULL(expr);
    if (expr->is<IR::Constant>())
        return expr;
    if (expr->is<IR::BoolLiteral>())
        return expr;
    if (auto list = expr->to<IR::ListExpression>()) {
        for (auto e : list->components)
            if (getConstant(e) == nullptr)
                return nullptr;
        return expr;
    } else if (auto si = expr->to<IR::StructExpression>()) {
        for (auto e : si->components)
            if (getConstant(e->expression) == nullptr)
                return nullptr;
        return expr;
    } else if (auto cast = expr->to<IR::Cast>()) {
        // Casts of a constant to a value with type Type_Newtype
        // are constants, but we cannot fold them.
        if (getConstant(cast->expr))
            return CloneConstants::clone(expr, this);
        return nullptr;
    }
    if (typesKnown) {
        auto ei = EnumInstance::resolve(expr, typeMap);
        if (ei != nullptr)
            return expr;
        // error and match_kind constants can be recognized from their type.
        if (auto pe = expr->to<IR::PathExpression>()) {
            if (auto decl = refMap->getDeclaration(pe->path, true)) {
                if (decl->is<IR::Declaration_ID>()) {
                    if (auto declType = typeMap->getType(decl->getNode())) {
                        if (declType->is<IR::Type_Error>() || declType->is<IR::Type_MatchKind>())
                            return expr;
                    }
                }
            }
        }
    }
    return nullptr;
}

const IR::Node* DoConstantFolding::postorder(IR::PathExpression* e) {
    if (refMap == nullptr || assignmentTarget)
        return e;
    auto decl = refMap->getDeclaration(e->path);
    if (decl == nullptr)
        return e;
    if (auto dc = decl->to<IR::Declaration_Constant>()) {
        auto cst = get(constants, dc);
        if (cst == nullptr)
            return e;
        if (cst->is<IR::ListExpression>()) {
            if (!typesKnown)
                // We don't want to commit to this value before we do
                // type checking; maybe it's wrong.
                return e;
        }
        return CloneConstants::clone(cst, this);
    }
    return e;
}

const IR::Node* DoConstantFolding::postorder(IR::Type_Bits* type) {
    if (type->expression != nullptr) {
        if (auto cst = type->expression->to<IR::Constant>()) {
            type->size = cst->asInt();
            type->expression = nullptr;
            if (type->size < 0 ||
                (type->size == 0 && type->isSigned)) {
                ::error(ErrorType::ERR_INVALID, "%1%: invalid type size", type);
                // Convert it to something legal so we don't get
                // weird errors elsewhere.
                type->size = 64;
            }
        } else {
            ::error(ErrorType::ERR_EXPECTED, "%1%: expected a constant", type->expression);
        }
    }
    return type;
}

const IR::Node* DoConstantFolding::postorder(IR::Type_Varbits* type) {
    if (type->expression != nullptr) {
        if (auto cst = type->expression->to<IR::Constant>()) {
            type->size = cst->asInt();
            type->expression = nullptr;
            if (type->size <= 0)
                ::error(ErrorType::ERR_INVALID, "%1%: invalid type size", type);
        } else {
            ::error(ErrorType::ERR_EXPECTED, "%1%: expected a constant", type->expression);
        }
    }
    return type;
}

const IR::Node* DoConstantFolding::postorder(IR::Declaration_Constant* d) {
    auto init = getConstant(d->initializer);
    if (init == nullptr) {
        if (typesKnown)
            ::error(ErrorType::ERR_INVALID,
                    "%1%: Cannot evaluate initializer for constant", d->initializer);
        return d;
    }
    if (!typesKnown) {
        // This declaration may imply a cast, so the actual value of d
        // is not init, but (d->type)init. The typechecker inserts
        // casts, but if we run this before typechecking we have to be
        // more conservative.
        if (auto cst = init->to<IR::Constant>()) {
            if (auto dtype = d->type->to<IR::Type_Bits>()) {
                auto cstBits = cst->type->to<IR::Type_Bits>();
                if (cstBits && !(*dtype == *cstBits))
                    ::error(ErrorType::ERR_TYPE_ERROR, "%1%: initializer has wrong type %2%",
                            d, cst->type);
                else if (cst->type->is<IR::Type_InfInt>())
                    init = new IR::Constant(init->srcInfo, d->type, cst->value, cst->base);
            } else if (!d->type->is<IR::Type_InfInt>()) {
                // Don't fold this yet, we can't evaluate the cast.
                return d;
            } else {
                // Destination type is InfInt; we must "erase" the width of the source
                if (!cst->type->is<IR::Type_InfInt>()) {
                    init = new IR::Constant(cst->srcInfo, new IR::Type_InfInt(), cst->value,
                                            cst->base);
                }
            }
        }
        if (init != d->initializer)
            d = new IR::Declaration_Constant(d->srcInfo, d->name, d->annotations, d->type, init);
    }
    if (!typesKnown && (init->is<IR::StructExpression>() ||
                        // If we substitute structs before type checking we may lose casts
                        // e.g. struct S { bit<8> x; }
                        // const S s = { x = 1024 };
                        // const bit<16> z = (bit<16>)s.x;
                        // If we substitute this too early we may get a value of 1024 for z.
                        init->is<IR::Cast>()))
                        // Also, note that early in the compilation some struct expressions may
                        // still be represented as cast expressions that cast to a struct type.
        return d;
    LOG3("Constant " << d << " set to " << init);
    constants.emplace(getOriginal<IR::Declaration_Constant>(), init);
    return d;
}

const IR::Node* DoConstantFolding::preorder(IR::AssignmentStatement* statement) {
    assignmentTarget = true;
    visit(statement->left);
    assignmentTarget = false;
    visit(statement->right);
    prune();
    return statement;
}

const IR::Node* DoConstantFolding::preorder(IR::ArrayIndex* e) {
    visit(e->left);
    bool save = assignmentTarget;
    assignmentTarget = false;
    visit(e->right);
    assignmentTarget = save;
    prune();

    if (!typesKnown)
        return e;
    auto orig = getOriginal<IR::ArrayIndex>();
    auto type = typeMap->getType(orig->left, true);
    if (type->is<IR::Type_Tuple>()) {
        auto init = getConstant(e->right);
        if (init == nullptr) {
            if (typesKnown)
                ::error(ErrorType::ERR_INVALID,
                        "%1%: Index must evaluate to a constant", e->right);
            return e;
        }
        if (auto cst = init->to<IR::Constant>()) {
            if (!cst->fitsInt()) {
                ::error(ErrorType::ERR_INVALID, "Index too large: %1%", cst);
                return e;
            }
            int index = cst->asInt();
            if (index < 0) {
                ::error(ErrorType::ERR_INVALID,
                        "Tuple index %1% must be constant", e->right);
                return e;
            }
            auto value = getConstant(e->left);
            if (!value)
                return e;
            if (auto list = value->to<IR::ListExpression>()) {
                if (static_cast<size_t>(index) >= list->size()) {
                    ::error(ErrorType::ERR_INVALID,
                            "Tuple index %1% out of bounds", e->right);
                    return e;
                }
                return CloneConstants::clone(list->components.at(static_cast<size_t>(index)), this);
            }
        }
    }
    return e;
}

const IR::Node* DoConstantFolding::postorder(IR::Cmpl* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::Constant>();
    if (cst == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", op);
        return e;
    }
    const IR::Type* t = op->type;
    if (t->is<IR::Type_InfInt>()) {
        ::error(ErrorType::ERR_INVALID,
                "%1%: Operation cannot be applied to values with unknown width;\n"
                "please specify width explicitly", e);
        return e;
    }
    auto tb = t->to<IR::Type_Bits>();
    if (tb == nullptr) {
        // This could be a serEnum value
        return e;
    }

    big_int value = ~cst->value;
    return new IR::Constant(cst->srcInfo, t, value, cst->base, true);
}

const IR::Node* DoConstantFolding::postorder(IR::Neg* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::Constant>();
    if (cst == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", op);
        return e;
    }
    const IR::Type* t = op->type;
    if (t->is<IR::Type_InfInt>())
        return new IR::Constant(cst->srcInfo, t, -cst->value, cst->base);

    auto tb = t->to<IR::Type_Bits>();
    if (tb == nullptr) {
        // This could be a SerEnum value
        return e;
    }

    big_int value = -cst->value;
    return new IR::Constant(cst->srcInfo, t, value, cst->base, true);
}

const IR::Node* DoConstantFolding::postorder(IR::UPlus* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::Constant>();
    if (cst == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", op);
        return e;
    }
    return cst;
}

const IR::Constant*
DoConstantFolding::cast(const IR::Constant* node, unsigned base, const IR::Type_Bits* type) const {
    return new IR::Constant(node->srcInfo, type, node->value, base);
}

const IR::Node* DoConstantFolding::postorder(IR::Add* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a + b; });
}

const IR::Node* DoConstantFolding::postorder(IR::AddSat* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a + b; }, true);
}

const IR::Node* DoConstantFolding::postorder(IR::Sub* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a - b; });
}

const IR::Node* DoConstantFolding::postorder(IR::SubSat* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a - b; }, true);
}

const IR::Node* DoConstantFolding::postorder(IR::Mul* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a * b; });
}

const IR::Node* DoConstantFolding::postorder(IR::BXor* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a ^ b; });
}

const IR::Node* DoConstantFolding::postorder(IR::BAnd* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a & b; });
}

const IR::Node* DoConstantFolding::postorder(IR::BOr* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a | b; });
}

const IR::Node* DoConstantFolding::postorder(IR::Equ* e) {
    return compare(e);
}

const IR::Node* DoConstantFolding::postorder(IR::Neq* e) {
    return compare(e);
}

const IR::Node* DoConstantFolding::postorder(IR::Lss* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a < b; });
}

const IR::Node* DoConstantFolding::postorder(IR::Grt* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a > b; });
}

const IR::Node* DoConstantFolding::postorder(IR::Leq* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a <= b; });
}

const IR::Node* DoConstantFolding::postorder(IR::Geq* e) {
    return binary(e, [](big_int a, big_int b) -> big_int { return a >= b; });
}

const IR::Node* DoConstantFolding::postorder(IR::Div* e) {
    return binary(e, [e](big_int a, big_int b) -> big_int {
            if (a < 0 || b < 0) {
                ::error(ErrorType::ERR_INVALID,
                     "%1%: Division is not defined for negative numbers", e);
                return 0;
            }
            if (b == 0) {
                ::error(ErrorType::ERR_INVALID, "%1%: Division by zero", e);
                return 0;
            }
            return a / b;
        });
}

const IR::Node* DoConstantFolding::postorder(IR::Mod* e) {
    return binary(e, [e](big_int a, big_int b) -> big_int {
            if (a < 0 || b < 0) {
                ::error(ErrorType::ERR_INVALID,
                        "%1%: Modulo is not defined for negative numbers", e);
                return 0;
            }
            if (b == 0) {
                ::error(ErrorType::ERR_INVALID, "%1%: Modulo by zero", e);
                return 0;
            }
            return a % b; });
}

const IR::Node* DoConstantFolding::postorder(IR::Shr* e) {
    return shift(e);
}

const IR::Node* DoConstantFolding::postorder(IR::Shl* e) {
    return shift(e);
}

const IR::Node*
DoConstantFolding::compare(const IR::Operation_Binary* e) {
    auto eleft = getConstant(e->left);
    auto eright = getConstant(e->right);
    if (eleft == nullptr || eright == nullptr)
        return e;

    bool eqTest = e->is<IR::Equ>();
    if (eleft->is<IR::BoolLiteral>()) {
        auto left = eleft->to<IR::BoolLiteral>();
        auto right = eright->to<IR::BoolLiteral>();
        if (left == nullptr || right == nullptr) {
            ::error(ErrorType::ERR_INVALID, "%1%: both operands must be Boolean", e);
            return e;
        }
        bool bresult = (left->value == right->value) == eqTest;
        return new IR::BoolLiteral(e->srcInfo, IR::Type_Boolean::get(),  bresult);
    } else if (typesKnown) {
        auto le = EnumInstance::resolve(eleft, typeMap);
        auto re = EnumInstance::resolve(eright, typeMap);
        if (le != nullptr && re != nullptr) {
            BUG_CHECK(le->type == re->type,
                      "%1%: different enum types in comparison", e);
            bool bresult = (le->name == re->name) == eqTest;
            return new IR::BoolLiteral(e->srcInfo, IR::Type_Boolean::get(), bresult);
        }

        auto llist = eleft->to<IR::ListExpression>();
        auto rlist = eright->to<IR::ListExpression>();
        if (llist != nullptr && rlist != nullptr) {
            if (llist->components.size() != rlist->components.size()) {
                ::error(ErrorType::ERR_INVALID, "%1%: comparing lists of different size", e);
                return e;
            }

            for (size_t i = 0; i < llist->components.size(); i++) {
                auto li = llist->components.at(i);
                auto ri = rlist->components.at(i);
                const IR::Operation_Binary* tmp;
                if (eqTest)
                    tmp = new IR::Equ(li, ri);
                else
                    tmp = new IR::Neq(li, ri);
                auto cmp = compare(tmp);
                auto boolLit = cmp->to<IR::BoolLiteral>();
                if (boolLit == nullptr)
                    return e;
                if (boolLit->value != eqTest)
                    return boolLit;
            }
            return new IR::BoolLiteral(e->srcInfo, IR::Type_Boolean::get(), eqTest);
        }
    }

    if (eqTest)
        return binary(e, [](big_int a, big_int b) -> big_int { return a == b; });
    else
        return binary(e, [](big_int a, big_int b) -> big_int { return a != b; });
}

const IR::Node*
DoConstantFolding::binary(const IR::Operation_Binary* e,
                          std::function<big_int(big_int, big_int)> func,
                          bool saturating) {
    auto eleft = getConstant(e->left);
    auto eright = getConstant(e->right);
    if (eleft == nullptr || eright == nullptr)
        return e;

    auto left = eleft->to<IR::Constant>();
    if (left == nullptr) {
        // This can be a serEnum value
        return e;
    }
    auto right = eright->to<IR::Constant>();
    if (right == nullptr) {
        // This can be a serEnum value
        return e;
    }

    const IR::Type* lt = left->type;
    const IR::Type* rt = right->type;
    bool lunk = lt->is<IR::Type_InfInt>();
    bool runk = rt->is<IR::Type_InfInt>();

    const IR::Type* resultType;

    const IR::Type_Bits* ltb = nullptr;
    const IR::Type_Bits* rtb = nullptr;
    if (!lunk) {
        ltb = lt->to<IR::Type_Bits>();
        if (ltb == nullptr) {
            // Could be a serEnum
            return e;
        }
    }
    if (!runk) {
        rtb = rt->to<IR::Type_Bits>();
        if (rtb == nullptr) {
            // Could be a serEnum
            return e;
        }
    }

    if (!lunk && !runk) {
        // both typed
        if (!ltb->operator==(*rtb)) {
            ::error(ErrorType::ERR_INVALID,
                    "%1%: operands have different types: %2% and %3%",
                    e, ltb->toString(), rtb->toString());
            return e;
        }
        resultType = rtb;
    } else if (lunk && runk) {
        resultType = lt;  // i.e., Type_InfInt
    } else {
        // must cast one to the type of the other
        if (lunk) {
            resultType = rtb;
            left = cast(left, left->base, rtb);
        } else {
            resultType = ltb;
            right = cast(right, left->base, ltb);
        }
    }
    big_int value = func(left->value, right->value);
    if (saturating) {
        if ((rtb = resultType->to<IR::Type::Bits>())) {
            big_int limit = 1;
            if (rtb->isSigned) {
                limit <<= rtb->size-1;
                if (value < -limit)
                    value = -limit;
            } else {
                limit <<= rtb->size;
                if (value < 0)
                    value = 0; }
            if (value >= limit)
                value = limit - 1;
        } else {
            ::error(ErrorType::ERR_INVALID,
                    "%1%: saturating operation on untyped values", e);
        }
    }

    if (e->is<IR::Operation_Relation>())
        return new IR::BoolLiteral(e->srcInfo, IR::Type_Boolean::get(), value != 0);
    else
        return new IR::Constant(e->srcInfo, resultType, value, left->base, true);
}

const IR::Node* DoConstantFolding::postorder(IR::LAnd* e) {
    auto left = getConstant(e->left);
    if (left == nullptr)
        return e;

    auto lcst = left->to<IR::BoolLiteral>();
    if (lcst == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: Expected a boolean value", left);
        return e;
    }
    if (lcst->value) {
        return e->right;
    }
    return new IR::BoolLiteral(left->srcInfo, IR::Type_Boolean::get(), false);
}

const IR::Node* DoConstantFolding::postorder(IR::LOr* e) {
    auto left = getConstant(e->left);
    if (left == nullptr)
        return e;

    auto lcst = left->to<IR::BoolLiteral>();
    if (lcst == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: Expected a boolean value", left);
        return e;
    }
    if (!lcst->value) {
        return e->right;
    }
    return new IR::BoolLiteral(left->srcInfo, IR::Type_Boolean::get(), true);
}

static bool overflowWidth(const IR::Node* node, int width) {
    if (width > P4CContext::getConfig().maximumWidthSupported()) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: Compiler only supports widths up to %2%",
                node, P4CContext::getConfig().maximumWidthSupported());
        return true;
    }
    return false;
}

const IR::Node* DoConstantFolding::postorder(IR::Slice* e) {
    const IR::Expression* msb = getConstant(e->e1);
    const IR::Expression* lsb = getConstant(e->e2);
    if (msb == nullptr) {
        if (typesKnown)
            ::error(ErrorType::ERR_EXPECTED,
                    "%1%: slice indexes must be compile-time constants", e->e1);
        return e;
    }
    if (lsb == nullptr) {
        if (typesKnown)
            ::error(ErrorType::ERR_EXPECTED,
                    "%1%: slice indexes must be compile-time constants", e->e2);
        return e;
    }

    auto e0 = getConstant(e->e0);
    if (e0 == nullptr)
        return e;

    auto cmsb = msb->to<IR::Constant>();
    if (cmsb == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", msb);
        return e;
    }
    auto clsb = lsb->to<IR::Constant>();
    if (clsb == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", lsb);
        return e;
    }
    auto cbase = e0->to<IR::Constant>();
    if (cbase == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", e->e0);
        return e;
    }

    int m = cmsb->asInt();
    int l = clsb->asInt();
    if (m < l) {
        ::error(ErrorType::ERR_EXPECTED,
                "%1%: bit slicing should be specified as [msb:lsb]", e);
        return e;
    }
    if (overflowWidth(e, m) || overflowWidth(e, l))
        return e;
    big_int value = cbase->value >> l;
    big_int mask = 1;
    mask = (mask << (m - l + 1)) - 1;
    value = value & mask;
    auto resultType = IR::Type_Bits::get(m - l + 1);
    return new IR::Constant(e->srcInfo, resultType, value, cbase->base, true);
}

const IR::Node* DoConstantFolding::postorder(IR::Member* e) {
    if (!typesKnown)
        return e;
    auto orig = getOriginal<IR::Member>();
    auto type = typeMap->getType(orig->expr, true);
    auto origtype = typeMap->getType(orig);

    const IR::Expression* result = e;
    if (type->is<IR::Type_Stack>() && e->member == IR::Type_Stack::arraySize) {
        auto st = type->to<IR::Type_Stack>();
        auto size = st->getSize();
        result = new IR::Constant(st->size->srcInfo, origtype, size);
    } else {
        auto expr = getConstant(e->expr);
        if (expr == nullptr)
            return e;

        auto structType = type->to<IR::Type_StructLike>();
        if (structType == nullptr)
            BUG("Expected a struct type, got %1%", type);
        if (auto list = expr->to<IR::ListExpression>()) {
            bool found = false;
            int index = 0;
            for (auto f : structType->fields) {
                if (f->name.name == e->member.name) {
                    found = true;
                    break;
                }
                index++;
            }

            if (!found)
                    BUG("Could not find field %1% in type %2%", e->member, type);
            result = CloneConstants::clone(list->components.at(index), this);
        } else if (auto si = expr->to<IR::StructExpression>()) {
            if (origtype->is<IR::Type_Header>() && e->member.name == IR::Type_Header::isValid)
                return e;
            auto ne = si->components.getDeclaration<IR::NamedExpression>(e->member.name);
            BUG_CHECK(ne != nullptr,
                      "Could not find field %1% in initializer %2%", e->member, si);
            return CloneConstants::clone(ne->expression, this);
        } else {
            BUG("Unexpected initializer: %1%", expr);
        }
    }
    return result;
}

const IR::Node* DoConstantFolding::postorder(IR::Concat* e) {
    auto eleft = getConstant(e->left);
    auto eright = getConstant(e->right);
    if (eleft == nullptr || eright == nullptr)
        return e;

    auto left = eleft->to<IR::Constant>();
    if (left == nullptr) {
        // Could be a SerEnum
        return e;
    }
    auto right = eright->to<IR::Constant>();
    if (right == nullptr) {
        // Could be a SerEnum
        return e;
    }

    auto lt = left->type->to<IR::Type_Bits>();
    auto rt = right->type->to<IR::Type_Bits>();
    if (lt == nullptr || rt == nullptr) {
        ::error(ErrorType::ERR_INVALID, "%1%: both operand widths must be known", e);
        return e;
    }

    auto resultType = IR::Type_Bits::get(lt->size + rt->size, lt->isSigned);
    if (overflowWidth(e, resultType->size))
        return e;
    big_int value = Util::shift_left(left->value, static_cast<unsigned>(rt->size)) + right->value;
    return new IR::Constant(e->srcInfo, resultType, value, left->base);
}

const IR::Node* DoConstantFolding::postorder(IR::LNot* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::BoolLiteral>();
    if (cst == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: Expected a boolean value", op);
        return e;
    }
    return new IR::BoolLiteral(cst->srcInfo, IR::Type_Boolean::get(), !cst->value);
}

const IR::Node* DoConstantFolding::postorder(IR::Mux* e) {
    auto cond = getConstant(e->e0);
    if (cond == nullptr)
        return e;
    auto b = cond->to<IR::BoolLiteral>();
    if (b == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected a Boolean", cond);
        return e; }
    if (b->value)
        return e->e1;
    else
        return e->e2;
}

const IR::Node* DoConstantFolding::shift(const IR::Operation_Binary* e) {
    auto right = getConstant(e->right);
    if (right == nullptr)
        return e;

    auto cr = right->to<IR::Constant>();
    if (cr == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", right);
        return e;
    }
    if (cr->value < 0) {
        ::error(ErrorType::ERR_INVALID, "%1%: Shifts with negative amounts are not permitted", e);
        return e;
    }

    if (cr->value == 0) {
        // ::warning("%1% with zero", e);
        return e->left;
    }

    auto left = getConstant(e->left);
    if (left == nullptr)
        return e;

    auto cl = left->to<IR::Constant>();
    if (cl == nullptr) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected an integer value", left);
        return e;
    }

    big_int value = cl->value;
    unsigned shift = static_cast<unsigned>(cr->asInt());
    if (overflowWidth(e, shift))
        return e;

    auto tb = left->type->to<IR::Type_Bits>();
    if (tb != nullptr) {
        if (((unsigned)tb->size < shift) && warnings)
            ::warning(ErrorType::WARN_OVERFLOW,
                      "%1%: Shifting %2%-bit value with %3%", e, tb->size, shift);
    }

    if (e->is<IR::Shl>())
        value = Util::shift_left(value, shift);
    else
        value = Util::shift_right(value, shift);
    return new IR::Constant(e->srcInfo, left->type, value, cl->base);
}

const IR::Node *DoConstantFolding::postorder(IR::Cast *e) {
    auto expr = getConstant(e->expr);
    if (expr == nullptr)
        return e;

    const IR::Type* etype;
    if (typesKnown)
        etype = typeMap->getType(getOriginal(), true);
    else
        etype = e->destType;

    if (etype->is<IR::Type_Bits>()) {
        auto type = etype->to<IR::Type_Bits>();
        if (auto arg = expr->to<IR::Constant>()) {
            return cast(arg, arg->base, type);
        } else if (auto arg = expr->to<IR::BoolLiteral>()) {
            int v = arg->value ? 1 : 0;
            return new IR::Constant(e->srcInfo, type, v, 10);
        } else if (expr->is<IR::Member>()) {
            auto ei = EnumInstance::resolve(expr, typeMap);
            if (ei == nullptr)
                return e;
            if (auto se = ei->to<SerEnumInstance>()) {
                return se->value;
            }
        } else {
            return e;
        }
    } else if (etype->is<IR::Type_Boolean>()) {
        if (expr->is<IR::BoolLiteral>())
            return expr;
        if (expr->is<IR::Constant>()) {
            auto cst = expr->to<IR::Constant>();
            auto ctype = cst->type;
            if (ctype->is<IR::Type_Bits>()) {
                auto tb = ctype->to<IR::Type_Bits>();
                if (tb->isSigned) {
                    ::error(ErrorType::ERR_INVALID, "%1%: Cannot cast signed value to boolean", e);
                    return e;
                }
                if (tb->size != 1) {
                    ::error(ErrorType::ERR_INVALID,
                            "%1%: Only bit<1> values can be cast to bool", e);
                    return e;
                }
            } else {
                BUG_CHECK(ctype->is<IR::Type_InfInt>(), "%1%: unexpected type %2% for constant",
                          cst, ctype);
            }

            int v = cst->asInt();
            if (v < 0 || v > 1) {
                ::error(ErrorType::ERR_INVALID, "%1%: Only 0 and 1 can be cast to booleans", e);
                return e;
            }
            return new IR::BoolLiteral(e->srcInfo, IR::Type_Boolean::get(), v == 1);
        }
    } else if (etype->is<IR::Type_StructLike>()) {
        return CloneConstants::clone(expr, this);
    }
    return e;
}

DoConstantFolding::Result
DoConstantFolding::setContains(const IR::Expression* keySet, const IR::Expression* select) const {
    if (keySet->is<IR::DefaultExpression>())
        return Result::Yes;
    if (auto list = select->to<IR::ListExpression>()) {
        if (auto klist = keySet->to<IR::ListExpression>()) {
            BUG_CHECK(list->components.size() == klist->components.size(),
                      "%1% and %2% size mismatch", list, klist);
            for (unsigned i=0; i < list->components.size(); i++) {
                auto r = setContains(klist->components.at(i), list->components.at(i));
                if (r == Result::DontKnow || r == Result::No)
                    return r;
            }
            return Result::Yes;
        } else {
            BUG_CHECK(list->components.size() == 1, "%1%: mismatch in list size", list);
            return setContains(keySet, list->components.at(0));
        }
    }

    if (select->is<IR::BoolLiteral>()) {
        auto key = getConstant(keySet);
        if (key == nullptr) {
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: expression must evaluate to a constant", key);
            return Result::No;
        }
        BUG_CHECK(key->is<IR::BoolLiteral>(), "%1%: expected a boolean", key);
        if (select->to<IR::BoolLiteral>()->value == key->to<IR::BoolLiteral>()->value)
            return Result::Yes;
        return Result::No;
    }

    if (select->is<IR::Member>()) {
        // This must be an enum value
        auto key = getConstant(keySet);
        if (key == nullptr) {
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: expression must evaluate to a constant", key);
            return Result::No;
        }
        auto sel = getConstant(select);
        // For Enum and SerEnum instances we can just use expression equivalence.
        // This assumes that type checking does not allow us to compare constants to SerEnums.
        if (key->equiv(*sel))
            return Result::Yes;
        return Result::No;
    }

    BUG_CHECK(select->is<IR::Constant>(), "%1%: expected a constant", select);
    auto cst = select->to<IR::Constant>();
    if (auto kc = keySet->to<IR::Constant>()) {
        if (kc->value == cst->value)
            return Result::Yes;
        return Result::No;
    } else if (auto range = keySet->to<IR::Range>()) {
        auto left = getConstant(range->left);
        if (left == nullptr) {
            ::error(ErrorType::ERR_INVALID, "%1%: expression must evaluate to a constant", left);
            return Result::DontKnow;
        }
        auto right = getConstant(range->right);
        if (right == nullptr) {
            ::error(ErrorType::ERR_INVALID, "%1%: expression must evaluate to a constant", right);
            return Result::DontKnow;
        }
        if (left->to<IR::Constant>()->value <= cst->value &&
            right->to<IR::Constant>()->value >= cst->value)
            return Result::Yes;
        return Result::No;
    } else if (auto mask = keySet->to<IR::Mask>()) {
        // check if left & right == cst & right
        auto left = getConstant(mask->left);
        if (left == nullptr) {
            ::error(ErrorType::ERR_INVALID, "%1%: expression must evaluate to a constant", left);
            return Result::DontKnow;
        }
        auto right = getConstant(mask->right);
        if (right == nullptr) {
            ::error(ErrorType::ERR_INVALID, "%1%: expression must evaluate to a constant", right);
            return Result::DontKnow;
        }
        if ((left->to<IR::Constant>()->value & right->to<IR::Constant>()->value) ==
            (right->to<IR::Constant>()->value & cst->value))
            return Result::Yes;
        return Result::No;
    }
    ::error(ErrorType::ERR_INVALID, "%1%: unexpected expression", keySet);
    return Result::DontKnow;
}

const IR::Node* DoConstantFolding::postorder(IR::SelectExpression* expression) {
    if (!typesKnown) return expression;
    auto sel = getConstant(expression->select);
    if (sel == nullptr)
        return expression;

    IR::Vector<IR::SelectCase> cases;
    bool someUnknown = false;
    bool changes = false;
    bool finished = false;

    const IR::Expression* result = expression;
    /* FIXME -- should erase/replace each element as needed, rather than creating a new Vector.
     * Should really implement this in SelectCase pre/postorder and this postorder goes away */
    for (auto c : expression->selectCases) {
        if (finished) {
            if (warnings)
                warn(ErrorType::WARN_PARSER_TRANSITION, "%1%: unreachable case", c);
            continue;
        }
        auto inside = setContains(c->keyset, sel);
        if (inside == Result::No) {
            changes = true;
            continue;
        } else if (inside == Result::DontKnow) {
            someUnknown = true;
            cases.push_back(c);
        } else {
            changes = true;
            finished = true;
            if (someUnknown) {
                auto newc = new IR::SelectCase(c->srcInfo, new IR::DefaultExpression(), c->state);
                cases.push_back(newc);
            } else {
                // This is the result.
                result = c->state;
            }
        }
    }

    if (changes) {
        if (cases.size() == 0 && result == expression && warnings)
            warn(ErrorType::WARN_PARSER_TRANSITION, "%1%: no case matches", expression);
        expression->selectCases = std::move(cases);
    }
    return result;
}

const IR::Node *DoConstantFolding::postorder(IR::IfStatement *ifstmt) {
    if (auto cond = ifstmt->condition->to<IR::BoolLiteral>()) {
        if (cond->value) {
            return ifstmt->ifTrue;
        } else {
            if (ifstmt->ifFalse == nullptr) {
                return new IR::EmptyStatement(ifstmt->srcInfo);
            } else {
                return ifstmt->ifFalse;
            }
        }
    }
    return ifstmt;
}

}  // namespace P4
