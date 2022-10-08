#ifndef COMMON_COMPILER_BOOLEAN_KEYS_H_
#define COMMON_COMPILER_BOOLEAN_KEYS_H_

#include "ir/ir.h"

namespace P4Tools {

/// Adds a bit<1> cast to all boolean keys. This is because these keys need to be converted into bit
/// expressions for the control plane. Also replaces all the corresponding entries in the table with
/// the corresponding expression.
class CastBooleanTableKeys : public Transform {
 public:
    CastBooleanTableKeys() { setName("CastBooleanTableKeys"); }

    const IR::Node* postorder(IR::KeyElement* key) override;
    const IR::Node* postorder(IR::Entry* entry) override;
};

}  // namespace P4Tools

#endif /* COMMON_COMPILER_BOOLEAN_KEYS_H_ */
