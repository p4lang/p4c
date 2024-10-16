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

#ifndef BF_P4C_MAU_PAYLOAD_GATEWAY_H_
#define BF_P4C_MAU_PAYLOAD_GATEWAY_H_

#include "ir/ir.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv.h"

using namespace P4;

class PhvInfo;
class LayoutChoices;

class FindPayloadCandidates {
 public:
    static constexpr int GATEWAY_ROWS_FOR_ENTRIES = 4;
    using PayloadArguments = std::vector<const IR::Constant *>;

 private:
    struct ContainerByte {
        PHV::Container container;
        int bit_start;

        bool operator<(const ContainerByte &cb) const {
            if (container != cb.container)
                return container < cb.container;
            return bit_start < cb.bit_start;
        }

        bool operator==(const ContainerByte &cb) const {
            return !((*this) < cb) &&  !(cb < (*this));
        }

        bool operator!=(const ContainerByte &cb) const {
            return !((*this) == cb);
        }

        ContainerByte(PHV::Container c, int bs) : container(c), bit_start(bs) {}
    };

    const PhvInfo &phv;
    ordered_set<cstring> candidates;


    static PayloadArguments convert_entry_to_payload_args(const IR::MAU::Table *tbl,
        const IR::Entry *entry, cstring *act_name);
    static bitvec determine_instr_address_payload(const IR::MAU::Action *act,
        const TableResourceAlloc *alloc);
    static bitvec determine_immediate_payload(const IR::MAU::Action *act,
        PayloadArguments &entry, const TableResourceAlloc *alloc);
    static bitvec determine_indirect_addr_payload(const IR::MAU::Action *act,
        PayloadArguments &payload_args, const IR::MAU::AttachedMemory *at);
    static bitvec determine_indirect_pfe_payload(const IR::MAU::Action *act,
        const IR::MAU::AttachedMemory *at);
    static bitvec determine_meter_type_payload(const IR::MAU::Action *act,
        const IR::MAU::AttachedMemory *at);

    static bitvec determine_match_group_payload(const IR::MAU::Table *tbl,
        const TableResourceAlloc *alloc, const IR::MAU::Action *act,
        std::vector<const IR::Constant *> arguments, int entry_idx);

 public:
    explicit FindPayloadCandidates(const PhvInfo &p) : phv(p) {}
    void add_option(const IR::MAU::Table *, LayoutChoices &lc);
    void clear() { candidates.clear(); }
    IR::MAU::Table *convert_to_gateway(const IR::MAU::Table *);
    static bitvec determine_payload(const IR::MAU::Table *tbl, const TableResourceAlloc *alloc,
        const IR::MAU::Table::Layout *layout);
};

#endif  /* BF_P4C_MAU_PAYLOAD_GATEWAY_H_ */
