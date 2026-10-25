#ifndef MIDEND_SIMPLIFYBITWISE_H_
#define MIDEND_SIMPLIFYBITWISE_H_

#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/ordered_set.h"

namespace P4 {

/**
 * The purpose of this pass is to simplify the translation of the p4-16 translation of the
 * following p4_14 primitive:
 *     modify_field(hdr.field, parameter, mask);
 *
 * This gets translated to the following p4_16:
 *     hdr.field = hdr.field & ~mask | parameter & mask;
 *
 * which in term can be further simplified to a vector of simple assignments over slices.
 * This extensions could be folded to any combinations of Binary Ors and Binary Ands as long
 * as the masks never have any collisions.
 *
 * Currently we deal with any assignment of the form
 *
 *    dest = (srcA & maskA) | (srcB & maskB);
 *
 * where `maskA' and `maskB' are constants such that maskA & maskB == 0.
 * This gets converted into
 *
 *    dest[slice_A1] = srcA[slice_A1]
 *                   :
 *    dest[slice_An] = srcA[slice_An]
 *    dest[slice_B1] = srcA[slice_B1]
 *                   :
 *    dest[slice_Bn] = srcA[slice_Bn]
 *    dest[slice_X1] = 0
 *                   :
 *
 * where the slice_Ai/Bi values are slices corresponding to each range of contiguous 1 bits
 * in maskA and maskB, and the slice_Xi values are any remaing bits where both masks are 0.
 * For example if maskA == 0xff00ff and maskB = 0xff00 they will be:
 *
 *    slice_A1 == 7:0
 *    slice_A2 == 23:16
 *    slice_B1 == 15:8
 *    slice_X1 == 31:24   (assuming bit<32> types involved)
 *
 * Naturally, if there are no uncovered bits, there will be no X slices.  The most common
 * case will end up with one A slice and one B slice and no X slice, but if the masks are
 * sparse/pessimal this will generate a lot of small slices which may be worse than the
 * original code, so perhaps there should be a knob targets can use to limit that.
 *
 * This works equally well for `&=', `|=' and `^=' as it does for simple assignments.
 * Any resulting `dest[slice] |= 0' or `des[slice] ^= 0' should later be eliminated by
 * constant folding.
 *
 * @pre none
 *
 * @todo: Extend the optimization to handle multiple combinations of masks
 */
class SimplifyBitwise : public Transform {
    IR::Vector<IR::StatOrDecl> *slice_statements = nullptr;
    const IR::BaseAssignmentStatement *changing_as = nullptr;

    void assignSlices(const IR::Expression *expr, big_int mask);

 public:
    const IR::Node *preorder(IR::BaseAssignmentStatement *as) override;
    const IR::Node *preorder(IR::OpAssignmentStatement *as) override { return as; }
    const IR::Node *preorder(IR::BAndAssign *as) override {
        return preorder(static_cast<IR::BaseAssignmentStatement *>(as));
    }
    const IR::Node *preorder(IR::BOrAssign *as) override {
        return preorder(static_cast<IR::BaseAssignmentStatement *>(as));
    }
    const IR::Node *preorder(IR::BXorAssign *as) override {
        return preorder(static_cast<IR::BaseAssignmentStatement *>(as));
    }
};

}  // namespace P4

#endif /* MIDEND_SIMPLIFYBITWISE_H_ */
