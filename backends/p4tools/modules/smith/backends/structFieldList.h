#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_BACKENDS_STRUCTFIELDLIST_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_BACKENDS_STRUCTFIELDLIST_H_

#include <set>
#include <vector>

#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

class structFieldList {
 public:
    structFieldList() {}

    static IR::IndexedVector<IR::StructField> gen_sm();
    static IR::IndexedVector<IR::StructField> gen_tf_ing_md_t();
    static IR::IndexedVector<IR::StructField> gen_tf_ing_md_for_tm_t();
    static IR::IndexedVector<IR::StructField> gen_tf_ing_intr_md_from_prsr();
    static IR::IndexedVector<IR::StructField> gen_tf_ing_intr_md_for_deprsr();
    static IR::IndexedVector<IR::StructField> gen_tf_eg_intr_md_t();
    static IR::IndexedVector<IR::StructField> gen_tf_eg_intr_md_from_prsr();
    static IR::IndexedVector<IR::StructField> gen_tf_eg_intr_md_for_deprsr();
    static IR::IndexedVector<IR::StructField> gen_tf_eg_intr_md_for_output_port();
};
}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_BACKENDS_STRUCTFIELDLIST_H_ */
