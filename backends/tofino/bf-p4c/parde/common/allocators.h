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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_COMMON_ALLOCATORS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_COMMON_ALLOCATORS_H_

#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "ir/ir.h"

/**
 * @ingroup parde
 */
class LoweredParserMatchAllocator {
    void sort_state_primitives() {
        for (auto p : state->extracts) {
            if (auto e = p->to<IR::BFN::LoweredExtractPhv>()) {
                phv_extracts.push_back(e);
            } else if (auto e = p->to<IR::BFN::LoweredExtractClot>()) {
                clot_extracts.push_back(e);
            }
        }
        for (auto p : state->checksums) {
            if (auto c = p->to<IR::BFN::LoweredParserChecksum>()) {
                checksums.push_back(c);
            }
        }
        for (auto p : state->counters) {
            if (auto pc = p->to<IR::BFN::ParserCounterPrimitive>()) {
                counters.push_back(pc);
            }
        }
    }

    struct OutOfBuffer : Inspector {
        bool lo = false, hi = false;
        bool preorder(const IR::BFN::LoweredPacketRVal *rval) override {
            auto max = Device::pardeSpec().byteInputBufferSize() * 8;
            hi |= rval->range.hi >= max;
            lo |= rval->range.lo >= max;
            return false;
        }
    };

    template <typename T>
    static bool within_buffer(const T *p) {
        OutOfBuffer oob;
        p->apply(oob);
        return !oob.hi;
    }

    struct Allocator {
        LoweredParserMatchAllocator &lpm_allocator;

        IR::Vector<IR::BFN::LoweredParserPrimitive> current, spilled;

        explicit Allocator(LoweredParserMatchAllocator &lpm_allocator)
            : lpm_allocator(lpm_allocator) {}

        virtual void allocate() = 0;

        void add_to_result() {
            for (auto c : current) lpm_allocator.current_statements.push_back(c);

            for (auto s : spilled) lpm_allocator.spilled_statements.push_back(s);
        }
    };

    struct ExtractAllocator : Allocator {
        ordered_map<PHV::Container, ordered_set<const IR::BFN::LoweredExtractPhv *>>
            container_to_extracts;

        virtual std::pair<size_t, unsigned> inbuf_extractor_use(size_t container_size) = 0;

        virtual std::map<size_t, unsigned> constant_extractor_use_choices(
            uint32_t value, size_t container_size) = 0;

        std::map<size_t, unsigned> constant_extractor_use_choices(
            PHV::Container container,
            const ordered_set<const IR::BFN::LoweredExtractPhv *> &extracts) {
            std::map<size_t, unsigned> rv;
            bool has_clr_on_write = false;

            unsigned c = merge_const_source(extracts, has_clr_on_write);

            if (c || has_clr_on_write) {
                rv = constant_extractor_use_choices(c, container.size());

                LOG4("constant: " << c);

                for (auto &[size, count] : rv)
                    LOG4("extractors needed: " << size << " : " << count);
            }

            return rv;
        }

        std::map<size_t, unsigned> inbuf_extractor_needed(
            PHV::Container container,
            const ordered_set<const IR::BFN::LoweredExtractPhv *> &extracts) {
            std::map<size_t, unsigned> rv;

            if (has_inbuf_extract(extracts)) {
                auto iu = inbuf_extractor_use(container.size());
                rv.insert(iu);
            }

            return rv;
        }

        bool has_inbuf_extract(const ordered_set<const IR::BFN::LoweredExtractPhv *> &extracts) {
            for (auto e : extracts) {
                if (e->source->is<IR::BFN::LoweredInputBufferRVal>()) return true;
            }
            return false;
        }

        unsigned merge_const_source(const ordered_set<const IR::BFN::LoweredExtractPhv *> &extracts,
                                    bool &has_clr_on_write) {
            unsigned merged = 0;

            for (auto e : extracts) {
                if (auto c = e->source->to<IR::BFN::LoweredConstantRVal>()) {
                    merged = c->constant;

                    if (e->write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE)
                        has_clr_on_write = true;
                }
            }

            return merged;
        }

        bool extract_out_of_buffer(const IR::BFN::LoweredExtractPhv *e) {
            GetExtractBufferPos get_buf_pos;
            e->apply(get_buf_pos);

            return get_buf_pos.max > Device::pardeSpec().byteInputBufferSize() * 8;
        }

        void allocate() override {
            std::map<size_t, unsigned> extractors_by_size;

            std::map<size_t, unsigned> constants_by_size;

            std::map<size_t, unsigned> csum_verify_by_size;

            // reserve extractor for checksum verification
            for (auto c : lpm_allocator.current_statements) {
                if (auto lpc = c->to<IR::BFN::LoweredParserChecksum>()) {
                    const IR::BFN::ContainerRef *dest = lpc->phv_dest;
                    if (lpc->type == IR::BFN::ChecksumMode::VERIFY && lpc->csum_err)
                        dest = lpc->csum_err->container;
                    if (!dest) continue;

                    auto container = dest->container;
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
                                if (choice.second + constants_by_size[choice.first] > 2) continue;
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

                    for (auto it = constant_choices.rbegin(); it != constant_choices.rend(); ++it) {
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

        explicit ExtractAllocator(LoweredParserMatchAllocator &lpm_allocator)
            : Allocator(lpm_allocator) {
            PHV::FieldUse use(PHV::FieldUse::WRITE);
            for (auto e : lpm_allocator.phv_extracts) {
                auto container = e->dest->container;
                container_to_extracts[container].insert(e);
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

        std::map<size_t, unsigned> constant_extractor_use_choices(unsigned value,
                                                                  size_t container_size) override {
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
        explicit TofinoExtractAllocator(LoweredParserMatchAllocator &lpm_allocator)
            : ExtractAllocator(lpm_allocator) {
            allocate();
            add_to_result();
        }
    };

    class JBayExtractAllocator : public ExtractAllocator {
        std::map<size_t, unsigned> constant_extractor_use_choices(uint32_t value,
                                                                  size_t container_size) override {
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
        explicit JBayExtractAllocator(LoweredParserMatchAllocator &lpm_allocator)
            : ExtractAllocator(lpm_allocator) {
            allocate();
            add_to_result();
        }
    };

    void allocate() {
        sort_state_primitives();
        for (const auto &checksum : checksums) current_statements.push_back(checksum);

        if (Device::currentDevice() == Device::TOFINO) {
            TofinoExtractAllocator tea(*this);
        } else if (Device::currentDevice() == Device::JBAY) {
            JBayExtractAllocator jea(*this);
        } else {
            BUG("Unknown device");
        }

        for (auto o : others) {
            if (within_buffer(o)) {
                current_statements.push_back(o);
            } else {
                spilled_statements.push_back(o);
                LOG3("spill " << o << " (out of buffer)");
            }
        }
    }

    struct GetExtractBufferPos : Inspector {
        int min = Device::pardeSpec().byteInputBufferSize() * 8;
        int max = -1;

        bool preorder(const IR::BFN::LoweredExtractPhv *extract) override {
            if (auto rval = extract->source->to<IR::BFN::LoweredPacketRVal>()) {
                min = std::min(min, rval->range.lo);
                max = std::max(max, rval->range.hi);
            }
            return false;
        }
    };

 public:
    LoweredParserMatchAllocator(const IR::BFN::LoweredParserMatch *s, ClotInfo &clot)
        : state(s), clot(clot) {
        allocate();
    }

    const IR::BFN::LoweredParserMatch *state;

    ClotInfo &clot;

    std::vector<const IR::BFN::LoweredExtractPhv *> phv_extracts;
    std::vector<const IR::BFN::LoweredExtractClot *> clot_extracts;
    std::vector<const IR::BFN::LoweredParserChecksum *> checksums;
    std::vector<const IR::BFN::ParserCounterPrimitive *> counters;
    std::vector<const IR::BFN::LoweredParserPrimitive *> others;

    IR::Vector<IR::BFN::LoweredParserPrimitive> current_statements, spilled_statements;
    bool spill_selects = false;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_COMMON_ALLOCATORS_H_ */
