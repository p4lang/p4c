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

/**
 * Impact for Large Packets Dropped at Ingress Deparser
 * Description: If the ingress deparser drops a packet it is possible that no
 * learning digest will be generated, and that ingress MAU byte counters and
 * meters are credited with a byte count less than the actual size of the
 * packet.  This may happen if the entire packet is not received before the
 * ingress deparser processes it.  The exact packet sizes at which this may
 * occur depend on the latency of the ingress pipeline but are on the order of
 * larger than about 7600 bytes for a 400g interface or about 2200 bytes for a
 * 100g interface. Workaround: Do not drop packets at the ingress deparser,
 * instead mirror them to a disabled mirroring session to drop them by the
 * mirror block. This fix applies to Tofino 1 and 2.
 */
#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_DROP_PACKET_WITH_MIRROR_ENGINE_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_DROP_PACKET_WITH_MIRROR_ENGINE_H_

#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/common/pragma/pragma.h"
#include "bf-p4c/device.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "type_checker.h"

namespace BFN {

/*
    This pass inserts the following code template into ingress deparser

    Mirror() reserved_mirror;

    apply {
        ...
        if (ig_dprsr_md.mirror_type == RESERVED_MIRROR_TYPE) {
            reserved_mirror.emit(meta.reserved_mirror_session);
    }
    ...
    }


*/
class DropPacketWithMirrorEngine_ : public Transform {
    cstring igIntrMdForDprsrName;
    std::set<cstring> unique_names;

 public:
    DropPacketWithMirrorEngine_() {}
    const IR::Node *preorder(IR::P4Program *p) override {
        // This pass only applies to TNA architecture
        if (BackendOptions().arch == "v1model" || BackendOptions().arch == "psa") prune();
        // collect and set global_pragmas
        CollectGlobalPragma collect_pragma;
        p->apply(collect_pragma);
        // Workaround can be disabled by pragma
        if (collect_pragma.exists(PragmaDisableI2EReservedDropImplementation::name)) prune();
        return p;
    }
    // const IR::Node* preorder(IR::BFN::TnaControl*) override;
    const IR::Node *preorder(IR::Declaration_Instance *) override;
    const IR::Node *postorder(IR::BFN::TnaDeparser *) override;
    const IR::Node *preorder(IR::BFN::TnaParser *parser) override {
        prune();
        return parser;
    }

    profile_t init_apply(const IR::Node *root) override { return Transform::init_apply(root); }
};

class DropPacketWithMirrorEngine : public PassManager {
 public:
    DropPacketWithMirrorEngine(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        addPasses({new PassIf(
            []() {
                // TOFINO1, 2 has the same hardware bug that need this workaround.
                return Device::currentDevice() == Device::TOFINO ||
                       Device::currentDevice() == Device::JBAY;
            },
            {new DropPacketWithMirrorEngine_(), new P4::ClearTypeMap(typeMap),
             new P4::ResolveReferences(refMap),
             new BFN::TypeInference(typeMap, false), /* extended P4::TypeInference */
             new P4::ApplyTypesToExpressions(typeMap), new P4::ResolveReferences(refMap)})});
        setName("DropPacketWithMirrorEngine");
    }
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_DROP_PACKET_WITH_MIRROR_ENGINE_H_ */
