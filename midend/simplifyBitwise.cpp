#include "simplifyBitwise.h"
#include "ir/pattern.h"

namespace P4 {

void SimplifyBitwise::assignSlices(const IR::Expression *expr, mpz_class mask) {
    int one_pos = mpz_scan1(mask.get_mpz_t(), 0);

    // Calculate the slices for this particular mask
    while (one_pos >= 0) {
        int zero_pos = mpz_scan0(mask.get_mpz_t(), one_pos);
        auto left_slice = IR::Slice::make(changing_as->left, zero_pos - 1, one_pos);
        auto right_slice = IR::Slice::make(expr, zero_pos - 1, one_pos);
        auto new_as = new IR::AssignmentStatement(changing_as->srcInfo, left_slice, right_slice);
        slice_statements->push_back(new_as);
        one_pos = mpz_scan1(mask.get_mpz_t(), zero_pos);
    }
}

const IR::Node *SimplifyBitwise::preorder(IR::AssignmentStatement *as) {
    const IR::Expression *a, *b;
    const IR::Constant *maskA, *maskB;

    if (!((Pattern(a) & Pattern(maskA)) | (Pattern(b) & Pattern(maskB))).match(as->right))
        return as;
    if ((maskA->value & maskB->value) != 0)
        return as;

    slice_statements = new IR::Vector<IR::StatOrDecl>();
    assignSlices(a, maskA->value);
    assignSlices(b, maskB->value);
    mpz_class parameter_mask = (mpz_class(1) << (as->left->type->width_bits())) - 1;
    parameter_mask &= ~maskA->value;
    parameter_mask &= ~maskB->value;
    if (parameter_mask != 0)
        assignSlices(new IR::Constant(0), parameter_mask);
    return slice_statements;
}

}  // namespace P4
