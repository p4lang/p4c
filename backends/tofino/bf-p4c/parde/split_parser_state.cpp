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

#include "split_parser_state.h"

#include "bf-p4c/common/utils.h"
#include "bf-p4c/parde/parde_utils.h"
#include "bf-p4c/parde/parser_query.h"

class DumpSplitStates : public DotDumper {
 public:
    explicit DumpSplitStates(std::string filename) : DotDumper(filename, true) {
        out.str(std::string());
        out << "digraph states { " << std::endl;
    }

    ~DumpSplitStates() { done(); }

    // call this in gdb to dump the graph till current iteration
    void done() {
        out << "}" << std::endl;

        if (cid > 1 && LOGGING(4)) {  // only dump if state gets split
            if (auto fs = open_file(INGRESS, 0, "debug/split_parser_state"_cs)) write_to_file(fs);
        }
    }

    unsigned cid = 0;
    std::string prev_tail;

    void add_cluster(std::vector<const IR::BFN::ParserState *> states) {
        cluster_name = "cluster_" + std::to_string(cid) + "_";

        out << "subgraph cluster_" << cid << " {" << std::endl;
        out << "label=\"iteration " << cid << "\"";
        out << "size=\"8,5\"" << std::endl;

        gress_t gress;

        for (auto s : states) {
            dump(s);
            gress = s->thread();
        }

        for (auto s : states) {
            for (auto t : s->transitions) {
                out << to_label("State", s) << " -> ";
                if (t->next)
                    out << to_label("State", t->next);
                else
                    out << to_label(::toString(gress) + "_pipe");

                out << " [ ";
                dump(t);
                out << " ]" << std::endl;
            }
        }

        out << "}" << std::endl;

        if (!prev_tail.empty()) {
            out << prev_tail << " -> " << to_label("State", states.front());
            out << " [ color=\"red\" ]";
            out << std::endl;
        }

        prev_tail = to_label("State", states.back());

        cluster_name = "";
        cid++;
    }
};

/// Slices each extract into multiple extracts that each write into a single PHV container or CLOT.
/// This prepares for the subsequent state-splitting pass, where we may need to allocate the sliced
/// extracts into different states (this is the case when field straddles the input-buffer
/// boundary).
struct SliceExtracts : public ParserModifier {
    const PhvInfo &phv;
    const ClotInfo &clot;
    const ParserQuery pq;

    const IR::BFN::ParserState *state = nullptr;

    SliceExtracts(const PhvInfo &phv, const ClotInfo &clot, const CollectParserInfo &parser_info,
                  const MapFieldToParserStates &field_to_states)
        : phv(phv), clot(clot), pq(parser_info, field_to_states) {}

    /// Rewrites an extract so that it extracts exactly the slice specified by @p le_low_idx and
    /// @p width, and marks the extract with the given extract type.
    ///
    /// @param extract extract to be rewritten.
    /// @param le_low_idx the little-endian index for the low end of the slice.
    /// @param width the width of the slice, in bits.
    /// @param extract_base The low index of the extract destination ignoring slicing.
    /// For example, for extract inbuf bit[112..175] to ig_md.f[95:32], assume ig_md.f[95:64] and
    /// ig_md.f[63:32] are allocated to two 32-bit container. Two make_slice will be called. For
    /// the first make_slice, le_low_idx is 32, width is 32 and extract_base is 32. For the second
    /// make_slice, le_low_idx is 64, width is 32 and extract_base is 32.
    template <class Extract>
    const Extract *make_slice(const IR::BFN::Extract *extract, int le_low_idx, int width,
                              int extract_base, std::optional<PHV::Container> cont = std::nullopt) {
        auto slice_lo = le_low_idx;
        auto slice_hi = slice_lo + width - 1;

        auto dest = extract->dest->field;
        while (auto s = dest->to<IR::Slice>()) dest = s->e0;
        if (width != phv.field(dest)->size) dest = IR::Slice::make(dest, slice_lo, slice_hi);

        auto dest_lval = new IR::BFN::FieldLVal(dest);

        IR::BFN::ParserRVal *src_slice = nullptr;

        if (auto src = extract->source->to<IR::BFN::InputBufferRVal>()) {
            auto src_hi = src->range.hi - slice_lo + extract_base;
            auto src_lo = src_hi - width + 1;

            if (auto *pkt_rval = src->to<IR::BFN::PacketRVal>())
                src_slice = new IR::BFN::PacketRVal(nw_bitrange(src_lo, src_hi),
                                                    pkt_rval->partial_hdr_err_proc);
            else if (src->is<IR::BFN::MetadataRVal>())
                src_slice = new IR::BFN::MetadataRVal(nw_bitrange(src_lo, src_hi));
            else
                BUG("expect source to be from input buffer");
        } else if (auto c = extract->source->to<IR::BFN::ConstantRVal>()) {
            auto const_slice = *(c->constant) >> (slice_lo - extract_base);
            const_slice = const_slice & IR::Constant::GetMask(width);

            BUG_CHECK(const_slice.fitsUint(), "Constant slice larger than 32-bit?");

            if (const_slice.asUnsigned() ||
                (extract->write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE &&
                 !(cont && pq.find_inits(*cont).count(extract) &&
                   width == static_cast<int>(cont->size())))) {
                src_slice =
                    new IR::BFN::ConstantRVal(IR::Type::Bits::get(width), const_slice.asUnsigned());
            } else {
                LOG4("Skipping extract of " << extract << " to " << const_slice);
                return nullptr;
            }
        } else {
            BUG("unknown extract source");
        }

        auto sliced = new Extract(dest_lval, src_slice);
        sliced->write_mode = extract->write_mode;
        sliced->original_state = state;
        return sliced;
    }

    /// Breaks up an Extract into a series of Extracts according to the extracted field's PHV and
    /// CLOT allocation.
    std::vector<const IR::BFN::Extract *> slice_extract(const IR::BFN::Extract *extract) {
        LOG4("rewrite " << extract << " as:");
        std::vector<const IR::BFN::Extract *> rv;

        // Keeps track of which bits have been allocated, for sanity checking.
        bitvec bits_allocated;
        PHV::FieldUse use(PHV::FieldUse::WRITE);
        // Slice according to the field's PHV allocation.
        le_bitrange range;
        auto field = phv.field(extract->dest->field, &range);

        // extract_base should be used to indicate the base index of extract->dest->field. For a
        // header field, the base index is always zero, because every part of a header should be
        // extracted to clot or a phv container. However, consider the following case:
        // extract inbuf bit[144..175] to ingress::ig_md.ip_dst_addr[95:64], the extract_base of
        // ig_md.ip_dst_addr[95:64] should be 64, since any other slices in ig_md.ip_dst_addr are
        // not extracted.
        int extract_base = range.lo;
        if (!field->metadata && !field->padding) {
            BUG_CHECK(extract_base == 0,
                      "header field slice does not start from 0; instead %1% for %2%", extract_base,
                      extract);
        }
        for (auto slice : phv.get_alloc(extract->dest->field, PHV::AllocContext::PARSER, &use)) {
            auto lo = slice.field_slice().lo;
            auto size = slice.width();
            auto cont = slice.container();

            if (auto sliced =
                    make_slice<IR::BFN::ExtractPhv>(extract, lo, size, extract_base, cont)) {
                rv.push_back(sliced);
                LOG4("  PHV: " << sliced);
            }

            bits_allocated.setrange(lo, size);
        }

        // Slice according to the field's CLOT allocation.
        for (auto kv : *clot.slice_clots(field)) {
            BUG_CHECK(!field->metadata, "metadata should not have clot allocation.");
            auto slice = kv.first;
            auto lo = slice->range().lo;
            auto size = slice->size();

            // Only header fields have clot allocation, so extract_base should always be zero.
            if (auto sliced = make_slice<IR::BFN::ExtractClot>(extract, lo, size, extract_base)) {
                rv.push_back(sliced);
                LOG4("  CLOT: " << sliced);
            }

            bits_allocated.setrange(lo, size);
        }
        BUG_CHECK(bits_allocated == bitvec(range.lo, range.hi - range.lo + 1),
                  "Extracted field %s (%s) received an incomplete allocation %s", field->name,
                  bits_allocated, range);

        return rv;
    }

    bool preorder(IR::BFN::ParserState *state) override {
        this->state = state;
        IR::Vector<IR::BFN::ParserPrimitive> new_statements;

        for (auto stmt : state->statements) {
            if (auto extract = stmt->to<IR::BFN::Extract>()) {
                if (!extract->dest->is<IR::BFN::MatchLVal>()) {
                    auto sliced = slice_extract(extract);
                    new_statements.insert(new_statements.end(), sliced.begin(), sliced.end());
                    continue;
                }
            }

            new_statements.push_back(stmt);
        }

        state->statements = new_statements;

        SortExtracts sort(state);

        return true;
    }
};

struct AllocateParserState : public ParserTransform {
    const PhvInfo &phv;
    ClotInfo &clot;

    AllocateParserState(const PhvInfo &phv, ClotInfo &clot) : phv(phv), clot(clot) {}

    class ParserStateAllocator {
        struct HasPacketRVal : Inspector {
            bool rv = false;

            bool preorder(const IR::BFN::PacketRVal *) override {
                rv = true;
                return false;
            }
        };

        void sort_state_primitives() {
            for (auto p : state->statements) {
                if (auto e = p->to<IR::BFN::ExtractPhv>()) {
                    phv_extracts.push_back(e);
                } else if (auto e = p->to<IR::BFN::ExtractClot>()) {
                    clot_extracts.push_back(e);
                } else if (auto c = p->to<IR::BFN::ParserChecksumPrimitive>()) {
                    checksums.push_back(c);
                } else if (auto pc = p->to<IR::BFN::ParserCounterPrimitive>()) {
                    counters.push_back(pc);
                } else if (auto e = p->to<IR::BFN::Extract>()) {
                    if (e->dest->is<IR::BFN::MatchLVal>()) {
                        others.push_back(e);
                    } else {
                        BUG("Unexpected extract encountered during state-splitting: %1%", e);
                    }
                } else {
                    others.push_back(p);
                }
            }
            reorderExtracts();
        }

        /// @brief reorder extracts by container start offset within the buffer + container
        ///
        /// ExtractAllocator looks at only the previous extract when considering whether to start a
        /// new group. By ordering by container, we keeps extracts together as much as possible.
        void reorderExtracts() {
            std::vector<const IR::BFN::ExtractPhv *> const_extracts;
            std::map<int, std::map<PHV::Container, std::vector<const IR::BFN::ExtractPhv *>>>
                extracts_by_offset;

            PHV::FieldUse use(PHV::FieldUse::WRITE);
            for (auto *extract : phv_extracts) {
                if (auto *source = extract->source->to<IR::BFN::InputBufferRVal>()) {
                    auto slices =
                        phv.get_alloc(extract->dest->field, PHV::AllocContext::PARSER, &use);

                    // Extracts should have been split so that there is exactly one slice per
                    // extract at this point in the compiler.
                    BUG_CHECK(slices.size() == 1,
                              "Expected extract %1% to correspond with 1 slice but saw %2% slices",
                              extract, slices.size());

                    auto &slice = slices.front();
                    auto buffer_range = source->interval();
                    auto container = slice.container();
                    unsigned int start_byte =
                        (buffer_range.lo - (container.size() - slice.container_slice().hi - 1)) / 8;
                    extracts_by_offset[start_byte][container].push_back(extract);

                } else {
                    const_extracts.push_back(extract);
                }
            }

            // Build the sorted phv_extracts vector
            phv_extracts.clear();
            for (auto &[offset, extracts_by_container] : extracts_by_offset) {
                for (auto &[container, extracts] : extracts_by_container) {
                    for (auto *extract : extracts) phv_extracts.push_back(extract);
                }
            }
            for (auto *extract : const_extracts) phv_extracts.push_back(extract);
        }

        struct OutOfBuffer : Inspector {
            bool lo = false, hi = false;
            bool preorder(const IR::BFN::PacketRVal *rval) override {
                auto max = Device::pardeSpec().byteInputBufferSize() * 8;
                hi |= rval->range.hi >= max;
                lo |= rval->range.lo >= max;
                return false;
            }
        };

        template <typename T>
        static bool out_of_buffer(const T *p) {
            OutOfBuffer oob;
            p->apply(oob);
            return oob.lo;
        }

        template <typename T>
        static bool straddles_buffer(const T *p) {
            OutOfBuffer oob;
            p->apply(oob);
            return !oob.lo && oob.hi;
        }

        template <typename T>
        static bool within_buffer(const T *p) {
            OutOfBuffer oob;
            p->apply(oob);
            return !oob.hi;
        }

        struct Allocator {
            ParserStateAllocator &sa;

            IR::Vector<IR::BFN::ParserPrimitive> current, spilled;

            explicit Allocator(ParserStateAllocator &sa) : sa(sa) {}

            virtual void allocate() = 0;

            void add_to_result() {
                for (auto c : current) sa.current_statements.push_back(c);

                for (auto s : spilled) sa.spilled_statements.push_back(s);
            }
        };

        struct ExtractAllocator : Allocator {
            std::list<std::pair<PHV::Container, ordered_set<const IR::BFN::ExtractPhv *>>>
                container_to_extracts;

            // FIXME: Should look at the actual extractions (could extract to half a 32b
            // container for JBay), but requires consistent and accurate analysis across all passes
            // and assembler.
            //
            // Problem is that code currently calculates a constant number of extracts per
            // container, but JBay can extract only half of a 32b container if only some bits are
            // being written.
            virtual std::pair<size_t, unsigned> inbuf_extractor_use(size_t container_size) = 0;

            virtual std::map<size_t, unsigned> constant_extractor_use_choices(
                uint32_t value, size_t container_size) = 0;

            std::map<size_t, unsigned> constant_extractor_use_choices(
                PHV::Container container,
                const ordered_set<const IR::BFN::ExtractPhv *> &extracts) {
                std::map<size_t, unsigned> rv;
                bool has_clr_on_write = false;

                unsigned c = merge_const_source(extracts, has_clr_on_write);

                if (c || has_clr_on_write) {
                    rv = constant_extractor_use_choices(c, container.size());

                    LOG4("constant: " << c);

                    for (auto &kv : rv)
                        LOG4("extractors needed: " << kv.first << " : " << kv.second);
                }

                return rv;
            }

            std::map<size_t, unsigned> inbuf_extractor_needed(
                PHV::Container container,
                const ordered_set<const IR::BFN::ExtractPhv *> &extracts) {
                std::map<size_t, unsigned> rv;

                if (has_inbuf_extract(extracts)) {
                    auto iu = inbuf_extractor_use(container.size());
                    rv.insert(iu);
                }

                return rv;
            }

            bool has_inbuf_extract(const ordered_set<const IR::BFN::ExtractPhv *> &extracts) {
                for (auto e : extracts) {
                    if (e->source->is<IR::BFN::InputBufferRVal>()) return true;
                }
                return false;
            }

            unsigned merge_const_source(const ordered_set<const IR::BFN::ExtractPhv *> &extracts,
                                        bool &has_clr_on_write) {
                unsigned merged = 0;
                PHV::FieldUse use(PHV::FieldUse::WRITE);

                for (auto e : extracts) {
                    if (auto c = e->source->to<IR::BFN::ConstantRVal>()) {
                        if (!c->constant->fitsUint())
                            BUG("large constants should have already been sliced");

                        auto alloc_slices =
                            sa.phv.get_alloc(e->dest->field, PHV::AllocContext::PARSER, &use);

                        BUG_CHECK(alloc_slices.size() == 1,
                                  "extract allocator expects dest to be individual slice");

                        merged |= c->constant->asUnsigned() << alloc_slices[0].container_slice().lo;

                        if (e->write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE)
                            has_clr_on_write = true;
                    }
                }

                return merged;
            }

            bool extract_out_of_buffer(const IR::BFN::ExtractPhv *e) {
                GetExtractBufferPos get_buf_pos(sa.phv);
                e->apply(get_buf_pos);

                return get_buf_pos.max > Device::pardeSpec().byteInputBufferSize() * 8;
            }

            void allocate() override {
                std::map<size_t, unsigned> extractors_by_size;

                std::map<size_t, unsigned> constants_by_size;

                std::map<size_t, unsigned> csum_verify_by_size;

                PHV::FieldUse use(PHV::FieldUse::WRITE);

                // reserve extractor for checksum verification
                for (auto c : sa.current_statements) {
                    if (auto v = c->to<IR::BFN::ChecksumVerify>()) {
                        if (v->dest) {
                            le_bitrange bits;
                            auto f = sa.phv.field(v->dest->field, &bits);
                            auto alloc_slices = sa.phv.get_alloc(
                                f, &bits, PHV::AllocContext::PARSER, &use);  // TODO

                            BUG_CHECK(alloc_slices.size() == 1,
                                      "extract allocator expects dest to be individual slice");

                            auto slice = alloc_slices[0];
                            auto container = slice.container();

                            BUG_CHECK(container.size() != 32,
                                      "checksum verification cannot be 32-bit container");

                            extractors_by_size[container.size()]++;
                            csum_verify_by_size[container.size()]++;

                            // reserve a dummy for checksum verification
                            // see MODEL-210 for discussion.

                            if (Device::currentDevice() == Device::TOFINO)
                                extractors_by_size[container.size()]++;

                            LOG2("reserved " << container.size()
                                             << "b extractor for checksum verification");
                        }
                    }
                }

                for (auto &kv : container_to_extracts) {
                    auto container = kv.first;
                    auto &extracts = kv.second;

                    auto ibuf_needed = inbuf_extractor_needed(container, extracts);

                    auto constant_choices = constant_extractor_use_choices(container, extracts);

                    bool constant_avail = true;

                    if (!constant_choices.empty()) {
                        if (Device::currentDevice() == Device::TOFINO) {
                            std::map<size_t, unsigned> valid_choices;

                            for (auto &choice : constant_choices) {
                                if (choice.first == 16 || choice.first == 32) {
                                    if (choice.second + constants_by_size[choice.first] > 2)
                                        continue;
                                }
                                // For narrow-to-wide extractions: verify
                                // extractors with constants are correctly aligned.
                                //
                                // Only a problem when generating 1 checksum validation result
                                // because this pushes extractor results back be 1 position.
                                // E.g.,
                                //   extractor 0's output -> result bus 1
                                //   extractor 1's output -> result bus 2
                                // The narrow-to-wide pair is not on an even+odd result bus pair.
                                //
                                // Only consider 16b extractors: limited to 2 constants.
                                // Don't consider 32b extractors: no 64b containers.
                                // Don't consider 8b extractors: all support constants.
                                if (choice.first < container.size() &&
                                    csum_verify_by_size[choice.first] == 1 && choice.first == 16)
                                    continue;

                                valid_choices.insert(choice);
                            }

                            constant_choices = valid_choices;
                            constant_avail = !valid_choices.empty();
                        } else if (Device::currentDevice() == Device::JBAY) {
                            constant_avail = constant_choices.at(16) + constants_by_size[16] <= 2;
                        }
                    }

                    bool extractor_avail = true;

                    std::pair<size_t, unsigned> constant_needed;

                    std::map<size_t, unsigned> total_needed;

                    if (!constant_choices.empty()) {
                        extractor_avail = false;

                        for (auto it = constant_choices.rbegin(); it != constant_choices.rend();
                             ++it) {
                            auto choice = *it;

                            total_needed = ibuf_needed;

                            if (total_needed.count(choice.first))
                                total_needed[choice.first] += choice.second;
                            else
                                total_needed.insert(choice);

                            bool choice_ok = true;

                            for (auto &kv : total_needed) {
                                auto avail = Device::pardeSpec().extractorSpec().at(kv.first);
                                if (extractors_by_size[kv.first] + kv.second > avail) {
                                    choice_ok = false;
                                    break;
                                }
                            }

                            if (choice_ok) {
                                extractor_avail = true;
                                constant_needed = choice;
                                break;
                            }
                        }
                    } else {
                        total_needed = ibuf_needed;

                        for (auto &kv : total_needed) {
                            auto avail = Device::pardeSpec().extractorSpec().at(kv.first);
                            if (extractors_by_size[kv.first] + kv.second > avail) {
                                extractor_avail = false;
                                break;
                            }
                        }
                    }

                    bool oob = false;

                    if (extractor_avail && constant_avail) {
                        for (auto e : extracts) {
                            if (extract_out_of_buffer(e)) {
                                oob = true;
                                break;
                            }
                        }
                    }

                    if (!oob && extractor_avail && constant_avail) {
                        // allocate
                        for (auto e : extracts) current.push_back(e);

                        for (auto &kv : total_needed) extractors_by_size[kv.first] += kv.second;

                        constants_by_size[constant_needed.first] += constant_needed.second;
                    } else {
                        std::stringstream reason;

                        if (oob) reason << "(out of buffer) ";
                        if (!extractor_avail) reason << "(ran out of extractors) ";
                        if (!constant_avail) reason << "(ran out of constants) ";

                        for (auto e : extracts) LOG3("spill " << e << " { " << reason.str() << "}");

                        // spill
                        for (auto e : extracts) spilled.push_back(e);
                    }
                }
            }

            explicit ExtractAllocator(ParserStateAllocator &sa) : Allocator(sa) {
                PHV::FieldUse use(PHV::FieldUse::WRITE);
                for (auto e : sa.phv_extracts) {
                    auto alloc_slices =
                        sa.phv.get_alloc(e->dest->field, PHV::AllocContext::PARSER, &use);

                    BUG_CHECK(!alloc_slices.empty(), "No slices allocated for %1%", e);
                    BUG_CHECK(alloc_slices.size() == 1,
                              "extract allocator expects dest to be individual slice for %1%", e);

                    auto slice = alloc_slices[0];
                    auto container = slice.container();

                    if (e->source->is<IR::BFN::InputBufferRVal>()) {
                        // Only allow the merge with the last extract
                        if (container_to_extracts.empty() ||
                            container_to_extracts.back().first != container) {
                            container_to_extracts.push_back(std::make_pair(
                                container, ordered_set<const IR::BFN::ExtractPhv *>()));
                        }
                        container_to_extracts.back().second.insert(e);
                    } else {
                        // Try to merge it with another extract related to the same container
                        // because the source is a constant and input buffer is irrelevant
                        bool merged = false;
                        for (auto &kv : container_to_extracts) {
                            if (kv.first == container) {
                                kv.second.insert(e);
                                merged = true;
                                break;
                            }
                        }
                        if (!merged) {
                            container_to_extracts.push_back(std::make_pair(
                                container, ordered_set<const IR::BFN::ExtractPhv *>()));
                            container_to_extracts.back().second.insert(e);
                        }
                    }
                }
            }
        };

        class TofinoExtractAllocator : public ExtractAllocator {
            bool can_extract(unsigned val, unsigned extractor_size) {
                if (val == 0) return true;

                switch (extractor_size) {
                    case 32:
                        for (int i = 0; i < 32; i++) {
                            if ((val & 1) && (0x7 & val) == val) return true;
                            val = ((val >> 1) | (val << 31)) & 0xffffffffU;
                        }
                        return false;
                    case 16:
                        if ((val >> 16) && !can_extract(val >> 16, extractor_size)) return false;
                        val &= 0xffff;
                        for (int i = 0; i < 16; i++) {
                            if ((val & 1) && (0xf & val) == val) return true;
                            val = ((val >> 1) | (val << 15)) & 0xffffU;
                        }
                        return false;
                    case 8:
                        return true;
                }

                return false;
            }

            std::map<size_t, unsigned> constant_extractor_use_choices(
                unsigned value, size_t container_size) override {
                std::map<size_t, unsigned> rv;

                for (const auto extractor_size : {PHV::Size::b32, PHV::Size::b16, PHV::Size::b8}) {
                    // can not use larger extractor on smaller container
                    if (container_size < size_t(extractor_size)) continue;

                    if (can_extract(value, unsigned(extractor_size)))
                        rv[size_t(extractor_size)] = container_size / unsigned(extractor_size);
                }

                BUG_CHECK(!rv.empty(), "Impossible constant value write in parser: %1%", value);

                return rv;
            }

            std::pair<size_t, unsigned> inbuf_extractor_use(size_t container_size) override {
                return {container_size, 1};
            }

         public:
            explicit TofinoExtractAllocator(ParserStateAllocator &sa) : ExtractAllocator(sa) {
                allocate();
                add_to_result();
            }
        };

        class JBayExtractAllocator : public ExtractAllocator {
            std::map<size_t, unsigned> constant_extractor_use_choices(
                uint32_t value, size_t container_size) override {
                std::map<size_t, unsigned> rv;

                unsigned num = 0;

                if (container_size == size_t(PHV::Size::b32) && value)
                    num = bool(value & 0xffff) + bool(value >> 16);
                else
                    num = 1;

                rv[16] = num;

                return rv;
            }

            std::pair<size_t, unsigned> inbuf_extractor_use(size_t container_size) override {
                return {16, container_size == 32 ? 2 : 1};
            }

         public:
            explicit JBayExtractAllocator(ParserStateAllocator &sa) : ExtractAllocator(sa) {
                allocate();
                add_to_result();
            }
        };

        struct ClipHi : Modifier {
            bool preorder(IR::BFN::PacketRVal *rval) override {
                rval->range.hi = Device::pardeSpec().byteInputBufferSize() * 8 - 1;
                return false;
            }
        };

        struct ClipLo : Modifier {
            bool preorder(IR::BFN::PacketRVal *rval) override {
                rval->range.lo = Device::pardeSpec().byteInputBufferSize() * 8;
                return false;
            }
        };

        struct ChecksumAllocator : Allocator {
            std::pair<const IR::BFN::ParserPrimitive *, const IR::BFN::ParserPrimitive *>
            slice_by_buffer(const IR::BFN::ParserPrimitive *c) {
                BUG_CHECK(c->is<IR::BFN::ChecksumAdd>() || c->is<IR::BFN::ChecksumSubtract>(),
                          "unexpected checksum primitive %1%", c);

                auto current = c->apply(ClipHi());
                auto spilled = c->apply(ClipLo());

                return {current->to<IR::BFN::ParserPrimitive>(),
                        spilled->to<IR::BFN::ParserPrimitive>()};
            }

            struct GetPacketRVal : Inspector {
                const IR::BFN::PacketRVal *rv = nullptr;

                bool preorder(const IR::BFN::PacketRVal *rval) override {
                    rv = rval;
                    return false;
                }
            };

            void sort_by_buffer_pos(std::vector<const IR::BFN::ParserChecksumPrimitive *> &prims) {
                std::stable_sort(prims.begin(), prims.end(),
                                 [&](const IR::BFN::ParserChecksumPrimitive *a,
                                     const IR::BFN::ParserChecksumPrimitive *b) {
                                     GetPacketRVal va, vb;

                                     a->apply(va);
                                     b->apply(vb);

                                     return (va.rv && vb.rv) ? va.rv->range < vb.rv->range : false;
                                 });
            }
            template <typename T>
            bool find_spill_needed_for_dup(std::map<nw_bitrange, int> &masked_ranges_map,
                                           const T *csum_entry) {
                GetPacketRVal gpr;
                csum_entry->apply(gpr);
                if (!gpr.rv) return false;
                auto r = gpr.rv->range;
                if (masked_ranges_map.count(r)) {
                    if (Device::currentDevice() == Device::TOFINO) {
                        masked_ranges_map[r]++;
                        return true;
                    } else if (Device::currentDevice() == Device::JBAY) {
                        if (masked_ranges_map[r] % 2 == 0 || r.size() % 16) {
                            masked_ranges_map[r]++;
                            return true;
                        }
                    } else {
                        BUG("unhandled device");
                    }
                }
                masked_ranges_map[r]++;
                return false;
            }

            void allocate() override {
                ordered_map<cstring, std::vector<const IR::BFN::ParserChecksumPrimitive *>>
                    decl_to_checksums;

                for (auto c : sa.checksums) decl_to_checksums[c->declName].push_back(c);

                for (auto &kv : decl_to_checksums) {
                    sort_by_buffer_pos(kv.second);
                }

                for (auto &kv : decl_to_checksums) {
                    bool oob = false;
                    std::map<nw_bitrange, int> masked_ranges_map;
                    for (auto c : kv.second) {
                        if (oob) {
                            spilled.push_back(c);
                        } else if (within_buffer(c)) {
                            if (find_spill_needed_for_dup(masked_ranges_map, c)) {
                                spilled.push_back(c);
                                LOG3("spill " << c << " (checksum entry duplication)");
                                oob = true;
                            } else {
                                current.push_back(c);
                            }
                        } else if (out_of_buffer(c) || (c->is<IR::BFN::ChecksumResidualDeposit>() &&
                                                        straddles_buffer(c))) {
                            spilled.push_back(c);
                            LOG3("spill " << c << " (out of buffer)");
                            oob = true;
                        } else if (straddles_buffer(c)) {
                            auto sliced = slice_by_buffer(c);
                            current.push_back(sliced.first);
                            spilled.push_back(sliced.second);
                            LOG3("spill " << c << " (straddles buffer)");
                            oob = true;
                        }
                    }
                }
            }

            explicit ChecksumAllocator(ParserStateAllocator &sa) : Allocator(sa) {
                allocate();
                add_to_result();
            }
        };

        struct CounterAllocator : Allocator {
            void allocate() override {
                std::map<cstring, std::vector<const IR::BFN::ParserCounterPrimitive *>>
                    decl_to_counters;

                for (auto c : sa.counters) decl_to_counters[c->declName].push_back(c);

                // HW only allows one counter primitive per state
                // If successive primitives are add/sub, can constant-fold them
                // into single primitive? TODO

                for (auto &kv : decl_to_counters) {
                    for (auto c : kv.second) {
                        if (current.empty() && within_buffer(c)) {
                            current.push_back(c);
                        } else {
                            spilled.push_back(c);
                            LOG3("spill " << c);
                        }
                    }
                }
            }

            explicit CounterAllocator(ParserStateAllocator &sa) : Allocator(sa) {
                allocate();
                add_to_result();
            }
        };

        struct ClotAllocator : Allocator {
            const IR::BFN::ExtractClot *create_extract(const IR::BFN::FieldLVal *dest,
                                                       const IR::BFN::PacketRVal *rval) {
                auto *extract_clot = new IR::BFN::ExtractClot(dest, rval);
                extract_clot->original_state = sa.state;
                return extract_clot;
            }

            const IR::BFN::FieldLVal *slice_dest(const IR::BFN::ExtractClot *extract, int slice_hi,
                                                 int slice_lo) {
                auto dest = extract->dest->field;
                auto dest_slice = IR::Slice::make(dest, slice_lo, slice_hi);
                auto dest_slice_lval = new IR::BFN::FieldLVal(dest_slice);
                return dest_slice_lval;
            }

            std::pair<const IR::BFN::ExtractClot *, const IR::BFN::ExtractClot *> slice_by_buffer(
                const IR::BFN::ExtractClot *extract) {
                auto rval = extract->source->to<IR::BFN::PacketRVal>();

                BUG_CHECK(rval, "unexpected source type for CLOT extract %1%", extract);

                auto extract_size = rval->range.size();

                auto current_slice_size = 256 - rval->range.lo;
                auto current_slice_lo = extract_size - current_slice_size;

                auto current =
                    create_extract(slice_dest(extract, extract_size - 1, current_slice_lo),
                                   rval->apply(ClipHi())->to<IR::BFN::PacketRVal>());

                auto spilled = create_extract(slice_dest(extract, current_slice_lo - 1, 0),
                                              rval->apply(ClipLo())->to<IR::BFN::PacketRVal>());

                return {current, spilled};
            }

            /* test to see if an access running past the end of the input buffer is due
             * to a lookahead rather than an extract.  FIXME -- this is a guess */
            bool buffer_is_lookahead() {
                for (auto t : sa.state->transitions)
                    if (static_cast<int>(t->shift) >= Device::pardeSpec().byteInputBufferSize())
                        return false;
                return !sa.state->transitions.empty();
            }

            void allocate() override {
                ordered_map<Clot *, ordered_set<const IR::BFN::ExtractClot *>> clot_to_extracts;

                for (auto e : sa.clot_extracts) {
                    le_bitrange range;
                    auto field = sa.phv.field(e->dest->field, &range);
                    auto slice = new PHV::FieldSlice(field, range);

                    for (auto entry : *sa.clot.slice_clots(slice)) {
                        auto clot = entry.second;
                        clot_to_extracts[clot].insert(e);
                    }
                }

                unsigned allocated_in_state = 0;

                for (auto &kv : clot_to_extracts) {
                    if (allocated_in_state < Device::pardeSpec().maxClotsPerState()) {
                        bool allocated = false;

                        for (auto e : kv.second) {
                            if (out_of_buffer(e)) {
                                spilled.push_back(e);
                                LOG3("spill " << e << " (out of buffer)");
                            } else if (within_buffer(e)) {
                                LOG4("have " << e << " (in buffer)");
                                current.push_back(e);
                                allocated = true;
                            } else if (straddles_buffer(e)) {
                                if (e->is<IR::BFN::ExtractClot>() && buffer_is_lookahead()) {
                                    /* CLOTs don't need to be all in the buffer -- they'll
                                     * spool in.  However if it has already been split due
                                     * to a checksum, we need to split it consistently */
                                    current.push_back(e);
                                    LOG4("have " << e << " (clot)");
                                } else {
                                    auto sliced = slice_by_buffer(e);
                                    current.push_back(sliced.first);
                                    spilled.push_back(sliced.second);
                                    LOG4("have " << sliced.first << " (in buffer)");
                                    LOG3("spill " << sliced.second << " (out of buffer)");
                                }
                                allocated = true;
                            }
                        }

                        if (allocated) allocated_in_state++;
                    } else {
                        for (auto e : kv.second) spilled.push_back(e);
                    }
                }
            };

            explicit ClotAllocator(ParserStateAllocator &sa) : Allocator(sa) {
                allocate();
                add_to_result();
            }
        };

        void allocate() {
            sort_state_primitives();

            ChecksumAllocator csum(*this);

            if (Device::currentDevice() == Device::TOFINO) {
                TofinoExtractAllocator tea(*this);
            } else if (Device::currentDevice() == Device::JBAY) {
                JBayExtractAllocator jea(*this);
                ClotAllocator cla(*this);
            } else {
                BUG("Unknown device");
            }

            CounterAllocator cntr(*this);

            for (auto o : others) {
                if (within_buffer(o)) {
                    current_statements.push_back(o);
                } else {
                    spilled_statements.push_back(o);
                    LOG3("spill " << o << " (out of buffer)");
                }
            }

            // If any of the selects is oob, we need to spill all selects
            for (auto s : state->selects) {
                if (!within_buffer(s)) {
                    LOG3(state->name << " has out of buffer select");
                    spill_selects = true;
                    break;
                }
            }

            // If select on counter and state already has counter primitive
            for (auto s : state->selects) {
                if (s->source->is<IR::BFN::ParserCounterRVal>()) {
                    bool has_counter_prim = false;

                    for (auto s : current_statements) {
                        if (s->is<IR::BFN::ParserCounterPrimitive>()) {
                            has_counter_prim = true;
                            break;
                        }
                    }

                    if (has_counter_prim) {
                        LOG3(state->name << " spill counter select");
                        spill_selects = true;
                        break;
                    }
                }
            }

            // Fix up for constraint that header end position must be less than the current state's
            // shift amount for residual checksum. See MODEL-542.
            std::vector<const IR::BFN::ParserPrimitive *> to_spill;
            bool spilledGet = false;
            for (auto s : current_statements) {
                if (auto get = s->to<IR::BFN::ChecksumResidualDeposit>()) {
                    if (get->header_end_byte->range.lo >= compute_max_shift_in_bits()) {
                        spilled_statements.push_back(s);
                        to_spill.push_back(s);
                        spilledGet = true;
                        LOG3("spill checksum get (end_pos > shift_amt)");
                    }
                }
            }
            if (spilledGet) {
                for (auto s : current_statements) {
                    // We have to also spill any adds/subs to the checksum that are not
                    // extracted otherwise the computed checksum is wrong
                    // This happens if some field is extracted in the spilled state
                    // but it is added/subtracted from the checksum in the original
                    // For example lets assume we have a state that extracts fields a and b
                    // It gets split into two, the first state will extract a, shift the buffer just
                    // behind a (and in front of b), but also will have buffer required set
                    // to include b as well and the checksum will subtract b in this first state
                    // Then the second split state basically only extracts b, shifts the buffer
                    // behind it and "ends" the checksum by doing ChecksumResidualDeposit
                    // However in this case the subtract of b in the first state does not actually
                    // happen since it is not really extracted there
                    if (auto add = s->to<IR::BFN::ChecksumAdd>()) {
                        const auto *src = add->source;
                        CHECK_NULL(src);
                        if (src->range.lo >= compute_max_shift_in_bits()) {
                            spilled_statements.push_back(s);
                            to_spill.push_back(s);
                            LOG3("spill checksum add (pos > shift_amt)");
                        }
                    } else if (auto sub = s->to<IR::BFN::ChecksumSubtract>()) {
                        const auto *src = sub->source;
                        CHECK_NULL(src);
                        if (src->range.lo >= compute_max_shift_in_bits()) {
                            spilled_statements.push_back(s);
                            to_spill.push_back(s);
                            LOG3("spill checksum subtract (pos > shift_amt)");
                        }
                    }
                }
            }

            for (auto s : to_spill) {
                current_statements.erase(
                    std::remove(current_statements.begin(), current_statements.end(), s),
                    current_statements.end());
            }
        }

        struct GetExtractBufferPos : Inspector {
            const PhvInfo &phv;

            int min = Device::pardeSpec().byteInputBufferSize() * 8;
            int max = -1;

            explicit GetExtractBufferPos(const PhvInfo &phv) : phv(phv) {}

            bool preorder(const IR::BFN::ExtractPhv *extract) override {
                PHV::FieldUse use(PHV::FieldUse::WRITE);
                if (auto rval = extract->source->to<IR::BFN::PacketRVal>()) {
                    auto alloc_slices =
                        phv.get_alloc(extract->dest->field, PHV::AllocContext::PARSER, &use);

                    if (!alloc_slices.empty()) {
                        BUG_CHECK(alloc_slices.size() == 1,
                                  "extract allocator expects dest to be individual slice");

                        auto slice = alloc_slices[0];

                        const nw_bitrange container_range =
                            slice.container_slice().toOrder<Endian::Network>(
                                slice.container().size());

                        const nw_bitrange buffer_range =
                            rval->range.shiftedByBits(-container_range.lo)
                                .resizedToBits(slice.container().size());

                        min = std::min(min, buffer_range.lo);
                        max = std::max(max, buffer_range.hi);
                    }
                }

                return false;
            }
        };

     public:
        int compute_max_shift_in_bits() {
            GetMinBufferPos min_statements;
            spilled_statements.apply(min_statements);

            auto shift = min_statements.rv;

            GetExtractBufferPos min_extracts(phv);
            spilled_statements.apply(min_extracts);

            shift = std::min(shift, min_extracts.min);

            auto state_shift = get_state_shift(state);
            shift = std::min(shift, static_cast<int>(state_shift * 8));

            BUG_CHECK(shift >= 0, "Computed negative shift");

            shift = (shift / 8) * 8;  // rval may not be byte-aligned, e.g. metadata

            shift = std::min(shift, Device::pardeSpec().byteInputBufferSize() * 8);

            // Trim the shift amount to ensure that no selects start within the shift range and
            // extend beyond the parser buffer range.
            //
            // This is necessary because parser match register allocation
            // (AllocateParserMatchRegisters) will generate an illegal allocation because either the
            // select data is saved in the first state (illegal because the data extends beyond the
            // buffer range) or the second state (illegal because some of the data has already been
            // shifted out).
            bool select_changed_shift = false;
            do {
                select_changed_shift = false;
                for (auto *select : state->selects) {
                    if (auto *source = select->source->to<IR::BFN::SavedRVal>()) {
                        if (auto *rval = source->source->to<IR::BFN::InputBufferRVal>()) {
                            if (rval->range.lo < shift &&
                                rval->range.hi >= Device::pardeSpec().byteInputBufferSize() * 8) {
                                shift = (rval->range.lo / 8) * 8;
                                select_changed_shift = true;
                            }
                        }
                    }
                }
            } while (select_changed_shift);

            return shift;
        }

        ParserStateAllocator(const IR::BFN::ParserState *s, const PhvInfo &phv, ClotInfo &clot)
            : state(s), phv(phv), clot(clot) {
            allocate();
        }

        const IR::BFN::ParserState *state;

        const PhvInfo &phv;
        ClotInfo &clot;

        std::vector<const IR::BFN::ExtractPhv *> phv_extracts;
        std::vector<const IR::BFN::ExtractClot *> clot_extracts;
        std::vector<const IR::BFN::ParserChecksumPrimitive *> checksums;
        std::vector<const IR::BFN::ParserCounterPrimitive *> counters;
        std::vector<const IR::BFN::ParserPrimitive *> others;

        IR::Vector<IR::BFN::ParserPrimitive> current_statements, spilled_statements;
        bool spill_selects = false;
    };

    struct ResetHeaderStackExtraction : public Modifier {
        std::map<int, int> orig_idx_to_new_idx;
        int current_idx = 0;
        bool header_stack_present = false;
        bool preorder(IR::HeaderStackItemRef *hs) override {
            header_stack_present = true;
            int orig_idx = hs->index_->to<IR::Constant>()->asInt();
            if (orig_idx_to_new_idx.count(orig_idx)) {
                hs->index_ = new IR::Constant(orig_idx_to_new_idx.at(orig_idx));
            } else {
                orig_idx_to_new_idx[orig_idx] = current_idx;
                hs->index_ = new IR::Constant(current_idx++);
            }
            return false;
        }
    };

    class ParserStateSplitter {
        const PhvInfo &phv;
        ClotInfo &clot;

        DumpSplitStates dbg;

     public:
        ParserStateSplitter(const PhvInfo &phv, ClotInfo &clot, IR::BFN::ParserState *state)
            : phv(phv), clot(clot), dbg(state->name.string()) {
            dbg.add_cluster({state});

            auto splits = split_parser_state(state, state->name, 0);
            splits.insert(splits.begin(), state);

            if (splits.size() > 1 && LOGGING(1)) {
                LOG1(state->name << " is split into " << splits.size() << " states:");
                for (auto s : splits) LOG1("  " << s->name);
            }
        }

     private:
        IR::BFN::Transition *shift_transition(const IR::BFN::Transition *t, int shift_amt) {
            BUG_CHECK(shift_amt % 8 == 0, "Shift amount not byte-aligned?");

            auto c = t->clone();

            auto new_shift = t->shift - shift_amt / 8;
            c->shift = new_shift;
            c->saves = *(c->saves.apply(ShiftPacketRVal(shift_amt)));

            return c;
        }

        IR::BFN::ParserState *create_split_state(const IR::BFN::ParserState *state, cstring prefix,
                                                 unsigned iteration) {
            cstring split_name = prefix + ".$split_"_cs + cstring::to_cstring(iteration);
            auto split = new IR::BFN::ParserState(state->p4States, split_name, state->gress);

            LOG2("created split state " << split);
            return split;
        }

        class VerifySplitStates {
            IR::BFN::ParserState *orig = nullptr;

         public:
            explicit VerifySplitStates(const IR::BFN::ParserState *o) { orig = o->clone(); }

            void check_sanity(const IR::BFN::ParserState *o, const IR::BFN::ParserState *s,
                              bool transition_split) {
                BUG_CHECK(orig && o && s, "Sanity check on split parser states failed.");

                // If transition split happened then both states should have full selects
                if (transition_split && (o->selects.empty() || s->selects.empty()))
                    BUG("Selects not in both states?");
                // Otherwise only one of them
                if (!transition_split && !o->selects.empty() && !s->selects.empty())
                    BUG("Selects not in one state?");

                if (!o->selects.empty()) {
                    BUG_CHECK(o->selects.size() == orig->selects.size(),
                              "Select don't add up after split");
                }

                if (!s->selects.empty()) {
                    BUG_CHECK(s->selects.size() == orig->selects.size(),
                              "Select don't add up after split");
                }

                auto orig_shift = get_state_shift(orig);

                unsigned o_shift = 0;
                // If transition split happened the last transition within the state
                // has a different shift (0), so check only that one
                if (transition_split) {
                    auto t = o->transitions.back();
                    o_shift = t->shift;
                    BUG_CHECK(t->next == s, "Last transition after split to unexpected state");
                } else {
                    o_shift = get_state_shift(o);
                }
                auto s_shift = get_state_shift(s);

                int total_shift = 0;

                if (o_shift) total_shift += o_shift;
                if (s_shift) total_shift += s_shift;

                if (orig_shift)
                    BUG_CHECK(int(orig_shift) == total_shift, "Shifts don't add up after split");
            }
        };

        IR::BFN::ParserState *insert_stall_if_needed(IR::BFN::ParserState *state, cstring prefix,
                                                     unsigned idx) {
            auto shift = get_state_shift(state);

            if (int(shift) <= Device::pardeSpec().byteInputBufferSize()) return nullptr;

            cstring name = prefix + ".$stall_"_cs + cstring::to_cstring(idx);
            auto stall = new IR::BFN::ParserState(state->p4States, name, state->gress);

            auto stall_amt = Device::pardeSpec().byteInputBufferSize();
            auto new_shift = shift - stall_amt;

            IR::Vector<IR::BFN::Transition> new_transitions;

            for (auto t : state->transitions) {
                auto c = t->clone();
                c->shift = new_shift;
                new_transitions.push_back(c);
            }

            stall->transitions = new_transitions;

            stall->selects = *(state->selects.apply(ShiftPacketRVal(stall_amt * 8, true)));
            state->selects = {};

            auto to_stall = new IR::BFN::Transition(match_t(), stall_amt, stall);
            state->transitions = {to_stall};

            return stall;
        }

        std::vector<IR::BFN::ParserState *> split_parser_state(IR::BFN::ParserState *state,
                                                               cstring prefix, unsigned iteration) {
            LOG3("split_parser_state(" << state->name << ", " << prefix << ", " << iteration << ")"
                                       << IndentCtl::indent);
            ParserStateAllocator alloc(state, phv, clot);

            // No more splits = recursion end
            if (alloc.spilled_statements.empty() && !alloc.spill_selects) {
                LOG3("no need to split " << state->name << " (nothing spilled)");

                // need to insert stall if next state is not reachable from current state
                // (if the shift is greater than input buffer size)
                unsigned idx = 0;
                IR::BFN::ParserState *stall = state;
                std::vector<IR::BFN::ParserState *> splits;

                while ((stall = insert_stall_if_needed(stall, prefix, idx++))) {
                    LOG2("inserted stall after " << stall->name);
                    splits.push_back(stall);
                }

                LOG3_UNINDENT;
                return splits;
            }

            VerifySplitStates verify(state);

            auto split = create_split_state(state, prefix, iteration);

            bool transitions_split = false;
            // Regular select spill or statement split
            if (!alloc.spilled_statements.empty() || alloc.spill_selects) {
                auto max_shift = alloc.compute_max_shift_in_bits();

                LOG3("computed max shift = " << max_shift << " for split iteration " << iteration
                                             << " of " << state->name);
                LOG4(alloc.spilled_statements.size()
                     << " split, " << alloc.current_statements.size() << " current");
                if (max_shift == 0 && alloc.current_statements.empty()) {
                    error(ErrorType::ERR_OVERLIMIT, "lookahead in %s too far", state);
                    LOG3_UNINDENT;
                    return std::vector<IR::BFN::ParserState *>();
                }

                state->statements = alloc.current_statements;
                split->statements = *(alloc.spilled_statements.apply(ShiftPacketRVal(max_shift)));
                split->selects = *(state->selects.apply(ShiftPacketRVal(max_shift, true)));
                state->selects = {};
                // Determine if the current state and spilled extracts any strided header stacks
                // Reset the header stack indices and mark the appropriate state as strided.
                if (state->stride) {
                    ResetHeaderStackExtraction split_rhse;
                    split->statements = *(split->statements.apply(split_rhse));
                    if (split_rhse.header_stack_present) {
                        split->stride = true;
                    }
                    ResetHeaderStackExtraction orig_rhse;
                    state->statements.apply(orig_rhse);
                    if (!orig_rhse.header_stack_present) {
                        state->stride = false;
                    }
                }
                // move state's transitions to split state
                for (auto t : state->transitions) {
                    auto shifted = shift_transition(t, max_shift);
                    split->transitions.push_back(shifted);
                }

                auto to_split = new IR::BFN::Transition(match_t(), max_shift / 8, split);
                state->transitions = {to_split};
            } else {
                BUG("Unexpected state splitting spill");
            }

            // add to step dot dump
            dbg.add_cluster({state, split});

            // verify this iteration
            verify.check_sanity(state, split, transitions_split);

            LOG3_UNINDENT;
            // recurse
            auto splits = split_parser_state(split, prefix, ++iteration);
            splits.insert(splits.begin(), split);

            return splits;
        }
    };

    IR::BFN::ParserState *postorder(IR::BFN::ParserState *state) override {
        try {
            ParserStateSplitter(phv, clot, state);
        } catch (const Util::CompilerBug &e) {
            std::string workaround =
                "As a workaround, try applying @pragma critical on the parser state. "
                "The compiler will optimize the extractor usage of the state so that "
                "splitting may not be needed.";

            BUG("An error occurred while the compiler was splitting parser state %1%.\n%2%",
                state->name, workaround);
        }

        return state;
    }
};

/// The counter zero and negative flags are always zero in the cycle immediately following
/// a load via the Counter Initialization RAM. Therefore the compiler needs to insert a
/// stall state if is_zero/is_negative is used in the cycle immediately following the load.
struct InsertParserCounterStall : public ParserTransform {
    std::map<cstring, int> stall_counts;

    void insert_stall_state(IR::BFN::Transition *t) {
        auto src = findContext<IR::BFN::ParserState>();

        int suffix = stall_counts[src->name]++;
        cstring name = src->name + ".$ctr_stall"_cs + std::to_string(suffix).c_str();
        auto stall = new IR::BFN::ParserState(src->p4States, name, src->gress);

        LOG2("created stall state for counter select on " << src->name << " -> " << t->next->name);

        auto to_dst = new IR::BFN::Transition(match_t(), 0, t->next);
        stall->transitions.push_back(to_dst);
        t->next = stall;
    }

    bool has_counter_select(const IR::BFN::ParserState *state) {
        for (auto s : state->selects) {
            if (s->source->is<IR::BFN::ParserCounterRVal>()) return true;
        }

        return false;
    }

    bool has_counter_load(const IR::BFN::ParserState *state) {
        for (auto s : state->statements) {
            if (s->is<IR::BFN::ParserCounterLoadImm>() || s->is<IR::BFN::ParserCounterLoadPkt>()) {
                return true;
            }
        }

        return false;
    }

    IR::BFN::Transition *postorder(IR::BFN::Transition *t) override {
        auto state = findContext<IR::BFN::ParserState>();

        if (has_counter_load(state)) {
            if (t->next && has_counter_select(t->next)) insert_stall_state(t);
        }

        return t;
    }
};

/// If after state splitting, if the transition to end of parsing shifts more
/// than 32 bytes (input buffer size), we can safely clip the shift amount to
/// 32, as the remaining amount will not contribute to header length.
struct ClipTerminalTransition : ParserModifier {
    bool preorder(IR::BFN::Transition *t) override {
        if (t->next == nullptr && int(t->shift) > Device::pardeSpec().byteInputBufferSize()) {
            t->shift = Device::pardeSpec().byteInputBufferSize();
        }

        return true;
    }
};

// If after state splitting, the extraction source is still out of current
// state's input buffer, that means we cannot implement this program, i.e.
// program error.
struct CheckOutOfBufferExtracts : ParserInspector {
    bool preorder(const IR::BFN::PacketRVal *rval) override {
        if (auto extract = findContext<IR::BFN::Extract>()) {
            // CLOTs don't need to be entirely in the buffer to be extracted -- just the
            // start of the CLOT needs to be in the buffer
            if (rval->range.lo < 0 ||
                (extract->is<IR::BFN::ExtractClot>()
                     ? rval->range.lo >= Device::pardeSpec().byteInputBufferSize() * 8
                     : rval->range.hi > Device::pardeSpec().byteInputBufferSize() * 8)) {
                auto state = findContext<IR::BFN::ParserState>();

                ::fatal_error(
                    "Extraction source for %1% is out of state %2%'s input buffer"
                    " (%3% bytes)",
                    extract->dest->field, state->name, Device::pardeSpec().byteInputBufferSize());
            }
        }

        return false;
    }
};

SplitParserState::SplitParserState(const PhvInfo &phv, ClotInfo &clot,
                                   const CollectParserInfo &parser_info) {
    auto *field_to_states = new MapFieldToParserStates(phv);
    addPasses({field_to_states, LOGGING(4) ? new DumpParser("before_split_parser_states") : nullptr,
               new SliceExtracts(phv, clot, parser_info, *field_to_states),
               LOGGING(4) ? new DumpParser("after_slice_extracts") : nullptr,
               new AllocateParserState(phv, clot),
               LOGGING(4) ? new DumpParser("after_alloc_state_prims") : nullptr,
               new InsertParserCounterStall,
               LOGGING(4) ? new DumpParser("after_insert_counter_stall") : nullptr,
               new ClipTerminalTransition, new CheckOutOfBufferExtracts,
               LOGGING(4) ? new DumpParser("after_split_parser_states") : nullptr});
}
