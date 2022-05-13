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
#ifndef BACKENDS_EBPF_PSA_XDPHELPPROGRAM_H_
#define BACKENDS_EBPF_PSA_XDPHELPPROGRAM_H_

#include "backends/ebpf/ebpfProgram.h"

namespace EBPF {

class XDPHelpProgram : public EBPFProgram {
    cstring XDPProgUsingMetaForXDP2TC =
            "    void *data_end = (void *)(long)skb->data_end;\n"
            "    struct ethhdr *eth = (struct ethhdr *)(long)skb->data;\n"
            "    if ((void *)((struct ethhdr *) eth + 1) > data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    if (eth->h_proto == bpf_htons(0x0800) || eth->h_proto == bpf_htons(0x86DD)) {\n"
            "        return XDP_PASS;\n"
            "    }\n"
            "\n"
            "    struct internal_metadata *meta;\n"
            "    int ret = bpf_xdp_adjust_meta(skb, -(int)sizeof(*meta));\n"
            "    if (ret < 0) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    meta = (struct internal_metadata *)(unsigned long)skb->data_meta;\n"
            "    eth = (void *)(long)skb->data;\n"
            "    data_end = (void *)(long)skb->data_end;\n"
            "    if ((void *) ((struct internal_metadata *) meta + 1) > (void *)(long)skb->data)\n"
            "        return XDP_ABORTED;\n"
            "    if ((void *)((struct ethhdr *) eth + 1) > data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    meta->pkt_ether_type = eth->h_proto;\n"
            "    eth->h_proto = bpf_htons(0x0800);\n"
            "\n"
            "    return XDP_PASS;";

    cstring XDPProgUsingHeadForXDP2TC =
            "    void *data = (void *)(long)skb->data;\n"
            "    void *data_end = (void *)(long)skb->data_end;\n"
            "    struct ethhdr *eth = data;\n"
            "    if ((void *)((struct ethhdr *) eth + 1) > data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    __u16 orig_ethtype = eth->h_proto;\n"
            "    int ret = bpf_xdp_adjust_head(skb, -2);\n"
            "    if (ret < 0) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "\n"
            "    data = (void *)(long)skb->data;\n"
            "    data_end = (void *)(long)skb->data_end;\n"
            "\n"
            "    if ((void *)(data + 16) > data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    __builtin_memmove(data, data + 2, 14);\n"
            "    eth = data;\n"
            "    if ((void *)((struct ethhdr *) eth + 1) > data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    eth->h_proto = bpf_htons(0x0800);\n"
            "    __builtin_memcpy((char *)data + 14, &orig_ethtype, 2);\n"
            "    "
            "\n"
            "    return XDP_PASS;";

    cstring XDPProgUsingCPUMAPForXDP2TC =
            "    void *data = (void *)(long)skb->data;\n"
            "    void *data_end = (void *)(long)skb->data_end;\n"
            "    struct ethhdr *eth = data;\n"
            "    if ((void *)((struct ethhdr *) eth + 1) > data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n"
            "    u16 orig_ethtype = eth->h_proto;\n"
            "    eth->h_proto = bpf_htons(0x0800);\n"
            "    u32 zero = 0;\n"
            "    BPF_MAP_UPDATE_ELEM(xdp2tc_cpumap, &zero, &orig_ethtype, BPF_ANY);\n"
            "    return XDP_PASS;";

 public:
    cstring sectionName;
    explicit XDPHelpProgram(const EbpfOptions& options) :
        EBPFProgram(options, nullptr, nullptr, nullptr, nullptr) {
        sectionName = "xdp/xdp-ingress";
        functionName = "xdp_func";
    }

    void emit(CodeBuilder *builder) {
        builder->target->emitCodeSection(builder, sectionName);
        builder->emitIndent();
        builder->appendFormat("int %s(struct xdp_md *%s)",
                              functionName, model.CPacketName.str());
        builder->spc();
        builder->blockStart();
        builder->emitIndent();
        // this is static program, so we can just paste a piece of code.
        if (options.xdp2tcMode == XDP2TC_META) {
            builder->appendLine(XDPProgUsingMetaForXDP2TC);
        } else if (options.xdp2tcMode == XDP2TC_HEAD) {
            builder->appendLine(XDPProgUsingHeadForXDP2TC);
        } else if (options.xdp2tcMode == XDP2TC_CPUMAP) {
            builder->appendLine(XDPProgUsingCPUMAPForXDP2TC);
        }
        builder->blockEnd(true);
    }
};

}  // namespace EBPF

#endif  /* BACKENDS_EBPF_PSA_XDPHELPPROGRAM_H_ */
