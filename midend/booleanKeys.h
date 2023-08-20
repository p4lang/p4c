#ifndef MIDEND_BOOLEANKEYS_H_
#define MIDEND_BOOLEANKEYS_H_

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4 {

/// Adds a bit<1> cast to all boolean keys. This is because these keys need to be converted into bit
/// expressions for the control plane. Also replaces all the corresponding entries in the table with
/// the appropriate expression.
/// Example:
/**
 *
 * \code{.cpp}
 *  table t {
 *    key = { h.a  : exact; }
 *    const entries = {
 *           true : action();
 +        }
 *  }
 *  apply {
 *    t.apply();
 *  }
 * \endcode
 *
 * is transformed to
 *
 * \code{.cpp}
 *  table t {
 *    key = { (bit<1>) h.a  : exact; }
 *    const entries = {
 *           1w1 : action();
 +        }
 *  }
 *  apply {
 *    t.apply();
 *  }
 * \endcode
 *
 * @pre none
 * @post all boolean table key expressions are replaced with a bit<1> cast. Constant entries are
 * replaced with 1w1 (true) or 1w0 (false).
 */

class CastBooleanTableKeys : public Transform {
 public:
    CastBooleanTableKeys() { setName("CastBooleanTableKeys"); }

    const IR::Node *postorder(IR::KeyElement *key) override;

    const IR::Node *postorder(IR::Entry *entry) override;
};

}  // namespace P4

#endif /* MIDEND_BOOLEANKEYS_H_ */
