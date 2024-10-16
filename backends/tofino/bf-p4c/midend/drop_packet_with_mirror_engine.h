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
#ifndef EXTENSIONS_BF_P4C_MIDEND_DROP_PACKET_WITH_MIRROR_ENGINE_H_
#define EXTENSIONS_BF_P4C_MIDEND_DROP_PACKET_WITH_MIRROR_ENGINE_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "type_checker.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/common/pragma/pragma.h"
#include "bf-p4c/device.h"

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
    const IR::Node* preorder(IR::P4Program* p) override {
        // This pass only applies to TNA architecture
        if (BackendOptions().arch == "v1model" || BackendOptions().arch == "psa")
            prune();
        // collect and set global_pragmas
        CollectGlobalPragma collect_pragma;
        p->apply(collect_pragma);
        // Workaround can be disabled by pragma
        if (collect_pragma.exists(PragmaDisableI2EReservedDropImplementation::name))
            prune();
        return p;
    }
    // const IR::Node* preorder(IR::BFN::TnaControl*) override;
    const IR::Node* preorder(IR::Declaration_Instance*) override;
    const IR::Node* postorder(IR::BFN::TnaDeparser*) override;
    const IR::Node* preorder(IR::BFN::TnaParser* parser) override { prune(); return parser; }

    profile_t init_apply(const IR::Node* root) override {
        return Transform::init_apply(root);
    }
};

class DropPacketWithMirrorEngine : public PassManager {
 public:
    DropPacketWithMirrorEngine(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        addPasses({
            new PassIf([]() {
                // TOFINO1, 2 has the same hardware bug that need this workaround.
                return Device::currentDevice() == Device::TOFINO
                    || Device::currentDevice() == Device::JBAY
                    ;
            }, {
                new DropPacketWithMirrorEngine_(),
                new P4::ClearTypeMap(typeMap),
                new P4::ResolveReferences(refMap),
                new BFN::TypeInference(typeMap, false), /* extended P4::TypeInference */
                new P4::ApplyTypesToExpressions(typeMap),
                new P4::ResolveReferences(refMap) })
        });
        setName("DropPacketWithMirrorEngine");
    }
};

}  // namespace BFN

#endif  /* EXTENSIONS_BF_P4C_MIDEND_DROP_PACKET_WITH_MIRROR_ENGINE_H_ */
