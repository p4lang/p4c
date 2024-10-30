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

#ifndef BF_P4C_MAU_PAYLOAD_GATEWAY_H_
#define BF_P4C_MAU_PAYLOAD_GATEWAY_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv.h"
#include "ir/ir.h"

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
            if (container != cb.container) return container < cb.container;
            return bit_start < cb.bit_start;
        }

        bool operator==(const ContainerByte &cb) const {
            return !((*this) < cb) && !(cb < (*this));
        }

        bool operator!=(const ContainerByte &cb) const { return !((*this) == cb); }

        ContainerByte(PHV::Container c, int bs) : container(c), bit_start(bs) {}
    };

    const PhvInfo &phv;
    ordered_set<cstring> candidates;

    static PayloadArguments convert_entry_to_payload_args(const IR::MAU::Table *tbl,
                                                          const IR::Entry *entry,
                                                          cstring *act_name);
    static bitvec determine_instr_address_payload(const IR::MAU::Action *act,
                                                  const TableResourceAlloc *alloc);
    static bitvec determine_immediate_payload(const IR::MAU::Action *act, PayloadArguments &entry,
                                              const TableResourceAlloc *alloc);
    static bitvec determine_indirect_addr_payload(const IR::MAU::Action *act,
                                                  PayloadArguments &payload_args,
                                                  const IR::MAU::AttachedMemory *at);
    static bitvec determine_indirect_pfe_payload(const IR::MAU::Action *act,
                                                 const IR::MAU::AttachedMemory *at);
    static bitvec determine_meter_type_payload(const IR::MAU::Action *act,
                                               const IR::MAU::AttachedMemory *at);

    static bitvec determine_match_group_payload(const IR::MAU::Table *tbl,
                                                const TableResourceAlloc *alloc,
                                                const IR::MAU::Action *act,
                                                std::vector<const IR::Constant *> arguments,
                                                int entry_idx);

 public:
    explicit FindPayloadCandidates(const PhvInfo &p) : phv(p) {}
    void add_option(const IR::MAU::Table *, LayoutChoices &lc);
    void clear() { candidates.clear(); }
    IR::MAU::Table *convert_to_gateway(const IR::MAU::Table *);
    static bitvec determine_payload(const IR::MAU::Table *tbl, const TableResourceAlloc *alloc,
                                    const IR::MAU::Table::Layout *layout);
};

#endif /* BF_P4C_MAU_PAYLOAD_GATEWAY_H_ */
