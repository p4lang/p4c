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

#include "bf-p4c/phv/v2/parser_packing_validator.h"

#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "lib/algorithm.h"

namespace PHV {
namespace v2 {

namespace {

/// @returns network-order bitrange of extract to @p fs;
nw_bitrange slice_extract_range(const FieldSlice &fs, const IR::BFN::Extract *e) {
    BUG_CHECK(e->source->is<IR::BFN::PacketRVal>(), "not a packet extract: %1%", e);
    const auto &buf_range = e->source->to<IR::BFN::PacketRVal>()->range;
    return nw_bitrange(
        StartLen(buf_range.hi - fs.range().lo - fs.range().size() + 1, fs.range().size()));
}

le_bitrange expand_to_byte_range(int cont_idx, int size) {
    int start_byte = cont_idx / 8;
    int end_byte = ROUNDUP(cont_idx + size, 8);
    return le_bitrange(FromTo(start_byte * 8, end_byte * 8 - 1));
}

}  // namespace

ParserPackingValidator::StatePrimitiveMap ParserPackingValidator::get_primitives(
    const FieldSlice &fs) const {
    StatePrimitiveMap rst;
    for (const auto &[loc, expr] : defuse_i.getParserDefs(fs)) {
        auto state = loc->to<IR::BFN::ParserState>();
        if (!state) continue;  // cannot be write if not in state

        auto [it, inserted] = state_extracts_cache.try_emplace({state, expr});
        if (inserted) {  // not cached, need to calculate
            // defuse does not track extracts directly so we need to find it in the state
            for (auto *stmt : state->statements) {
                if (auto *extract = stmt->to<IR::BFN::Extract>()) {
                    state_extracts_cache[{state, extract->dest->field}].push_back(extract);
                } else if (auto *checksum = stmt->to<IR::BFN::ParserChecksumWritePrimitive>()) {
                    if (auto *dest = checksum->getWriteDest())
                        state_extracts_cache[{state, dest->field}].push_back(checksum);
                }
            }
        }

        for (auto extract : it->second) {
            rst[state].push_back(extract);
        }
    }
    return rst;
}

bool ParserPackingValidator::allow_clobber(const Field *f) const {
    if (f->is_ignore_alloc() || f->is_padding()) return true;
    const bool parser_def = parser_i.field_to_writes.count(f) || f->pov || f->isGhostField();
    // TODO: parser_error?
    return !parser_def && !parser_zero_init(f) && !f->is_invalidate_from_arch();
}

// TODO: strided header fields are handled in the same way as normal fields,
// but in the old allocator, strided header fields are skipped in these checks.
// TODO: enable strict mode.
// TODO: allow a and b to be packed together when
// a is extracted by clear-on-write and b is okay to be 0 after the state.
const AllocError *ParserPackingValidator::will_buf_extract_clobber_the_other(
    const FieldSlice &fs, const StateExtract &state_extract, const int cont_idx,
    const FieldSlice &other_fs, const StatePrimitiveMap &other_prims, const int other_cont_idx,
    bool add_mau_inits) const {
    auto *err = new AllocError(ErrorCode::CONTAINER_PARSER_PACKING_INVALID);
    const auto [state, extract] = state_extract;
    const auto *parser = parser_i.state_to_parser.at(state);
    const auto buf_range = slice_extract_range(fs, extract);
    const bool is_clear_on_write =
        (extract->write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE);
    bool support_byte_extract = Device::pardeSpec().parserAllExtractorsSupportSingleByte();
    const bool is_byte_extract = !is_clear_on_write && support_byte_extract;
    bool write_overlap = true;
    if (is_byte_extract) {
        le_bitrange write_range = expand_to_byte_range(cont_idx, fs.size());
        le_bitrange other_write = expand_to_byte_range(other_cont_idx, other_fs.size());
        write_overlap = write_range.overlaps(other_write);
    }
    if (!write_overlap) {
        LOG5("destination bytes do not overlap, skip parser checks: "
             << fs << " @" << cont_idx << " v.s. " << other_fs << " @" << other_cont_idx);
        return nullptr;
    }

    LOG5("check if " << extract << " in " << state->name
                     << " will clobber the other: " << other_fs);

    // For extracts in the same state, other input buffer offsets need to be the same as their
    // PHV container offset: must be extracted together in the same state.
    if (auto prims_it = other_prims.find(state); prims_it != other_prims.end()) {
        for (auto *other_prim : prims_it->second) {
            bool other_prim_has_packet_rval = false;
            if (auto *other_extract = other_prim->to<IR::BFN::Extract>()) {
                if (other_extract->source->is<IR::BFN::PacketRVal>()) {
                    other_prim_has_packet_rval = true;
                    const auto other_buf_range = slice_extract_range(other_fs, other_extract);
                    if (!(other_buf_range.hi - buf_range.hi == cont_idx - other_cont_idx)) {
                        *err << "cannot pack " << fs << " with " << other_fs << " because in state "
                             << state->name << " the former " << "slice is extracted from " << ": "
                             << buf_range << ", but " << "the latter is " << other_buf_range
                             << ", which is not consistent with " << "their allocation: former@"
                             << cont_idx << ", other@" << other_cont_idx;
                        return err;
                    }
                }
            }
            if (!other_prim_has_packet_rval) {
                // TODO: This if check is not required! Remove to always return an error
                // when we transition to strict mode.
                if (!phv_i.must_alloc_same_container(fs, other_fs)) {
                    *err << "cannot pack " << fs << " with " << other_fs << " because in state "
                         << state->name << " the former " << "slice is extracted from " << ": "
                         << buf_range << ", but " << "the latter has " << other_prim;
                    return err;
                }
            }
        }
    }

    // TODO: THIS is not required! BUT there are some P4 tests in our CI
    // that are incorrect: the manual extraction they wrote will corrupt other
    // fields!
    // TODO(yumin): we should disallow these behaviors^, and remove this check,
    // unless there is some fancy way to bypass it?
    if (phv_i.must_alloc_same_container(fs, other_fs)) {
        LOG5("Packing is unavoidable.");
        return (AllocError *)nullptr;
    }

    // if the other field is not extracted in this state, then
    //    a. the other field cannot be parser zero initialized, unless
    //       this extract is clear-on-write extract.
    //    b. all other extracts are mutex.
    if (!other_prims.count(state) && (!is_clear_on_write && parser_zero_init(other_fs.field()))) {
        // Make an exception for non extracted fields packed with extracted
        // fields in same slicelist for trivial allocation. Track these
        // fields in mauInitFields for initialization in MAU
        if (is_trivial_alloc && !defuse_i.hasDefInParser(other_fs)) {
            mauInitFields.insert(
                {other_fs.field()->name, other_fs.range(), {{fs.field()->name, fs.range(), {}}}});
            LOG5(Log::indent << "Adding mau-init-field " << other_fs.field()->name << " "
                             << other_fs.range() << " add_mau_inits:" << add_mau_inits);
            LOG5(Log::indent << "... due to: " << fs.field()->name << " " << fs.range()
                             << Log::unindent << Log::unindent);
        } else {
            bool flag_error = true;
            // Do not flag error for non-trivial allocation when
            // initialization has been selected by trivial allocation
            if (!is_trivial_pass && !add_mau_inits) {
                auto mau_init = mauInitFields.find({other_fs.field()->name, other_fs.range(), {}});
                if (mau_init != mauInitFields.end()) {
                    if (mau_init->has_conflict({fs.field()->name, fs.range(), {}})) {
                        LOG5(other_fs << " has been marked as mau-init-field wrt " << fs);
                        flag_error = false;
                    }
                }
                if (flag_error) {
                    *err << "cannot pack " << fs << " with " << other_fs << " because the former "
                         << "slice is extracted in " << state->name << ": " << extract << ", but "
                         << "the latter field requires parser zero initialization.";
                    return err;
                }
            }
        }
    }
    for (const auto &other_state_prim : other_prims) {
        const auto *other_state = other_state_prim.first;
        const auto &other_prims = other_state_prim.second;
        if (other_state == state) continue;
        const auto *other_parser = parser_i.state_to_parser.at(other_state);
        if (other_parser != parser) {
            continue;
        }
        LOG5("Check extract: " << other_state->name << ": " << other_prims);
        if (write_overlap && !parser_info_i.graph(parser).is_mutex(state, other_state)) {
            *err << "cannot pack " << fs << " with " << other_fs << " because the former "
                 << "slice is extracted in " << state->name << ": " << extract << ", but "
                 << "the latter is extracted in this non-mutex state: " << other_state->name;
            return err;
        }
    }
    return (AllocError *)nullptr;
}

const AllocError *ParserPackingValidator::will_a_extracts_clobber_b(const FieldSliceStart &a,
                                                                    const FieldSliceStart &b,
                                                                    bool add_mau_inits) const {
    const auto &[a_fs, a_idx] = a;
    const auto &[b_fs, b_idx] = b;
    const auto a_extracts = get_primitives(a_fs);
    const auto b_extracts = get_primitives(b_fs);
    // because a is fine-sliced, each parser state will have at most 1 extract to the slice,
    // we do not need to check extract within the a_extracts,
    for (const auto &[state, state_extracts] : a_extracts) {
        // extractions will set container validity bit to 1 (including const), so we cannot
        // pack is_invalidate_from_arch with any extracted field.
        if (b_fs.field()->is_invalidate_from_arch() && a_fs.field() != b_fs.field()) {
            auto *err = new AllocError(ErrorCode::CONTAINER_PARSER_PACKING_INVALID);
            *err << "cannot pack with invalidate from arch field: " << b.first;
            return err;
        }
        if (allow_clobber(b_fs.field())) break;
        for (const auto &prim : state_extracts) {
            if (auto *extract = prim->to<IR::BFN::Extract>()) {
                StateExtract state_extract{state, extract};
                // only extract from input buffer will clobber bits.
                // TODO: what about checksum-residual-deposit? what about
                // clean-on-write from constant / checksums?
                if (!extract->source->is<IR::BFN::PacketRVal>()) {
                    continue;
                }
                auto *err = will_buf_extract_clobber_the_other(a_fs, state_extract, a_idx, b_fs,
                                                               b_extracts, b_idx, add_mau_inits);
                if (err) {
                    return err;
                }
            }
        }
    }
    return nullptr;
}

bool ParserPackingValidator::is_parser_error(const Field *f) const {
    const auto &defs = defuse_i.getAllDefs(f->id);
    for (const auto &def : defs) {
        if (def.second->is<WriteParserError>()) {
            return true;
        }
    }
    return false;
}

const AllocError *ParserPackingValidator::can_pack(const FieldSliceStart &a,
                                                   const FieldSliceStart &b,
                                                   bool add_mau_inits) const {
    const auto *f_a = a.first.field();
    const auto *f_b = b.first.field();
    if (f_a != f_b && (is_parser_error(f_a) || is_parser_error(f_b))) {
        auto *err = new AllocError(ErrorCode::CONTAINER_PARSER_PACKING_INVALID);
        *err << "do not support parser error packing with other fields for now";
        return err;
    }
    if (phv_i.field_mutex()(a.first.field()->id, b.first.field()->id)) {
        LOG5("field_mutex is true: " << a.first << " and " << b.first);
        return nullptr;
    }
    if (auto *err = will_a_extracts_clobber_b(a, b, add_mau_inits)) {
        return err;
    } else if (auto *err = will_a_extracts_clobber_b(b, a, add_mau_inits)) {
        return err;
    } else {
        return nullptr;
    }
}

const AllocError *ParserPackingValidator::can_pack(const FieldSliceAllocStartMap &alloc,
                                                   bool add_mau_inits) const {
    /// make sure that every pair of field slices can be packed.
    for (auto i = alloc.begin(); i != alloc.end(); ++i) {
        for (auto j = std::next(i); j != alloc.end(); ++j) {
            if (auto *err = can_pack(*i, *j, add_mau_inits)) {
                return err;
            }
        }
    }
    return nullptr;
}

void ParserPackingValidator::set_trivial_pass(bool triv) const { is_trivial_pass = triv; }

void ParserPackingValidator::set_trivial_alloc(bool triv) const { is_trivial_alloc = triv; }
}  // namespace v2
}  // namespace PHV
