#include "removeLeftSlices.h"

namespace P4 {

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

    auto cast = new IR::Cast(right->srcInfo, stat->right, type);
    auto sh = new IR::Shl(right->srcInfo, cast, new IR::Constant(l));
    auto and2 = new IR::BAnd(right->srcInfo, sh, mask);
    auto rhs = new IR::BOr(right->srcInfo, and1, and2);
    auto result = new IR::AssignmentStatement(stat->srcInfo, ls->e0, rhs);
    LOG1("Replaced " << stat << " with " << result);
    return result;
}

}  // namespace P4
