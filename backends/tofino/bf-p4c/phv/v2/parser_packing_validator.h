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

#ifndef BF_P4C_PHV_V2_PARSER_PACKING_VALIDATOR_H_
#define BF_P4C_PHV_V2_PARSER_PACKING_VALIDATOR_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/parser_packing_validator_interface.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "bf-p4c/phv/v2/utils_v2.h"
#include "ir/ir.h"

namespace PHV {
namespace v2 {

/// what we have done wrong:
/// only check against candidate and allocated: this is because we assume that slice lists
/// make sure that the packing is okay because they are very likely coming out from a header.
/// But, it is not true for
/// (1) bridged metadata, they are metadata and we packed them.
/// (2) fancy usage of header fields.
/// (3) result came from co-packer or table key optimization packing.
/// (4) pa_byte_pack.
class ParserPackingValidator : public ParserPackingValidatorInterface {
 private:
    using StateExtract = std::pair<const IR::BFN::ParserState *, const IR::BFN::Extract *>;
    using StatePrimitiveMap =
        ordered_map<const IR::BFN::ParserState *, std::vector<const IR::BFN::ParserPrimitive *>>;
    const PhvInfo &phv_i;
    const MapFieldToParserStates &parser_i;
    const CollectParserInfo &parser_info_i;
    const FieldDefUse &defuse_i;
    const PragmaNoInit &pa_no_init_i;
    mutable bool is_trivial_pass = false;
    mutable bool is_trivial_alloc = false;
    std::set<FieldRange> &mauInitFields;

    // The cache does not affect behaviour, only speed. Therefore we make it mutable to make it
    // possible to use it from a (semantically) const method.
    mutable assoc::hash_map<std::pair<const IR::BFN::ParserState *, const IR::Expression *>,
                            std::vector<const IR::BFN::ParserPrimitive *>>
        state_extracts_cache;

    /// @returns all primitives to @p fs, grouped by states.
    StatePrimitiveMap get_primitives(const FieldSlice &fs) const;

    /// @returns true if the field needs to be the default value when it left parser.
    /// The default value is zero and the container validity bit (in Tofino) is zero.
    bool parser_zero_init(const Field *f) const {
        return (defuse_i.hasUninitializedRead(f->id) && !pa_no_init_i.getFields().count(f));
    }

    /// @returns true if it is okay to write random bits to @p in parser.
    bool allow_clobber(const Field *f) const;

    /// @returns true if @p f is parser error
    bool is_parser_error(const Field *f) const;

    /// @returns an error if @p state_extract will clobber value of other_fs in parser.
    const AllocError *will_buf_extract_clobber_the_other(
        const FieldSlice &fs, const StateExtract &state_extract, const int cont_idx,
        const FieldSlice &other_fs, const StatePrimitiveMap &other_extracts,
        const int other_cont_idx, bool add_mau_inits) const;

    /// @returns an error if there is an extract from a that will clobber b's bits.
    const AllocError *will_a_extracts_clobber_b(const FieldSliceStart &a, const FieldSliceStart &b,
                                                bool add_mau_inits) const;

 public:
    explicit ParserPackingValidator(const PhvInfo &phv, const MapFieldToParserStates &parser,
                                    const CollectParserInfo &parser_info, const FieldDefUse &defuse,
                                    const PragmaNoInit &pa_no_init,
                                    std::set<FieldRange> &mauInitFields)
        : phv_i(phv),
          parser_i(parser),
          parser_info_i(parser_info),
          defuse_i(defuse),
          pa_no_init_i(pa_no_init),
          mauInitFields(mauInitFields) {}

    /// @returns an error if we cannot allocated @p a and @p b in a container.
    /// @p c is optional for 32-bit container half-word extract optimization.
    const AllocError *can_pack(const FieldSliceStart &a, const FieldSliceStart &b,
                               bool add_mau_inits = false) const;

    /// @returns an error if we allocated slices in the format of @p alloc.
    /// @p c is optional for 32-bit container half-word extract optimization.
    const AllocError *can_pack(const FieldSliceAllocStartMap &alloc,
                               bool add_mau_inits = false) const override;

    // Marks current iteration as trivial allocation iteration
    void set_trivial_pass(bool trivial) const;
    // Marks the allocation part of a trivial allocation
    void set_trivial_alloc(bool trivial) const;
};
}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_PARSER_PACKING_VALIDATOR_H_ */
