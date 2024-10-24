/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INITIALIZE_MIRROR_IO_SELECT_H_
#define INITIALIZE_MIRROR_IO_SELECT_H_

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/device.h"
#include "ir/ir.h"
#include "type_checker.h"

namespace BFN {

// Initialize eg_intr_md_for_dprsr.mirror_io_select
class DoInitializeMirrorIOSelect : public Transform {
    cstring egIntrMdForDprsrName;

 public:
    DoInitializeMirrorIOSelect(){};

    // disable this pass if the @disable_egress_mirror_io_select_initialization pragma is used
    const IR::Node *preorder(IR::P4Program *p) override {
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
    const IR::Node *preorder(IR::BFN::TnaParser *parser) override;
    const IR::Node *preorder(IR::ParserState *state) override;

    // Skip controls and deparsers
    const IR::Node *preorder(IR::BFN::TnaControl *control) override {
        prune();
        return control;
    };
    const IR::Node *preorder(IR::BFN::TnaDeparser *deparser) override {
        prune();
        return deparser;
    };
};

/**
 * @ingroup midend
 * @ingroup parde
 * @brief Initializes eg_intr_md_for_dprsr.mirror_io_select on devices except Tofino1
 */
class InitializeMirrorIOSelect : public PassManager {
 public:
    InitializeMirrorIOSelect(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        addPasses({new PassIf(
            []() { return Device::hasMirrorIOSelect(); },
            {new DoInitializeMirrorIOSelect(), new P4::ClearTypeMap(typeMap),
             new P4::ResolveReferences(refMap),
             new BFN::TypeInference(typeMap, false), /* extended P4::TypeInference */
             new P4::ApplyTypesToExpressions(typeMap), new P4::ResolveReferences(refMap)})});
        setName("InitializeMirrorIOSelect");
    }
};

}  // namespace BFN

#endif  // INITIALIZE_MIRROR_IO_SELECT_H_
