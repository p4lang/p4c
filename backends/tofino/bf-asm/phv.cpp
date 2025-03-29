/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "phv.h"

#include <algorithm>
#include <iostream>

#include "lib/log.h"
#include "misc.h"

Phv Phv::phv;
const Phv::Register Phv::Slice::invalid("<bad>", Phv::Register::NORMAL, 0, ~0, 0);

void Phv::init_phv(target_t target_type) {
    if (target) {
        BUG_CHECK(target->type() == target_type, "target type mismatch");  // sanity check
        return;
    }
    switch (target_type) {
#define INIT_FOR_TARGET(TARGET)           \
    case Target::TARGET::tag:             \
        target = new Target::TARGET::Phv; \
        break;
        FOR_ALL_TARGETS(INIT_FOR_TARGET)
        default:
            BUG("Unknown target type %d", target_type);
    }
#undef INIT_FOR_TARGET
    target->init_regs(*this);
}

void Phv::start(int lineno, VECTOR(value_t) args) {
    if (options.target == NO_TARGET) {
        error(lineno, "No target specified prior to PHV section");
        return;
    }
    init_phv(options.target);
    // The only argument to phv is the thread.  We allow phv section with no thread argument
    // which defines aliases for all threads.  Does that really make sense when threads can't
    // share registers?  We never use this capability in the compiler.
    if (args.size > 1 || (args.size == 1 && args[0] != "ingress" && args[0] != "egress" &&
                          (args[0] != "ghost" || options.target < JBAY)))
        error(lineno, "phv can only be ingress%s or egress",
              (options.target >= JBAY ? ", ghost" : 0));
}

int Phv::addreg(gress_t gress, const char *name, const value_t &what, int stage, int max_stage) {
    std::string phv_name = name;
    remove_name_tail_range(phv_name);
    if (stage == -1 && what.type == tMAP) {
        int rv = 0;
        for (auto &kv : what.map) {
            auto &key = kv.key.type == tCMD && kv.key.vec.size > 1 && kv.key == "stage" ? kv.key[1]
                                                                                        : kv.key;
            if (CHECKTYPE2(key, tINT, tRANGE)) {
                if (key.type == tINT)
                    rv |= addreg(gress, name, kv.value, key.i);
                else
                    rv |= addreg(gress, name, kv.value, key.range.lo, key.range.hi);
            }
        }
        int size = -1;
        PerStageInfo *prev = 0;
        for (auto &ch : names[gress].at(name)) {
            if (prev) {
                if (prev->max_stage >= ch.first) {
                    if (prev->max_stage != INT_MAX)
                        error(what.lineno, "Overlapping assignments in stages %d..%d for %s",
                              ch.first, prev->max_stage, name);
                    prev->max_stage = ch.first - 1;
                }
            }
            prev = &ch.second;
            if (size < 0) {
                size = ch.second.slice->size();
            } else if (size != ch.second.slice->size() && size > 0) {
                error(what.lineno, "Inconsitent sizes for %s", name);
                size = 0;
            }
        }
        if (prev && prev->max_stage >= Target::NUM_MAU_STAGES()) prev->max_stage = INT_MAX;
        add_phv_field_sizes(gress, phv_name, size);
        return rv;
    }
    if (!CHECKTYPE2M(what, tSTR, tCMD, "register or slice")) return -1;
    auto reg = what.type == tSTR ? what.s : what[0].s;
    if (const Slice *sl = get(gress, stage, reg)) {
        if (sl->valid) {
            phv_use[gress][sl->reg.uid] = true;
            user_defined[&sl->reg].first = gress;
            if (max_stage != INT_MAX) {
                /* a name that spans across stages - add it to all stages */
                for (int i = stage; i <= max_stage; i++) {
                    user_defined[&sl->reg].second[i].insert(name);
                }
            } else {
                for (int i = 0; i <= Target::NUM_MAU_STAGES(); i++) {
                    user_defined[&sl->reg].second[i].insert(name);
                }
            }
            LOG5(" Adding " << name << " to user_defined");
        }
        auto &reg = names[gress][name];
        if (what.type == tSTR) {
            reg[stage].slice = *sl;
        } else if (what.vec.size != 2) {
            error(what.lineno, "Syntax error, expecting bit or slice");
            return -1;
        } else if (!CHECKTYPE2M(what[1], tINT, tRANGE, "bit or slice")) {
            return -1;
        } else if (what[1].type == tINT) {
            reg[stage].slice = Slice(*sl, what[1].i, what[1].i);
        } else {
            reg[stage].slice = Slice(*sl, what[1].range.lo, what[1].range.hi);
        }
        reg[stage].max_stage = max_stage;
        if (!reg[stage].slice.valid) {
            auto slice = reg[stage].slice;
            error(what.lineno, "Invalid register slice - %s[%d:%d]", slice.reg.name, slice.hi,
                  slice.lo);
            return -1;
        }
        if (stage == -1) {
            add_phv_field_sizes(gress, phv_name, reg[stage].slice->size());
            if (is_pov(phv_name)) {
                phv_pov_names[sl->reg.mau_id()][reg[stage].slice.lo] = phv_name;
            }
        }
        return 0;
    } else {
        error(what.lineno, "No register named %s", reg);
        return -1;
    }
}

void Phv::input(VECTOR(value_t) args, value_t data) {
    if (!CHECKTYPE(data, tMAP)) return;
    gress_t gress =
        args[0] == "ingress"  ? INGRESS
        : args[0] == "egress" ? EGRESS
        : args[0] == "ghost" && options.target >= JBAY
            ? GHOST
            : (error(args[1].lineno, "Invalid thread %s", value_desc(args[1])), INGRESS);
    for (auto &kv : data.map) {
        if (!CHECKTYPE(kv.key, tSTR)) continue;
        if (kv.key == "context_json") {
            if (!CHECKTYPE(kv.value, tMAP)) continue;
            field_context_json.merge(*toJson(kv.value.map));
        } else {
            if (get(gress, INT_MAX, kv.key.s) || (!args.size && get(EGRESS, INT_MAX, kv.key.s)) ||
                (!args.size && get(GHOST, INT_MAX, kv.key.s))) {
                error(kv.key.lineno, "Duplicate phv name '%s'", kv.key.s);
                continue;
            }
            if (!addreg(gress, kv.key.s, kv.value) && args.size == 0) {
                addreg(EGRESS, kv.key.s, kv.value);
                if (options.target >= JBAY) addreg(GHOST, kv.key.s, kv.value);
            }
        }
    }
}

Phv::Ref::Ref(gress_t g, int stage, const value_t &n)
    : gress_(g), stage(stage), lo(-1), hi(-1), lineno(n.lineno) {
    if (CHECKTYPE2M(n, tSTR, tCMD, "phv or register reference or slice")) {
        if (n.type == tSTR) {
            name_ = n.s;
        } else {
            name_ = n[0].s;
            if (PCHECKTYPE2M(n.vec.size == 2, n[1], tINT, tRANGE, "register slice")) {
                if (n[1].type == tINT) {
                    lo = hi = n[1].i;
                } else {
                    lo = n[1].range.lo;
                    hi = n[1].range.hi;
                    if (lo > hi) {
                        lo = n[1].range.hi;
                        hi = n[1].range.lo;
                    }
                }
            }
        }
    }
}

Phv::Ref::Ref(const Phv::Register &r, gress_t gr, int l, int h)
    : gress_(gr), name_(r.name), stage(0), lo(l), hi(h < 0 ? l : h), lineno(-1) {}

bool Phv::Ref::merge(const Phv::Ref &r) {
    if (r.name_ != name_ || r.gress_ != gress_) return false;
    if (lo < 0) return true;
    if (r.lo < 0) {
        lo = hi = -1;
        return true;
    }
    if (r.hi + 1 < lo || hi + 1 < r.lo) return false;
    if (r.lo < lo) lo = r.lo;
    if (r.hi > hi) {
        lineno = r.lineno;
        hi = r.hi;
    }
    return true;
}

void merge_phv_vec(std::vector<Phv::Ref> &vec, const Phv::Ref &r) {
    int merged = -1;
    for (int i = 0; (unsigned)i < vec.size(); i++) {
        if (merged >= 0) {
            if (vec[merged].merge(vec[i])) {
                vec.erase(vec.begin() + i);
                --i;
            }
        } else if (vec[i].merge(r)) {
            merged = i;
        }
    }
    if (merged < 0) vec.push_back(r);
}

void merge_phv_vec(std::vector<Phv::Ref> &v1, const std::vector<Phv::Ref> &v2) {
    for (auto &r : v2) merge_phv_vec(v1, r);
}

std::vector<Phv::Ref> split_phv_bytes(const Phv::Ref &r) {
    std::vector<Phv::Ref> rv;
    const auto &sl = *r;
    for (unsigned byte = sl.lo / 8U; byte <= sl.hi / 8U; byte++) {
        int lo = byte * 8 - sl.lo;
        int hi = lo + 7;
        if (lo < 0) lo = 0;
        if (hi >= static_cast<int>(sl.size())) hi = sl.size() - 1;
        rv.emplace_back(r, lo, hi);
    }
    return rv;
}

std::vector<Phv::Ref> split_phv_bytes(const std::vector<Phv::Ref> &v) {
    std::vector<Phv::Ref> rv;
    for (auto &r : v) append(rv, split_phv_bytes(r));
    return rv;
}

std::string Phv::Ref::toString() const {
    std::stringstream str;
    str << *this;
    return str.str();
}

void Phv::Ref::dbprint(std::ostream &out) const {
    out << name_;
    if (lo >= 0) {
        out << '[' << hi;
        if (hi != lo) out << ":" << lo;
        out << ']';
    }
    Slice sl(**this);
    if (sl.valid) {
        out << '[';
        sl.dbprint(out);
        out << ']';
    }
}

std::string Phv::Ref::desc() const { return toString(); }

std::string Phv::Slice::toString() const {
    std::stringstream str;
    str << *this;
    return str.str();
}

void Phv::Slice::dbprint(std::ostream &out) const {
    if (valid) {
        out << reg.name;
        if (lo != 0 || hi != reg.size - 1) {
            out << '[' << hi;
            if (hi != lo) out << ":" << lo;
            out << ']';
        }
    } else {
        out << "<invalid>";
    }
}

std::string Phv::db_regset(const bitvec &s) {
    std::string rv;
    for (int reg : s) {
        if (!rv.empty()) rv += ", ";
        rv += Phv::reg(reg)->name;
    }
    return rv;
}

// For snapshot, the driver (generate pd script) generates a buffer of all phv
// fields and indexes through the buffer with a position offset to determine its
// location. It assumes the phv fields are arranged with the pov fields at the
// end. To maintain this ordering while generating the position offsets for each
// phv field, we initially generate 2 separate maps for normal and pov phv
// fields. We loop through the normap phv map first and then the pov phv map
// adding field sizes. The fields are byte aligned and put into 8/16/32 bit
// containers.
int Phv::get_position_offset(gress_t gress, std::string name) {
    int position_offset = 0;
    for (auto f : phv_field_sizes[gress]) {
        if (f.first == name) return position_offset;
        auto bytes_to_add = (f.second + 7) / 8U;
        if (bytes_to_add == 3) bytes_to_add++;
        position_offset += bytes_to_add;
    }
    for (auto f : phv_pov_field_sizes[gress]) {
        if (f.first == name) return position_offset;
        // POV should be single bit
        BUG_CHECK(f.second == 1, "POV should be single bit");
        position_offset += 1;
    }
    return 0;
}

// Output function sets the 'phv_allocation' node in context json Contains info
// on phv containers per gress (INGRESS/EGRESS) per stage Currently the phv
// containers are assumed to be present in all stages hence are replicated in
// each stage. Support for liveness indication for each container must be added
// (in assembly syntax/compiler) to set per stage phv containers correctly.
void Phv::output(json::map &ctxt_json) {
    bool warn_once = false;
    json::vector &phv_alloc = ctxt_json["phv_allocation"];
    for (int i = 0; i <= Target::NUM_MAU_STAGES(); i++) {
        json::map phv_alloc_stage;
        json::vector &phv_alloc_stage_ingress = phv_alloc_stage["ingress"] = json::vector();
        json::vector &phv_alloc_stage_egress = phv_alloc_stage["egress"] = json::vector();
        for (auto &slot : phv.user_defined) {
            unsigned phv_number = slot.first->uid;
            unsigned phv_container_size = slot.first->size;
            gress_t gress = slot.second.first;
            auto stage_usernames = slot.second.second[i];
            json::map phv_container;
            phv_container["phv_number"] = phv_number;
            phv_container["container_type"] = slot.first->type_to_string();
            json::vector &phv_records = phv_container["records"] = json::vector();
            for (auto field_name : stage_usernames) {
                LOG5("Output phv record for field : " << field_name);
                unsigned phv_lsb = 0, phv_msb = 0;
                unsigned field_lo = 0;
                int field_size = 0;
                json::map phv_record;
                auto sl = get(gress, i, field_name);
                if (!sl) continue;
                phv_lsb = sl->lo;
                phv_msb = sl->hi;
                field_lo = remove_name_tail_range(field_name, &field_size);
                auto field_width = get_phv_field_size(gress, field_name);
                if (field_size == 0) field_size = field_width;
                phv_record["position_offset"] = get_position_offset(gress, field_name);
                phv_record["field_name"] = field_name;
                phv_record["field_msb"] = field_lo + field_size - 1;
                phv_record["field_lsb"] = field_lo;
                auto field_width_bytes = (field_width + 7) / 8U;
                phv_record["field_width"] = field_width_bytes;
                phv_record["phv_msb"] = phv_msb;
                phv_record["phv_lsb"] = phv_lsb;
                // FIXME-P4C: 'is_compiler_generated' is set to false for all
                // fields except POV as there is no sure way of knowing from
                // current assembly syntax whether the field is in the header or
                // generated by the compiler. This will require additional
                // assembly syntax to convey the same. Driver does not use
                // is_compiler_generated (other than requiring it).  p4i does
                // use it for display purposes.
                phv_record["is_compiler_generated"] = false;
                phv_record["is_pov"] = false;
                if (is_pov(field_name)) {
                    phv_record["is_pov"] = true;
                    phv_record["is_compiler_generated"] = true;
                    phv_record["field_width"] = 0;
                    phv_record["position_offset"] = 0;
                    /* Now that we know that this record is representing a POV, overwrite the
                     * phv_record to call it "POV" and get rid of "$valid" */
                    phv_record["field_name"] = "POV";
                    json::vector &pov_headers = phv_record["pov_headers"] = json::vector();
                    json::map pov_header;
                    pov_header["bit_index"] = phv_lsb;
                    pov_header["position_offset"] = get_position_offset(gress, field_name);
                    pov_header["header_name"] = field_name;
                    // FIXME: Checks for reserved POV bits, not supported?
                    pov_header["hidden"] = false;
                    ;
                    pov_headers.push_back(std::move(pov_header));
                }
                // Pass through per-field context_json information from the compiler.
                if (field_context_json.count(slot.first->name)) {
                    auto add_phv_record_items = [&](int live_stage, std::string live_string) {
                        if (live_stage == -1) {
                            phv_record[live_string] = "parser";
                            return;
                        }
                        if (live_stage == Target::NUM_MAU_STAGES()) {
                            phv_record[live_string] = "deparser";
                            return;
                        }
                        phv_record[live_string] = live_stage;
                    };
                    auto container_json = field_context_json[slot.first->name];
                    BUG_CHECK(container_json, " No context_json");
                    bool field_added = false;
                    if (!container_json->as_vector()) {
                        // FIXME -- should be flexible about parsing context_json -- continue
                        // to accept a map instead of a vector here.
                        if (!warn_once) {
                            // FIXME -- would be nice to have the bfa lineno here.
                            warning(-1, "Invalid/obsolete phv context_json:, ignoring");
                            warn_once = true;
                        }
                        continue;
                    }
                    for (auto &field_json : *container_json->as_vector()) {
                        auto live_start = -1, live_end = Target::NUM_MAU_STAGES();
                        auto container_field_json = field_json->as_map();
                        if (container_field_json->count("name")) {
                            if ((*container_field_json)["name"] != field_name) continue;
                        } else {
                            continue;
                        }
                        if (container_field_json->count("live_start")) {
                            auto live_start_json = (*container_field_json)["live_start"];
                            if (auto n = live_start_json->as_number()) live_start = n->val;
                        }
                        if (container_field_json->count("live_end")) {
                            auto live_end_json = (*container_field_json)["live_end"];
                            if (auto n = live_end_json->as_number()) live_end = n->val;
                        }
                        if (i >= live_start && i <= live_end) {
                            add_phv_record_items(live_start, "live_start");
                            add_phv_record_items(live_end, "live_end");
                            phv_record["mutually_exclusive_with"] = json::vector();
                            if (container_field_json->count("mutually_exclusive_with")) {
                                auto mutex_json =
                                    (*container_field_json)["mutually_exclusive_with"];
                                if (json::vector *mutex_json_vec = mutex_json->as_vector())
                                    phv_record["mutually_exclusive_with"] =
                                        std::move(*mutex_json_vec);
                            }
                            field_added = true;
                            // Skip duplicates
                            if (!std::any_of(phv_records.begin(), phv_records.end(),
                                             [&phv_record](std::unique_ptr<json::obj> &r) {
                                                 return *r == phv_record;
                                             }))
                                phv_records.push_back(phv_record.clone());
                        }
                    }
                    if (!field_added) {
                        auto live_start = -1, live_end = Target::NUM_MAU_STAGES();
                        add_phv_record_items(live_start, "live_start");
                        add_phv_record_items(live_end, "live_end");
                        phv_record["mutually_exclusive_with"] = json::vector();
                        phv_records.push_back(phv_record.clone());
                    }
                } else {
                    phv_records.push_back(std::move(phv_record));
                }
            }
            phv_container["word_bit_width"] = phv_container_size;
            // Ghost phv's are considered as ingress phv's
            if (phv_records.size() > 0) {
                if ((gress == INGRESS) || (gress == GHOST)) {
                    phv_alloc_stage_ingress.push_back(std::move(phv_container));
                } else if (gress == EGRESS) {
                    phv_alloc_stage_egress.push_back(std::move(phv_container));
                }
            }
        }
        phv_alloc_stage["stage_number"] = i;
        phv_alloc.push_back(std::move(phv_alloc_stage));
    }
    // FIXME: Fix json clone method to do above loops more efficiently
    // for (int i = 0; i < Target::NUM_MAU_STAGES(); i++) {
    //     phv_alloc_stage["stage_number"] = i;
    //     phv_alloc.push_back(std::move(phv_alloc_stage.clone())); }
}

#include "jbay/phv.cpp"    // NOLINT(build/include)
#include "tofino/phv.cpp"  // NOLINT(build/include)
