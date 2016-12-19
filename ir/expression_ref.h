#ifndef IR_EXPRESSION_REF_H_
#define IR_EXPRESSION_REF_H_

#include "ir.h"

namespace IR {

class ERef {
    const Expression *expr;

    template<class T> class ERef_specific {
        const T *expr;
        friend class ERef;

     public:
        ERef_specific(const T *e) : expr(e) {}  // NOLINT(runtime/explicit)
        const T &operator*() const { return *expr; }
        const T *operator->() const { return expr; }
        operator const T *() const { return expr; }
    };

 public:
    ERef(const Expression *e) : expr(e) {}   // NOLINT(runtime/explicit)
    ERef(const Expression &e) : expr(&e) {}   // NOLINT(runtime/explicit)
    template<class T> ERef(ERef_specific<T> e) : expr(e) {}   // NOLINT(runtime/explicit)
    ERef(int v) : expr(new Constant(v)) {}   // NOLINT(runtime/explicit)
    ERef(unsigned v) : expr(new Constant(v)) {}   // NOLINT(runtime/explicit)
    ERef(long v) : expr(new Constant(v)) {}   // NOLINT(runtime/explicit)
    ERef(unsigned long v) : expr(new Constant(v)) {}   // NOLINT(runtime/explicit)
    ERef(bool v) : expr(new BoolLiteral(v)) {}   // NOLINT(runtime/explicit)
    operator const Expression *() const { return expr; }
    const Expression &operator *() const { return *expr; }
    const Expression *operator ->() const { return expr; }
    ERef operator[](ERef idx) const { return new ArrayIndex(expr, idx); }
    ERef operator-() const { return new Neg(expr); }
    ERef operator~() const { return new Cmpl(expr); }
    ERef operator!() const { return new LNot(expr); }
    ERef operator+(ERef a) const { return new Add(expr, a); }
    ERef operator-(ERef a) const { return new Sub(expr, a); }
    ERef operator*(ERef a) const { return new Mul(expr, a); }
    ERef operator/(ERef a) const { return new Div(expr, a); }
    ERef operator%(ERef a) const { return new Mod(expr, a); }
    ERef operator<<(ERef a) const { return new Shl(expr, a); }
    ERef operator>>(ERef a) const { return new Shr(expr, a); }
    ERef operator==(ERef a) const { return new Equ(expr, a); }
    ERef operator!=(ERef a) const { return new Neq(expr, a); }
    ERef operator<=(ERef a) const { return new Leq(expr, a); }
    ERef operator>=(ERef a) const { return new Geq(expr, a); }
    ERef operator<(ERef a) const { return new Lss(expr, a); }
    ERef operator>(ERef a) const { return new Grt(expr, a); }
    ERef operator&(ERef a) const { return new BAnd(expr, a); }
    ERef operator|(ERef a) const { return new BOr(expr, a); }
    ERef operator^(ERef a) const { return new BXor(expr, a); }
    ERef operator&&(ERef a) const { return new LAnd(expr, a); }
    ERef operator||(ERef a) const { return new LOr(expr, a); }
    ERef Slice(ERef hi, ERef lo) const { return new IR::Slice(expr, hi, lo); }
    ERef Range(ERef to) const { return new IR::Range(expr, to); }
    ERef Concat(ERef a) const { return new IR::Concat(expr, a); }
    ERef Mask(ERef mask) const { return new IR::Mask(expr, mask); }
    ERef Mux(ERef t, ERef f) const { return new IR::Mux(expr, t, f); }
    ERef_specific<IR::Member> Member(Util::SourceInfo srcInfo, IR::ID name) const {
        return new IR::Member(srcInfo + expr->srcInfo + name.srcInfo, expr, name); }
    ERef_specific<IR::Member> Member(IR::ID name) const {
        return new IR::Member(expr->srcInfo + name.srcInfo, expr, name); }
    ERef_specific<MethodCallExpression> operator()(const Vector<Type> *targs = nullptr,
                                                   const Vector<Expression> *args = nullptr) const {
        auto srcInfo = expr->srcInfo;
        if (args) {
            srcInfo += args->srcInfo;
            if (!args->empty()) srcInfo += args->back()->srcInfo;
        } else if (targs) {
            srcInfo += targs->srcInfo;
            if (!targs->empty()) srcInfo += targs->back()->srcInfo; }
        return new MethodCallExpression(srcInfo, expr, targs ? targs : new Vector<Type>(),
                                        args ? args : new Vector<Expression>()); }
    ERef_specific<MethodCallExpression> operator()(const Vector<Expression> *args) const {
        return (*this)(nullptr, args); }
};

}  // end namespace IR

#endif /* IR_EXPRESSION_REF_H_ */
