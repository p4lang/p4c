#include "lower.h"
#include "lib/gmputil.h"

namespace BMV2 {

const IR::Node* RemoveLeftSlices::postorder(IR::AssignmentStatement* stat) {
    if (!stat->left->is<IR::Slice>())
        return stat;
    auto ls = stat->left->to<IR::Slice>();
    int h = ls->getH();
    int l = ls->getL();
    mpz_class m = Util::maskFromSlice(h, l);
    auto type = typeMap->getType(ls->e0, true);
    auto mask = new IR::Constant(ls->srcInfo, type, m, 16);

    auto right = stat->right;
    // Handle a[m:l] = e;  ->  a = (a & ~mask) | (((cast)e << l) & mask);
    auto cmpl = new IR::Cmpl(ls->srcInfo, mask);
    auto and1 = new IR::BAnd(ls->srcInfo, ls->e0, cmpl);

    auto cast = new IR::Cast(right->srcInfo, type, stat->right);
    auto sh = new IR::Shl(right->srcInfo, cast, new IR::Constant(l));
    auto and2 = new IR::BAnd(right->srcInfo, sh, mask);
    auto rhs = new IR::BOr(right->srcInfo, and1, and2);
    auto result = new IR::AssignmentStatement(stat->srcInfo, ls->e0, rhs);
    LOG1("Replaced " << stat << " with " << result);
    return result;
}

const IR::Node* LowerExpressions::postorder(IR::Slice* expression) {
    // This is in a RHS expression  a[m:l]  ->  (cast)(a >> l)
    int h = expression->getH();
    int l = expression->getL();
    auto sh = new IR::Shr(expression->e0->srcInfo, expression->e0, new IR::Constant(l));
    auto type = IR::Type_Bits::get(h - l + 1);
    auto result = new IR::Cast(expression->srcInfo, type, sh);
    LOG1("Replaced " << expression << " with " << result);
    return result;
}

const IR::Node* LowerExpressions::postorder(IR::Concat* expression) {
    // a ++ b  -> ((cast)a << sizeof(b)) | ((cast)b & mask)
    auto type = typeMap->getType(expression->right, true);
    auto resulttype = typeMap->getType(expression, true);
    BUG_CHECK(type->is<IR::Type_Bits>(), "%1%: expected a bitstring got a %2%",
              expression->right, type);
    BUG_CHECK(resulttype->is<IR::Type_Bits>(), "%1%: expected a bitstring got a %2%",
              expression->right, type);
    unsigned sizeofb = type->to<IR::Type_Bits>()->size;
    unsigned sizeofresult = resulttype->to<IR::Type_Bits>()->size;
    auto cast0 = new IR::Cast(expression->left->srcInfo, resulttype, expression->left);
    auto cast1 = new IR::Cast(expression->right->srcInfo, resulttype, expression->right);

    auto sh = new IR::Shl(cast0->srcInfo, cast0, new IR::Constant(sizeofb));
    mpz_class m = Util::maskFromSlice(sizeofb, 0);
    auto mask = new IR::Constant(expression->right->srcInfo,
                                 IR::Type_Bits::get(sizeofresult), m, 16);
    auto and0 = new IR::BAnd(expression->right->srcInfo, cast1, mask);
    auto result = new IR::BOr(expression->srcInfo, sh, and0);
    return result;
}

}  // namespace BMV2
