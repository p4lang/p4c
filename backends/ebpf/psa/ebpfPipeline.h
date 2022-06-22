/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef BACKENDS_EBPF_PSA_EBPFPIPELINE_H_
#define BACKENDS_EBPF_PSA_EBPFPIPELINE_H_

#include "ebpfPsaControl.h"
#include "backends/ebpf/ebpfProgram.h"
#include "ebpfPsaDeparser.h"
#include "backends/ebpf/target.h"

namespace EBPF {

/*
 * EBPFPipeline represents a single eBPF program in the TC/XDP hook.
 */
class EBPFPipeline : public EBPFProgram {
 public:
    // a custom name of eBPF program
    cstring name;
    // eBPF section name, which should a concatenation of `classifier/` + a custom name.
    cstring sectionName;
    // Variable name storing pointer to eBPF packet descriptor (e.g., __sk_buff).
    cstring contextVar;
    // Variable name storing current timestamp retrieved from bpf_ktime_get_ns().
    cstring timestampVar;
    // Variable storing ingress interface index.
    cstring ifindexVar;
    // Variable storing skb->priority value (TC only).
    cstring priorityVar;
    // Variables storing global metadata (packet_path & instance).
    cstring packetPathVar, pktInstanceVar;
    // A name of an internal variable storing global metadata.
    cstring compilerGlobalMetadata;
    // A variable name storing "1" value. Used to access BPF array map index.
    cstring oneKey;

    EBPFControlPSA* control;
    EBPFDeparserPSA* deparser;

    EBPFPipeline(cstring name, const EbpfOptions& options, P4::ReferenceMap* refMap,
                 P4::TypeMap* typeMap)
                 : EBPFProgram(options, nullptr, refMap, typeMap, nullptr),
                 name(name), control(nullptr), deparser(nullptr) {
        sectionName = "classifier/" + name;
        functionName = name.replace("-", "_") + "_func";
        errorEnum = "ParserError_t";
        packetStartVar = cstring("pkt");
        contextVar = cstring("skb");
        lengthVar = cstring("pkt_len");
        endLabel = cstring("deparser");
        timestampVar = cstring("tstamp");
        ifindexVar = cstring("skb->ifindex");
        compilerGlobalMetadata = cstring("compiler_meta__");
        packetPathVar = compilerGlobalMetadata + cstring("->packet_path");
        pktInstanceVar = compilerGlobalMetadata + cstring("->instance");
        priorityVar = cstring("skb->priority");
        oneKey = EBPFModel::reserved("one");
    }

    /* Check if pipeline does any processing.
     * Return false if not. */
    bool isEmpty() const;

    virtual cstring dropReturnCode() {
        if (sectionName.startsWith("xdp")) {
            return "XDP_DROP";
        }

        // TC is the default hookpoint
        return "TC_ACT_SHOT";
    }
    virtual cstring forwardReturnCode() {
        if (sectionName.startsWith("xdp")) {
            return "XDP_PASS";
        }

        // TC is the default hookpoint
        return "TC_ACT_OK";
    }

    virtual void emit(CodeBuilder* builder) = 0;
    virtual void emitTrafficManager(CodeBuilder *builder) = 0;
    virtual void emitPSAControlInputMetadata(CodeBuilder* builder) = 0;
    virtual void emitPSAControlOutputMetadata(CodeBuilder* builder) = 0;

    /* Generates a pointer to struct Headers_t and puts it on the BPF program's stack. */
    void emitLocalHeaderInstancesAsPointers(CodeBuilder *builder);
    /* Generates a pointer to struct hdr_md. The pointer is used to access data from per-CPU map. */
    void emitCPUMAPHeadersInitializers(CodeBuilder *builder);
    /* Generates an instance of struct Headers_t,
     * allocated in the per-CPU map. */
    void emitHeaderInstances(CodeBuilder *builder) override;
    /* Generates a set of helper variables that are used during packet processing. */
    void emitLocalVariables(CodeBuilder* builder) override;

    /* Generates and instance of user metadata for a pipeline,
     * allocated in the per-CPU map. */
    void emitUserMetadataInstance(CodeBuilder *builder);

    virtual void emitCPUMAPInitializers(CodeBuilder *builder);
    virtual void emitCPUMAPLookup(CodeBuilder *builder);
    /* Generates a pointer to skb->cb and maps it to
     * psa_global_metadata to access global metadata shared between pipelines. */
    virtual void emitGlobalMetadataInitializer(CodeBuilder *builder);
    virtual void emitPacketLength(CodeBuilder *builder);
    virtual void emitTimestamp(CodeBuilder *builder);

    void emitHeadersFromCPUMAP(CodeBuilder* builder);
    void emitMetadataFromCPUMAP(CodeBuilder *builder);

    bool hasAnyMeter() const {
        auto directMeter = std::find_if(control->tables.begin(),
                                        control->tables.end(),
                                        [](std::pair<const cstring, EBPFTable*> elem) {
                                            return !elem.second->to<EBPFTablePSA>()->meters.empty();
                                        });
        bool anyDirectMeter = directMeter != control->tables.end();
        return anyDirectMeter || (!control->meters.empty());
    }
    /*
     * Returns whether the compiler should generate
     * timestamp retrieved by bpf_ktime_get_ns().
     *
     * This allows to avoid overhead introduced by bpf_ktime_get_ns(),
     * if the timestamp field is not used within a pipeline.
     */
    bool shouldEmitTimestamp() const {
        return hasAnyMeter() || control->timestampIsUsed;
    }
};

/*
 * EBPFIngressPipeline represents a hook-independent EBPF-based ingress pipeline.
 * It includes common definitions for TC and XDP.
 */
class EBPFIngressPipeline : public EBPFPipeline {
 public:
    unsigned int maxResubmitDepth;
    // actUnspecCode stores the "undefined action" value.
    // It's returned from eBPF program is PSA-eBPF doesn't make any forwarding/drop decision.
    int actUnspecCode;

    EBPFIngressPipeline(cstring name, const EbpfOptions& options, P4::ReferenceMap* refMap,
                        P4::TypeMap* typeMap) : EBPFPipeline(name, options, refMap, typeMap) {
        // FIXME: hardcoded
        maxResubmitDepth = 4;
        // actUnspecCode should not collide with TC/XDP return codes,
        // but it's safe to use the same value as TC_ACT_UNSPEC.
        actUnspecCode = -1;
    }

    void emitSharedMetadataInitializer(CodeBuilder* builder);

    void emit(CodeBuilder *builder) override;
    void emitPSAControlInputMetadata(CodeBuilder* builder) override;
    void emitPSAControlOutputMetadata(CodeBuilder* builder) override;
};

/*
 * EBPFEgressPipeline represents a hook-independent EBPF-based egress pipeline.
 * It includes common definitions for TC and XDP.
 */
class EBPFEgressPipeline : public EBPFPipeline {
 public:
    EBPFEgressPipeline(cstring name, const EbpfOptions& options, P4::ReferenceMap* refMap,
                       P4::TypeMap* typeMap) : EBPFPipeline(name, options, refMap, typeMap) { }

    void emit(CodeBuilder* builder) override;
    void emitPSAControlInputMetadata(CodeBuilder* builder) override;
    void emitPSAControlOutputMetadata(CodeBuilder* builder) override;
    void emitCPUMAPLookup(CodeBuilder *builder) override;
};

class TCIngressPipeline : public EBPFIngressPipeline {
 public:
    TCIngressPipeline(cstring name, const EbpfOptions& options, P4::ReferenceMap* refMap,
                        P4::TypeMap* typeMap) :
            EBPFIngressPipeline(name, options, refMap, typeMap) {}

    void emitGlobalMetadataInitializer(CodeBuilder *builder) override;
    void emitTrafficManager(CodeBuilder *builder) override;
 private:
    void emitTCWorkaroundUsingMeta(CodeBuilder *builder);
    void emitTCWorkaroundUsingHead(CodeBuilder *builder);
    void emitTCWorkaroundUsingCPUMAP(CodeBuilder *builder);
};

class TCEgressPipeline : public EBPFEgressPipeline {
 public:
    TCEgressPipeline(cstring name, const EbpfOptions& options, P4::ReferenceMap* refMap,
                       P4::TypeMap* typeMap) :
            EBPFEgressPipeline(name, options, refMap, typeMap) { }

    void emitTrafficManager(CodeBuilder *builder) override;
};
}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPIPELINE_H_ */
