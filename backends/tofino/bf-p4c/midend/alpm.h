/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/**
 * \defgroup alpm BFN::AlpmImplementation
 * \ingroup midend
 * \brief Set of passes that implement ALPM.
 */
#ifndef BF_P4C_MIDEND_ALPM_H_
#define BF_P4C_MIDEND_ALPM_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace BFN {

/**
 * \ingroup alpm
 */
class CollectAlpmInfo : public Inspector {
 public:
    std::set<cstring> alpm_actions;
    std::set<cstring> alpm_table;
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;

 public:
    CollectAlpmInfo(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
        refMap(refMap), typeMap(typeMap) {}

    void postorder(const IR::P4Table* tbl) override;
};

/**
 * \ingroup alpm
 */
class HasTableApply : public Inspector {
    P4::ReferenceMap* refMap;
    P4::TypeMap*      typeMap;

 public:
    const IR::P4Table*  table;
    const IR::MethodCallExpression* call;
    HasTableApply(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
        refMap(refMap), typeMap(typeMap), table(nullptr), call(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("HasTableApply"); }
    void postorder(const IR::MethodCallExpression* expression) override {
        auto mi = P4::MethodInstance::resolve(expression, refMap, typeMap);
        if (!mi->isApply()) return;
        auto am = mi->to<P4::ApplyMethod>();
        if (!am->object->is<IR::P4Table>()) return;
        BUG_CHECK(table == nullptr, "%1% and %2%: multiple table applications in one expression",
                  table, am->object);
        table = am->object->to<IR::P4Table>();
        call = expression;
        LOG3("Invoked table is " << dbp(table));
    }
    bool preorder(const IR::Member* member) override {
        if (member->member == "hit" || member->member == "miss")
            return true;
        return false;
    }
};

/**
 * \ingroup alpm
 * \brief Pass that splits ALPM table into pre-classifier TCAM and TCAM.
 * 
 * This class implements ALPM as a pair of pre-classifer tcam table and an
 * algorithmic tcam table. Currently, this class supports two different
 * implementations selected based on the constructor parameters to the Alpm
 * extern.
 *
 * 1. The following:
 * 
 *        Alpm(number_partitions = 4095, subtrees_per_partition=2) alpm;
 *        table alpm {
 *            key = { f0 : exact; f1 : lpm }
 *            actions = { ... }
 *            alpm = alpm;
 *        }
 *
 *    is translated to:
 * 
 *        table alpm_preclassifier {
 *            key = { f0 : exact; f1 : lpm }
 *            actions = { set_partition_index; }
 *         }
 *         table alpm {
 *             key = { f0 : exact; f1 : lpm; partition_index : atcam_partition_index; }
 *             actions = { ... }
 *         }
 *
 * 
 * 2. The following:
 *        
 *        Alpm(number_partitions = 4095, subtrees_per_partition=2,
 *             atcam_subset_width = 12, shift_granularity=1) alpm;
 *
 *    is translated to:
 * 
 *        action set_partition_index_0 ( bit<..> pidx, bit<..> pkey ) {
 *            partition_index = pidx;
 *            partition_key = pkey;
 *        }
 *
 *        ...
 *
 *        table alpm_preclassifier {
 *            key = { f0 : exact; f1 : lpm {
 *            actions = { set_partition_index_0; set_partition_index_1; ... }
 *        }
 *
 *        table alpm {
 *            key = { partition_key : lpm; partition_index : atcam_partition_index; }
 *            actions = { ... }
 *        }
 *
 */
class SplitAlpm : public Transform {
    CollectAlpmInfo* alpm_info;
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    static const std::set<unsigned> valid_partition_values;
    std::set<cstring> alpm_tables;
    std::map<cstring, cstring> preclassifier_tables;

    const IR::MAU::Table* create_pre_classifier_tcam(
            IR::MAU::Table* tbl, IR::TempVar* tv, IR::TempVar* tk,
            unsigned partition_index_bits,
            unsigned pre_classifer_number_entries,
            unsigned pre_classifer_number_actions);
    bool values_through_pragmas(const IR::P4Table *tbl, int &number_of_partitions,
            int &number_subtrees_par_partition);
    bool values_through_impl(const IR::P4Table *tbl, int &number_of_partitions,
            int &number_subtrees_per_partition, int &atcam_subset_width,
            int &shift_granularity);
    bool pragma_exclude_msbs(const IR::P4Table* tbl,
            ordered_map<cstring, int>& fields_to_exclude);

    const IR::Node* postorder(IR::P4Table* tbl) override;
    const IR::Node* postorder(IR::MethodCallStatement*) override;
    const IR::Node* postorder(IR::IfStatement*) override;
    const IR::Node* postorder(IR::SwitchStatement*) override;

    const IR::IndexedVector<IR::Declaration>* create_temp_var(
            const IR::P4Table*, unsigned, unsigned, unsigned, unsigned,
            const IR::Expression*);
    const IR::IndexedVector<IR::Declaration>* create_preclassifer_actions(
            const IR::P4Table*, unsigned, unsigned, unsigned,
            unsigned, unsigned, const IR::Expression*);
    const IR::P4Table* create_preclassifier_table(const IR::P4Table*, unsigned,
            unsigned, unsigned, unsigned);
    const IR::P4Table* create_atcam_table(const IR::P4Table*, unsigned,
            unsigned, unsigned, int, int,
            ordered_map<cstring, int>& fields_to_exclude,
            const IR::Expression*);
    void apply_pragma_exclude_msbs(const IR::P4Table* tbl,
            const ordered_map<cstring, int>* fields_to_exclude,
            IR::Vector<IR::KeyElement>& keys);
    const IR::StatOrDecl* synth_funnel_shift_ops(const IR::P4Table*, const IR::Expression*, int);
    bool use_funnel_shift(int);

 public:
    static const cstring ALGORITHMIC_LPM_PARTITIONS;
    static const cstring ALGORITHMIC_LPM_SUBTREES_PER_PARTITION;
    static const cstring ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS;

    SplitAlpm(CollectAlpmInfo *info, P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
        alpm_info(info), refMap(refMap), typeMap(typeMap) {}
};

/**
 * \ingroup alpm
 * \brief Top level PassManager that governs the ALPM implementation.
 */
class AlpmImplementation : public PassManager {
 public:
    AlpmImplementation(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);
};

}  // namespace BFN

#endif  /* BF_P4C_MIDEND_ALPM_H_ */


