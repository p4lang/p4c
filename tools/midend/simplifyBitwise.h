#ifndef MIDEND_SIMPLIFYBITWISE_H_
#define MIDEND_SIMPLIFYBITWISE_H_

#include "ir/ir.h"
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
 * which in term could be further simplified to a vector of simple assignments over slices.
 * This extensions could be folded to any combinations of Binary Ors and Binary Ands as long
 * as the masks never have any collisions.
 *
 * @pre none
 *
 * @todo: Extend the optimization to handle multiple combinations of masks
 */
class SimplifyBitwise : public Transform {
    IR::Vector<IR::StatOrDecl> *slice_statements;
    const IR::AssignmentStatement *changing_as;

    void assignSlices(const IR::Expression *expr, mpz_class mask);

 public:
    const IR::Node *preorder(IR::AssignmentStatement *as) override;
};

}  // namespace P4

#endif /* MIDEND_SIMPLIFYBITWISE_H_ */
