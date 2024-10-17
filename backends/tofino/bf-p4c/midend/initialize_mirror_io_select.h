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

#ifndef INITIALIZE_MIRROR_IO_SELECT_H_
#define INITIALIZE_MIRROR_IO_SELECT_H_

#include "bf-p4c/common/pragma/all_pragmas.h"
#include "ir/ir.h"
#include "bf-p4c/arch/arch.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/device.h"
#include "type_checker.h"

namespace BFN {

// Initialize eg_intr_md_for_dprsr.mirror_io_select
class DoInitializeMirrorIOSelect: public Transform {
    cstring egIntrMdForDprsrName;

 public:
    DoInitializeMirrorIOSelect() {};

    // disable this pass if the @disable_egress_mirror_io_select_initialization pragma is used
    const IR::Node* preorder(IR::P4Program* p) override {
        // collect and set global_pragmas
        CollectGlobalPragma collect_pragma;
        p->apply(collect_pragma);
        // Workaround can be disabled by pragma
        if (collect_pragma.exists(PragmaDisableEgressMirrorIOSelectInitialization::name)) {
            prune();
        }
        return p;
    }

    // Add mirror_io_select initialization to egress parser
    const IR::Node* preorder(IR::BFN::TnaParser* parser) override;
    const IR::Node* preorder(IR::ParserState* state) override;

    // Skip controls and deparsers
    const IR::Node* preorder(IR::BFN::TnaControl* control) override { prune(); return control; };
    const IR::Node* preorder(IR::BFN::TnaDeparser* deparser) override { prune(); return deparser; };
};

/**
 * @ingroup midend
 * @ingroup parde
 * @brief Initializes eg_intr_md_for_dprsr.mirror_io_select on devices except Tofino1
 */
class InitializeMirrorIOSelect : public PassManager {
 public:
    InitializeMirrorIOSelect(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        addPasses({
            new PassIf([]() {
                return Device::hasMirrorIOSelect(); }, {
                new DoInitializeMirrorIOSelect(),
                new P4::ClearTypeMap(typeMap),
                new P4::ResolveReferences(refMap),
                new BFN::TypeInference(typeMap, false), /* extended P4::TypeInference */
                new P4::ApplyTypesToExpressions(typeMap),
                new P4::ResolveReferences(refMap) })
        });
        setName("InitializeMirrorIOSelect");
    }
};

}  // namespace BFN

#endif  // INITIALIZE_MIRROR_IO_SELECT_H_
