#include "lib/gmputil.h"
#include "constantFolding.h"
#include "ir/configuration.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

const IR::Expression* ConstantFolding::getConstant(const IR::Expression* expr) const {
    if (expr == nullptr)
        BUG("Null pointer for expression");

    auto cst = get(constants, expr);
    if (cst != nullptr)
        return cst;
    if (expr->is<IR::Constant>())
        return expr;
    if (expr->is<IR::BoolLiteral>())
        return expr;
    if (expr->is<IR::ListExpression>()) {
        auto list = expr->to<IR::ListExpression>();
        for (auto e : *list->components)
            if (getConstant(e) == nullptr)
                return nullptr;
        return list;
    }
    if (typesKnown) {
        auto ei = EnumInstance::resolve(expr, typeMap);
        if (ei != nullptr)
            return expr;
    }

    return nullptr;
}

// This has to be called from a visitor method - it calls getOriginal()
void ConstantFolding::setConstant(const IR::Node* node, const IR::Expression* result) {
    LOG1("Folding " << node << " to " << result << " (" << result->id << ")");
    constants.emplace(node, result);
    constants.emplace(getOriginal(), result);
}

const IR::Node* ConstantFolding::postorder(IR::P4Program* program) {
    // dump(program);
    return program;
}

const IR::Node* ConstantFolding::postorder(IR::PathExpression* e) {
    if (refMap == nullptr)
        return e;
    auto decl = refMap->getDeclaration(e->path);
    if (decl == nullptr)
        return e;
    auto v = get(constants, decl->getNode());
    if (v == nullptr)
        return e;
    setConstant(e, v);
    if (v->is<IR::ListExpression>())
        return e;
    return v;
}

const IR::Node* ConstantFolding::postorder(IR::Declaration_Constant* d) {
    auto init = getConstant(d->initializer);
    if (init == nullptr) {
        if (typesKnown)
            ::error("%1%: Cannot evaluate initializer for constant", d->initializer);
        return d;
    }

    if (typesKnown) {
        // If we typechecked we're safe
        setConstant(d, init);
    } else {
        // In fact, this declaration may imply a cast, so the actual value of
        // d is not init, but (d->type)init.  The typechecker inserts casts,
        // but if we run this before typechecking we have to be more conservative.
        if (init->is<IR::Constant>()) {
            auto cst = init->to<IR::Constant>();
            if (d->type->is<IR::Type_Bits>()) {
                if (cst->type->is<IR::Type_InfInt>() ||
                    (cst->type->is<IR::Type_Bits>() &&
                     !(*d->type->to<IR::Type_Bits>() == *cst->type->to<IR::Type_Bits>())))
                    init = new IR::Constant(init->srcInfo, d->type, cst->value, cst->base);
                setConstant(d, init);
            }
        }
    }
    if (init != d->initializer)
        d = new IR::Declaration_Constant(d->srcInfo, d->name, d->annotations, d->type, init);
    return d;
}

const IR::Node* ConstantFolding::postorder(IR::Cmpl* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::Constant>();
    if (cst == nullptr) {
        ::error("%1%: Expected an integer value", op);
        return e;
    }
    const IR::Type* t = op->type;
    if (t->is<IR::Type_InfInt>()) {
        ::error("%1%: Operation cannot be applied to values with unknown width;\n"
                "please specify width explicitly", e);
        return e;
    }
    auto tb = t->to<IR::Type_Bits>();
    if (tb == nullptr) {
        if (typesKnown)
            ::error("%1%: Operation can only be applied to int<> or bit<> types", e);
        return e;
    }

    mpz_class value = ~cst->value;
    auto result = new IR::Constant(cst->srcInfo, t, value, cst->base, true);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::Neg* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::Constant>();
    if (cst == nullptr) {
        ::error("%1%: Expected an integer value", op);
        return e;
    }
    const IR::Type* t = op->type;
    if (t->is<IR::Type_InfInt>())
        return new IR::Constant(cst->srcInfo, t, -cst->value, cst->base);

    auto tb = t->to<IR::Type_Bits>();
    if (tb == nullptr) {
        if (typesKnown)
            ::error("%1%: Operation can only be applied to int<> or bit<> types", e);
        return e;
    }

    mpz_class value = -cst->value;
    auto result = new IR::Constant(cst->srcInfo, t, value, cst->base, true);
    setConstant(e, result);
    return result;
}

const IR::Constant*
ConstantFolding::cast(const IR::Constant* node, unsigned base, const IR::Type_Bits* type) const {
    return new IR::Constant(node->srcInfo, type, node->value, base);
}

const IR::Node* ConstantFolding::postorder(IR::Add* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a + b; });
}

const IR::Node* ConstantFolding::postorder(IR::Sub* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a - b; });
}

const IR::Node* ConstantFolding::postorder(IR::Mul* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a * b; });
}

const IR::Node* ConstantFolding::postorder(IR::BXor* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a ^ b; });
}

const IR::Node* ConstantFolding::postorder(IR::BAnd* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a & b; });
}

const IR::Node* ConstantFolding::postorder(IR::BOr* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a | b; });
}

const IR::Node* ConstantFolding::postorder(IR::Equ* e) {
    return compare(e);
}

const IR::Node* ConstantFolding::postorder(IR::Neq* e) {
    return compare(e);
}

const IR::Node* ConstantFolding::postorder(IR::Lss* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a < b; });
}

const IR::Node* ConstantFolding::postorder(IR::Grt* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a > b; });
}

const IR::Node* ConstantFolding::postorder(IR::Leq* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a <= b; });
}

const IR::Node* ConstantFolding::postorder(IR::Geq* e) {
    return binary(e, [](mpz_class a, mpz_class b) { return a >= b; });
}

const IR::Node* ConstantFolding::postorder(IR::Div* e) {
    return binary(e, [e](mpz_class a, mpz_class b)->mpz_class {
            if (sgn(a) < 0 || sgn(b) < 0) {
                ::error("%1%: Division is not defined for negative numbers", e);
                return 0;
            }
            if (sgn(b) == 0) {
                ::error("%1%: Division by zero", e);
                return 0;
            }
            return a / b;
        });
}

const IR::Node* ConstantFolding::postorder(IR::Mod* e) {
    return binary(e, [e](mpz_class a, mpz_class b)->mpz_class {
            if (sgn(a) < 0 || sgn(b) < 0) {
                ::error("%1%: Modulo is not defined for negative numbers", e);
                return 0;
            }
            if (sgn(b) == 0) {
                ::error("%1%: Modulo by zero", e);
                return 0;
            }
            return a % b; });
}

const IR::Node* ConstantFolding::postorder(IR::Shr* e) {
    return shift(e);
}

const IR::Node* ConstantFolding::postorder(IR::Shl* e) {
    return shift(e);
}

const IR::Node*
ConstantFolding::compare(const IR::Operation_Binary* e) {
    auto eleft = getConstant(e->left);
    auto eright = getConstant(e->right);
    if (eleft == nullptr || eright == nullptr)
        return e;

    bool eqTest = e->is<IR::Equ>();
    if (eleft->is<IR::BoolLiteral>()) {
        auto left = eleft->to<IR::BoolLiteral>();
        auto right = eright->to<IR::BoolLiteral>();
        if (left == nullptr || right == nullptr) {
            ::error("%1%: both operands must be Boolean", e);
            return e;
        }
        bool bresult = (left->value == right->value) == eqTest;
        auto result = new IR::BoolLiteral(e->srcInfo, bresult);
        setConstant(e, result);
        return result;
    }

    if (eqTest)
        return binary(e, [](mpz_class a, mpz_class b) { return a == b; });
    else
        return binary(e, [](mpz_class a, mpz_class b) { return a != b; });
}

const IR::Node*
ConstantFolding::binary(const IR::Operation_Binary* e,
                        std::function<mpz_class(mpz_class, mpz_class)> func) {
    auto eleft = getConstant(e->left);
    auto eright = getConstant(e->right);
    if (eleft == nullptr || eright == nullptr)
        return e;

    auto left = eleft->to<IR::Constant>();
    if (left == nullptr) {
        ::error("%1%: Expected a bit<> or int<> value", e->left);
        return e;
    }
    auto right = eright->to<IR::Constant>();
    if (right == nullptr) {
        ::error("%1%: Expected an bit<> or int<> value", e->right);
        return e;
    }

    const IR::Type* lt = left->type;
    const IR::Type* rt = right->type;
    bool lunk = lt->is<IR::Type_InfInt>();
    bool runk = rt->is<IR::Type_InfInt>();

    const IR::Type* resultType;
    mpz_class value = func(left->value, right->value);

    const IR::Type_Bits* ltb = nullptr;
    const IR::Type_Bits* rtb = nullptr;
    if (!lunk) {
        ltb = lt->to<IR::Type_Bits>();
        if (ltb == nullptr) {
            if (typesKnown)
                ::error("%1%: Operation can only be applied to int<> or bit<> types", e);
            return e;
        }
    }
    if (!runk) {
        rtb = rt->to<IR::Type_Bits>();
        if (rtb == nullptr) {
            if (typesKnown)
                ::error("%1%: Operation can only be applied to int<> or bit<> types", e);
            return e;
        }
    }

    if (!lunk && !runk) {
        // both typed
        if (!ltb->operator==(*rtb)) {
            ::error("%1%: operands have different types: %2% and %3%",
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

    const IR::Expression* result;
    if (e->is<IR::Operation_Relation>())
        result = new IR::BoolLiteral(e->srcInfo, static_cast<bool>(value));
    else
        result = new IR::Constant(e->srcInfo, resultType, value, left->base);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::LAnd* e) {
    auto left = getConstant(e->left);
    if (left == nullptr)
        return e;

    auto lcst = left->to<IR::BoolLiteral>();
    if (lcst == nullptr) {
        ::error("%1%: Expected a boolean value", left);
        return e;
    }

    if (lcst->value) {
        setConstant(e, e->right);
        return e->right;
    }

    // Short-circuit folding
    auto result = new IR::BoolLiteral(left->srcInfo, false);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::LOr* e) {
    auto left = getConstant(e->left);
    if (left == nullptr)
        return e;

    auto lcst = left->to<IR::BoolLiteral>();
    if (lcst == nullptr) {
        ::error("%1%: Expected a boolean value", left);
        return e;
    }

    if (!lcst->value) {
        setConstant(e, e->right);
        return e->right;
    }

    // Short-circuit folding
    auto result = new IR::BoolLiteral(left->srcInfo, true);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::Slice* e) {
    const IR::Expression* msb = getConstant(e->e1);
    const IR::Expression* lsb = getConstant(e->e2);
    if (msb == nullptr || lsb == nullptr) {
        ::error("%1%: bit indexes must be compile-time constants", e);
        return e;
    }

    if (!typesKnown)
        return e;
    auto e0 = getConstant(e->e0);
    if (e0 == nullptr)
        return e;

    auto cmsb = msb->to<IR::Constant>();
    if (cmsb == nullptr) {
        ::error("%1%: Expected an integer value", msb);
        return e;
    }
    auto clsb = lsb->to<IR::Constant>();
    if (clsb == nullptr) {
        ::error("%1%: Expected an integer value", lsb);
        return e;
    }
    auto cbase = e0->to<IR::Constant>();
    if (cbase == nullptr) {
        ::error("%1%: Expected an integer value", e->e0);
        return e;
    }

    int m = cmsb->asInt();
    int l = clsb->asInt();
    if (m < l) {
        ::error("%1%: bit slicing should be specified as [msb:lsb]", e);
        return e;
    }
    if (m > P4CConfiguration::MaximumWidthSupported ||
        l > P4CConfiguration::MaximumWidthSupported) {
        ::error("%1%: Compiler only supports widths up to %2%",
                e, P4CConfiguration::MaximumWidthSupported);
        return e;
    }
    mpz_class value = cbase->value >> l;
    mpz_class mask = 1;
    mask = mask << (m - l + 1) - 1;
    value = value & mask;
    auto resultType = typeMap->getType(getOriginal(), true);
    if (!resultType->is<IR::Type_Bits>())
        BUG("Type of slice is not Type_Bits, but %1%", resultType);
    auto result = new IR::Constant(e->srcInfo, resultType, value, cbase->base, true);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::Member* e) {
    if (!typesKnown)
        return e;
    auto expr = getConstant(e->expr);
    if (expr == nullptr)
        return e;
    auto type = typeMap->getType(e->expr, true);
    if (!type->is<IR::Type_StructLike>())
        BUG("Expected a struct type, got %1%", type);
    if (!expr->is<IR::ListExpression>())
        BUG("Expected a list of constants, got %1%", expr);

    auto list = expr->to<IR::ListExpression>();
    auto structType = type->to<IR::Type_StructLike>();

    bool found = false;
    int index = 0;
    for (auto f : *structType->fields) {
        if (f->name.name == e->member.name) {
            found = true;
            break;
        }
        index++;
    }

    if (!found)
        BUG("Could not find field %1% in type %2%", e->member, type);
    auto result = list->components->at(index)->clone();
    auto origtype = typeMap->getType(getOriginal());
    typeMap->setType(result, origtype);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::Concat* e) {
    auto eleft = getConstant(e->left);
    auto eright = getConstant(e->right);
    if (eleft == nullptr || eright == nullptr)
        return e;

    auto left = eleft->to<IR::Constant>();
    if (left == nullptr) {
        ::error("%1%: Expected a bit<> or int<> value", e->left);
        return e;
    }
    auto right = eright->to<IR::Constant>();
    if (right == nullptr) {
        ::error("%1%: Expected an bit<> or int<> value", e->right);
        return e;
    }

    auto lt = left->type->to<IR::Type_Bits>();
    auto rt = right->type->to<IR::Type_Bits>();
    if (lt == nullptr || rt == nullptr) {
        ::error("%1%: both operand widths must be known", e);
        return e;
    }
    if (!lt->operator==(*rt)) {
        ::error("%1%: operands have different types: %2% and %3%",
                e, lt->toString(), rt->toString());
        return e;
    }

    auto resultType = IR::Type_Bits::get(Util::SourceInfo(), lt->size + rt->size, lt->isSigned);
    mpz_class value = Util::shift_left(left->value, static_cast<unsigned>(lt->size)) + right->value;
    auto result = new IR::Constant(e->srcInfo, resultType, value, left->base);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::postorder(IR::LNot* e) {
    auto op = getConstant(e->expr);
    if (op == nullptr)
        return e;

    auto cst = op->to<IR::BoolLiteral>();
    if (cst == nullptr) {
        ::error("%1%: Expected a boolean value", op);
        return e;
    }

    auto result = new IR::BoolLiteral(cst->srcInfo, !cst->value);
    setConstant(e, result);
    return result;
}

const IR::Node* ConstantFolding::shift(const IR::Operation_Binary* e) {
    auto right = getConstant(e->right);
    if (right == nullptr)
        return e;

    auto cr = right->to<IR::Constant>();
    if (cr == nullptr) {
        ::error("%1%: Expected an integer value", right);
        return e;
    }
    if (sgn(cr->value) < 0) {
        ::error("%1%: Shifts with negative amounts are not permitted", e);
        return e;
    }

    if (sgn(cr->value) == 0) {
        // ::warning("%1% with zero", e);
        setConstant(e, e->left);
        return e->left;
    }

    auto left = getConstant(e->left);
    if (left == nullptr)
        return e;

    auto cl = left->to<IR::Constant>();
    if (cl == nullptr) {
        ::error("%1%: Expected an integer value", left);
        return e;
    }

    mpz_class value = cl->value;
    unsigned shift = static_cast<unsigned>(cr->asInt());

    auto tb = left->type->to<IR::Type_Bits>();
    if (tb != nullptr) {
        if ((unsigned)tb->size < shift)
            ::warning("%1%: Shifting %2%-bit value with %3%", e, tb->size, shift);
    }

    if (e->is<IR::Shl>())
        value = Util::shift_left(value, shift);
    else
        value = Util::shift_right(value, shift);
    auto result = new IR::Constant(e->srcInfo, left->type, value, cl->base);
    setConstant(e, result);
    return result;
}

const IR::Node *ConstantFolding::postorder(IR::Cast *e) {
    auto expr = getConstant(e->expr);
    if (expr == nullptr)
        return e;

    const IR::Type* etype;
    if (typesKnown)
        etype = typeMap->getType(getOriginal(), true);
    else
        etype = e->type;

    if (etype->is<IR::Type_Bits>()) {
        auto type = etype->to<IR::Type_Bits>();
        auto arg = expr->to<IR::Constant>();
        auto result = cast(arg, arg->base, type);
        setConstant(e, result);
        return result;
    } else if (etype->is<IR::Type_StructLike>()) {
        auto result = expr->clone();
        auto origtype = typeMap->getType(getOriginal());
        typeMap->setType(result, origtype);
        setConstant(e, result);
        return result;
    }

    return e;
}

}  // namespace P4
