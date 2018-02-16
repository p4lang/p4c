#include "simplifyBitwise.h"

namespace P4 {

bool SimplifyBitwise::Scan::preorder(const IR::AssignmentStatement *as) {
    if (!as->right->is<IR::BOr>())
        return false;
    can_be_changed = true;
    mpz_init(total_mask.get_mpz_t());
    return true;
}

bool SimplifyBitwise::Scan::preorder(const IR::BOr *bor) {
    if (!findContext<IR::AssignmentStatement>())
        can_be_changed = false;
    if (!(bor->left->is<IR::BAnd>() && bor->right->is<IR::BAnd>()))
        can_be_changed = false;
    return can_be_changed;
}

bool SimplifyBitwise::Scan::preorder(const IR::BAnd *band) {
    if (!findContext<IR::BOr>())
        can_be_changed = false;
    if (!band->left->is<IR::Constant>() && !band->right->is<IR::Constant>())
        can_be_changed = false;
    if (band->left->is<IR::Constant>() && band->right->is<IR::Constant>())
        can_be_changed = false;
    return can_be_changed;
}

bool SimplifyBitwise::Scan::preorder(const IR::Constant *constant) {
    if (!findContext<IR::BAnd>()) {
        can_be_changed = false;
        return false;
    }
    // Ensure that there are no collisions within the bitmask
    mpz_class mask;
    mpz_init(mask.get_mpz_t());
    mpz_class zero;
    mpz_init(zero.get_mpz_t());
    mpz_and(mask.get_mpz_t(), total_mask.get_mpz_t(), constant->value.get_mpz_t());
    if (mpz_cmp(mask.get_mpz_t(), zero.get_mpz_t()) == 0) {
        mpz_ior(total_mask.get_mpz_t(), mask.get_mpz_t(), total_mask.get_mpz_t());
    } else {
        can_be_changed = false;
    }
    return false;
}

void SimplifyBitwise::Scan::postorder(const IR::AssignmentStatement *as) {
    if (can_be_changed)
        self.changeable.insert(as);
}

const IR::Node *SimplifyBitwise::Update::preorder(IR::AssignmentStatement *as) {
    auto orig_as = getOriginal()->to<IR::AssignmentStatement>();
    if (self.changeable.count(orig_as) == 0) {
        prune();
    } else {
        slice_statements = new IR::Vector<IR::StatOrDecl>();
        changing_as = as;
    }
    return as;
}


const IR::Node *SimplifyBitwise::Update::preorder(IR::BAnd *band) {
    if (!findContext<IR::AssignmentStatement>())
        return band;
    const IR::Constant *constant = band->left->to<IR::Constant>();
    const IR::Expression *rvalue = band->right;
    if (constant == nullptr) {
        constant = band->right->to<IR::Constant>();
        rvalue = band->left;
    }
    BUG_CHECK(constant, "%s: No singular mask field in & compilation: %s", band->srcInfo, band);
    mp_bitcnt_t zero_pos = 0;
    mp_bitcnt_t one_pos = mpz_scan1(constant->value.get_mpz_t(), zero_pos);
    while (static_cast<int>(one_pos) >= 0) {
        zero_pos = mpz_scan0(constant->value.get_mpz_t(), one_pos);
        auto left_slice = new IR::Slice(changing_as->left, static_cast<int>(zero_pos - 1),
                                        static_cast<int>(one_pos));
        auto right_slice = new IR::Slice(rvalue, static_cast<int>(zero_pos - 1),
                                         static_cast<int>(one_pos));
        auto new_as = new IR::AssignmentStatement(changing_as->srcInfo, left_slice, right_slice);
        slice_statements->push_back(new_as);
        one_pos = mpz_scan1(constant->value.get_mpz_t(), zero_pos);
    }
    return band;
}

const IR::Node *SimplifyBitwise::Update::postorder(IR::AssignmentStatement *as) {
    BUG_CHECK(!slice_statements->empty(), "%s: Must have created separate assignment statements "
              "for this statement: %s", as->srcInfo, as);
    return slice_statements;
}

}  // namespace P4
