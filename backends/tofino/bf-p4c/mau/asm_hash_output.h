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

#ifndef BF_P4C_MAU_ASM_HASH_OUTPUT_H_
#define BF_P4C_MAU_ASM_HASH_OUTPUT_H_

#include <iterator>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "action_data_bus.h"
#include "bf-p4c/common/alias.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/mau/asm_format_hash.h"
#include "bf-p4c/mau/asm_output.h"
#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_format.h"
#include "bf-p4c/parde/asm_output.h"
#include "bf-p4c/parde/phase0.h"
#include "bf-p4c/phv/asm_output.h"
#include "boost/range/adaptor/reversed.hpp"
#include "lib/algorithm.h"
#include "lib/bitops.h"
#include "lib/bitrange.h"
#include "lib/hex.h"
#include "lib/indent.h"
#include "lib/stringref.h"

using namespace P4;

void emit_ixbar_gather_map(const PhvInfo &phv, std::multimap<int, Slice> &match_data_map,
                           std::map<le_bitrange, const IR::Constant *> &constant_map,
                           const safe_vector<Slice> &match_data,
                           const safe_vector<const IR::Expression *> &field_list_order,
                           const LTBitMatrix &sym_keys, int &total_size);

class EmitHashExpression : public Inspector {
    const PhvInfo &phv;
    std::ostream &out;
    indent_t indent;
    int bit;
    const safe_vector<Slice> &match_data;

    bool preorder(const IR::Concat *c) override;
    bool preorder(const IR::BFN::SignExtend *c) override;
    bool preorder(const IR::Constant *) override;
    bool preorder(const IR::Expression *e) override;
    bool preorder(const IR::MAU::HashGenExpression *hge) override;

 public:
    EmitHashExpression(const PhvInfo &phv, std::ostream &out, indent_t indent, int bit,
                       const safe_vector<Slice> &match_data)
        : phv(phv), out(out), indent(indent), bit(bit), match_data(match_data) {}
};

void emit_ixbar_match_func(std::ostream &out, indent_t indent, safe_vector<Slice> &match_data,
                           Slice *ghost, le_bitrange hash_bits);
void emit_ixbar_hash_atcam(std::ostream &out, indent_t indent, safe_vector<Slice> &ghost,
                           const ::IXBar::Use *use, int hash_group);
void ixbar_hash_exact_bitrange(Slice ghost_slice, int min_way_size, le_bitrange non_rotated_slice,
                               le_bitrange comp_slice, int initial_lo_bit,
                               safe_vector<std::pair<le_bitrange, Slice>> &ghost_positions);
void ixbar_hash_exact_info(int &min_way_size, int &min_way_slice, const ::IXBar::Use *use,
                           int hash_group, std::map<int, bitvec> &slice_to_select_bits);
void emit_ixbar_hash_exact(std::ostream &out, indent_t indent, safe_vector<Slice> &match_data,
                           safe_vector<Slice> &ghost, const ::IXBar::Use *use, int hash_group,
                           int &ident_bits_prev_alloc);
void emit_ixbar_gather_map(const PhvInfo &phv, std::multimap<int, Slice> &match_data_map,
                           std::map<le_bitrange, const IR::Constant *> &constant_map,
                           const safe_vector<Slice> &match_data,
                           const safe_vector<const IR::Expression *> &field_list_order,
                           const LTBitMatrix &sym_keys, int &total_size);
void emit_ixbar_gather_bytes(const PhvInfo &phv, const safe_vector<::IXBar::Use::Byte> &use,
                             std::map<int, std::map<int, Slice>> &sort,
                             std::map<int, std::map<int, Slice>> &midbytes,
                             const IR::MAU::Table *tbl, bool ternary, bool atcam = false);
void emit_ixbar_hash_table(int hash_table, safe_vector<Slice> &match_data,
                           safe_vector<Slice> &ghost, const TableMatch *fmt,
                           std::map<int, std::map<int, Slice>> &sort);

#endif /* BF_P4C_MAU_ASM_HASH_OUTPUT_H_ */
