#include "simplifyBitwise.h"

#include <string>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "ir/pattern.h"
#include "lib/cstring.h"

namespace P4 {

void SimplifyBitwise::assignSlices(const IR::Expression *expr, big_int mask) {
    int one_pos = Util::scan1(mask, 0);

    // Calculate the slices for this particular mask
    while (one_pos >= 0) {
        int zero_pos = Util::scan0(mask, one_pos);
        auto left_slice = IR::Slice::make(changing_as->left, one_pos, zero_pos - 1);
        auto right_slice = IR::Slice::make(expr, one_pos, zero_pos - 1);
        auto new_as = new IR::AssignmentStatement(changing_as->srcInfo, left_slice, right_slice);
        slice_statements->push_back(new_as);
        one_pos = Util::scan1(mask, zero_pos);
    }
}

const IR::Node *SimplifyBitwise::preorder(IR::AssignmentStatement *as) {
    Pattern::Match<IR::Expression> a, b;
    Pattern::Match<IR::Constant> maskA, maskB;

    if (!((a & maskA) | (b & maskB)).match(as->right)) return as;
    if ((maskA->value & maskB->value) != 0) return as;

    changing_as = as;
    slice_statements = new IR::Vector<IR::StatOrDecl>();
    assignSlices(a, maskA->value);
    assignSlices(b, maskB->value);
    big_int parameter_mask = (big_int(1) << (as->left->type->width_bits())) - 1;
    parameter_mask &= ~maskA->value;
    parameter_mask &= ~maskB->value;
    if (parameter_mask != 0) assignSlices(new IR::Constant(0), parameter_mask);
    return slice_statements;
}

}  // namespace P4
