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

#include "deparser.h"

#include "backends/tofino/bf-asm/config.h"
#include "backends/tofino/bf-asm/target.h"
#include "constants.h"
#include "lib/range.h"
#include "misc.h"
#include "parser-tofino-jbay.h"
#include "phv.h"
#include "top_level.h"
#include "ubits.h"

unsigned Deparser::unique_field_list_handle;
Deparser Deparser::singleton_object;

Deparser::Deparser() : Section("deparser") {}
Deparser::~Deparser() {}

struct Deparser::FDEntry {
    struct Base {
        virtual ~Base() {}
        virtual void check(bitvec &phv_use) = 0;
        virtual unsigned encode() = 0;
        virtual unsigned size() = 0;  // size in bytes;
        virtual void dbprint(std::ostream &) const = 0;
        template <class T>
        bool is() const {
            return dynamic_cast<const T *>(this) != nullptr;
        }
        template <class T>
        T *to() {
            return dynamic_cast<T *>(this);
        }
        friend std::ostream &operator<<(std::ostream &out, const Base &b) {
            b.dbprint(out);
            return out;
        }
    };
    struct Phv : Base {
        ::Phv::Ref val;
        Phv(gress_t g, const value_t &v) : val(g, DEPARSER_STAGE, v) {}
        void check(bitvec &phv_use) override {
            if (val.check()) {
                phv_use[val->reg.uid] = 1;
                if (val->lo != 0 || val->hi != val->reg.size - 1)
                    error(val.lineno,
                          "Can only output full phv registers, not slices, "
                          "in deparser");
            }
        }
        unsigned encode() override { return val->reg.deparser_id(); }
        unsigned size() override { return val->reg.size / 8; }
        const ::Phv::Register *reg() { return &val->reg; }
        void dbprint(std::ostream &out) const override { out << val.desc(); }
    };
    struct Checksum : Base {
        gress_t gress;
        int unit;
        Checksum(gress_t gr, const value_t &v) : gress(gr) {
            if (CHECKTYPE(v, tINT)) {
                if ((unit = v.i) < 0 || v.i >= Target::DEPARSER_CHECKSUM_UNITS())
                    error(v.lineno, "Invalid deparser checksum unit %" PRId64 "", v.i);
            }
        }
        void check(bitvec &phv_use) override {}
        template <class TARGET>
        unsigned encode();
        unsigned encode() override;
        unsigned size() override { return 2; }
        void dbprint(std::ostream &out) const override { out << gress << " checksum " << unit; }
    };
    struct Constant : Base {
        int lineno;
        gress_t gress;
        int val;
        Constant(gress_t g, const value_t &v) : gress(g), val(v.i) {
            lineno = v.lineno;
            if (v.i < 0 || v.i >> 8)
                error(lineno,
                      "Invalid deparser constant %" PRId64 ", valid constant range is 0-255", v.i);
            bool ok = Deparser::add_constant(gress, val);
            if (!ok) error(lineno, "Ran out of deparser constants");
        }
        void check(bitvec &phv_use) override {}
        template <class TARGET>
        unsigned encode();
        unsigned encode() override;
        unsigned size() override { return 1; }
        void dbprint(std::ostream &out) const override { out << val; }
    };
    struct Clot : Base {
        int lineno;
        gress_t gress;
        std::string tag;
        int length = -1;
        std::map<unsigned, ::Phv::Ref> phv_replace;
        std::map<unsigned, Checksum> csum_replace;
        Clot(gress_t gr, const value_t &tag, const value_t &data, ordered_set<::Phv::Ref> &pov)
            : lineno(tag.lineno), gress(gr) {
            if (CHECKTYPE2(tag, tINT, tSTR)) {
                if (tag.type == tSTR)
                    this->tag = tag.s;
                else
                    this->tag = std::to_string(tag.i);
            }
            if (data.type == tMAP) {
                for (auto &kv : data.map) {
                    if (kv.key == "pov") {
                        pov.emplace(gress, DEPARSER_STAGE, kv.value);
                    } else if (kv.key == "max_length" || kv.key == "length") {
                        if (length >= 0) error(kv.value.lineno, "Duplicate length");
                        if (CHECKTYPE(kv.value, tINT) && ((length = kv.value.i) < 0 || length > 64))
                            error(kv.value.lineno, "Invalid clot length");
                    } else if (kv.key.type == tINT) {
                        if (phv_replace.count(kv.key.i) || csum_replace.count(kv.key.i))
                            error(kv.value.lineno, "Duplicate value at offset %" PRId64 "",
                                  kv.key.i);
                        if (kv.value.type == tCMD && kv.value.vec.size == 2 &&
                            kv.value == "full_checksum")
                            csum_replace.emplace(kv.key.i, Checksum(gress, kv.value.vec[1]));
                        else
                            phv_replace.emplace(kv.key.i,
                                                ::Phv::Ref(gress, DEPARSER_STAGE, kv.value));
                    } else {
                        error(kv.value.lineno, "Unknown key for clot: %s", value_desc(kv.key));
                    }
                }
            } else {
                pov.emplace(gress, DEPARSER_STAGE, data);
            }
            if (pov.size() > Target::DEPARSER_MAX_POV_PER_USE())
                error(data.lineno, "Too many POV bits for CLOT");
        }
        void check(bitvec &phv_use) override {
            if (length < 0) length = Parser::clot_maxlen(gress, tag);
            if (length < 0) error(lineno, "No length for clot %s", tag.c_str());
            if (Parser::clot_tag(gress, tag) < 0) error(lineno, "No tag for clot %s", tag.c_str());
            unsigned next = 0;
            ::Phv::Ref *prev = nullptr;
            for (auto &r : phv_replace) {
                if (r.first < next) {
                    error(r.second.lineno, "Overlapping phvs in clot");
                    error(prev->lineno, "%s and %s", prev->name(), r.second.name());
                }
                if (r.second.check()) {
                    phv_use[r.second->reg.uid] = 1;
                    if (r.second->lo != 0 || r.second->hi != r.second->reg.size - 1)
                        error(r.second.lineno,
                              "Can only output full phv registers, not slices,"
                              " in deparser");
                    next = r.first + r.second->reg.size / 8U;
                    prev = &r.second;
                }
            }
        }
        unsigned size() override { return length; }
        unsigned encode() override {
            BUG();
            return -1;
        }
        void dbprint(std::ostream &out) const override {
            out << "clot " << tag;
            if (length > 0) out << " [len " << length << "]";
        }
    };

    int lineno;
    std::unique_ptr<Base> what;
    ordered_set<::Phv::Ref> pov;
    FDEntry(gress_t gress, const value_t &v, const value_t &p) {
        lineno = v.lineno;
        if (v.type == tCMD && v.vec.size == 2 && v == "clot") {
            what.reset(new Clot(gress, v.vec[1], p, pov));
        } else if (v.type == tCMD && v.vec.size == 2 && v == "full_checksum") {
            what.reset(new Checksum(gress, v.vec[1]));
            pov.emplace(gress, DEPARSER_STAGE, p);
        } else if (v.type == tINT) {
            what.reset(new Constant(gress, v));
            pov.emplace(gress, DEPARSER_STAGE, p);
        } else {
            what.reset(new Phv(gress, v));
            pov.emplace(gress, DEPARSER_STAGE, p);
        }
    }
    void check(bitvec &phv_use) { what->check(phv_use); }
};

struct Deparser::Intrinsic::Type {
    target_t target;
    gress_t gress;
    std::string name;
    int max;
    static std::map<std::string, Type *> all[TARGET_INDEX_LIMIT][2];

 protected:
    Type(target_t t, gress_t gr, const char *n, int m) : target(t), gress(gr), name(n), max(m) {
        BUG_CHECK(!all[t][gr].count(name));
        all[target][gress][name] = this;
    }
    ~Type() { all[target][gress].erase(name); }

 public:
#define VIRTUAL_TARGET_METHODS(TARGET)                                            \
    virtual void setregs(Target::TARGET::deparser_regs &regs, Deparser &deparser, \
                         Intrinsic &vals) {                                       \
        BUG_CHECK(!"target mismatch");                                            \
    }
    FOR_ALL_REGISTER_SETS(VIRTUAL_TARGET_METHODS)
#undef VIRTUAL_TARGET_METHODS
};

#define DEPARSER_INTRINSIC(TARGET, GR, NAME, MAX)                                                  \
    static struct TARGET##INTRIN##GR##NAME : public Deparser::Intrinsic::Type {                    \
        TARGET##INTRIN##GR##NAME()                                                                 \
            : Deparser::Intrinsic::Type(Target::TARGET::tag, GR, #NAME, MAX) {}                    \
        void setregs(Target::TARGET::deparser_regs &, Deparser &, Deparser::Intrinsic &) override; \
    } TARGET##INTRIN##GR##NAME##_singleton;                                                        \
    void TARGET##INTRIN##GR##NAME::setregs(Target::TARGET::deparser_regs &regs,                    \
                                           Deparser &deparser, Deparser::Intrinsic &intrin)

std::map<std::string, Deparser::Intrinsic::Type *>
    Deparser::Intrinsic::Type::all[TARGET_INDEX_LIMIT][2];

Deparser::Digest::Digest(Deparser::Digest::Type *t, int l, VECTOR(pair_t) & data) {
    type = t;
    lineno = l;
    for (auto &l : data) {
        if (l.key == "select") {
            if (l.value.type == tMAP && l.value.map.size == 1) {
                select = Val(t->gress, l.value.map[0].key, l.value.map[0].value);
            } else {
                select = Val(t->gress, l.value);
            }
        } else if (t->can_shift && l.key == "shift") {
            if (CHECKTYPE(l.value, tINT)) shift = l.value.i;
        } else if (l.key == "context_json") {
            if (CHECKTYPE(l.value, tMAP)) context_json = toJson(l.value.map);
        } else if (!CHECKTYPE(l.key, tINT)) {
            continue;
        } else if (l.key.i < 0 || l.key.i >= t->count) {
            error(l.key.lineno, "%s index %" PRId64 " out of range", t->name.c_str(), l.key.i);
        } else if (l.value.type != tVEC) {
            layout[l.key.i].emplace_back(t->gress, DEPARSER_STAGE, l.value);
        } else {
            // TODO : Need an empty layout entry if no values are present to
            // set the config registers correctly
            layout.emplace(l.key.i, std::vector<Phv::Ref>());
            for (auto &v : l.value.vec) layout[l.key.i].emplace_back(t->gress, DEPARSER_STAGE, v);
        }
    }
    if (!select && t->name != "pktgen") error(lineno, "No select key in %s spec", t->name.c_str());
}

#define DEPARSER_DIGEST(TARGET, GRESS, NAME, CNT, ...)                                          \
    static struct TARGET##GRESS##NAME##Digest : public Deparser::Digest::Type {                 \
        TARGET##GRESS##NAME##Digest()                                                           \
            : Deparser::Digest::Type(Target::TARGET::tag, GRESS, #NAME, CNT) {                  \
            __VA_ARGS__                                                                         \
        }                                                                                       \
        void setregs(Target::TARGET::deparser_regs &, Deparser &, Deparser::Digest &) override; \
    } TARGET##GRESS##NAME##Digest##_singleton;                                                  \
    void TARGET##GRESS##NAME##Digest::setregs(Target::TARGET::deparser_regs &regs,              \
                                              Deparser &deparser, Deparser::Digest &data)

std::map<std::string, Deparser::Digest::Type *> Deparser::Digest::Type::all[TARGET_INDEX_LIMIT][2];

void Deparser::start(int lineno, VECTOR(value_t) args) {
    if (args.size == 0) {
        this->lineno[INGRESS] = this->lineno[EGRESS] = lineno;
        return;
    }
    if (args.size != 1 || (args[0] != "ingress" && args[0] != "egress"))
        error(lineno, "deparser must specify ingress or egress");
    gress_t gress = args[0] == "egress" ? EGRESS : INGRESS;
    if (!this->lineno[gress]) this->lineno[gress] = lineno;
}

void Deparser::input(VECTOR(value_t) args, value_t data) {
    if (!CHECKTYPE(data, tMAP)) return;
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        if (args.size > 0) {
            if (args[0] == "ingress" && gress != INGRESS) continue;
            if (args[0] == "egress" && gress != EGRESS) continue;
        } else if (error_count > 0) {
            break;
        }
        for (auto &kv : MapIterChecked(data.map, true)) {
            if (kv.key == "dictionary") {
                if (kv.value.type == tVEC && kv.value.vec.size == 0) continue;
                collapse_list_of_maps(kv.value);
                if (!CHECKTYPE(kv.value, tMAP)) continue;
                for (auto &ent : kv.value.map)
                    dictionary[gress].emplace_back(gress, ent.key, ent.value);
            } else if (kv.key == "pov") {
                if (kv.value.type != tVEC) {
                    /// The check for correct type is done in Phv::Ref constructor
                    pov_order[gress].emplace_back(gress, DEPARSER_STAGE, kv.value);
                } else {
                    for (auto &ent : kv.value.vec)
                        pov_order[gress].emplace_back(gress, DEPARSER_STAGE, ent);
                }
            } else if (kv.key == "partial_checksum") {
                if (kv.key.type != tCMD || kv.key.vec.size != 2 || kv.key[1].type != tINT ||
                    kv.key[1].i < 0 || kv.key[1].i >= Target::DEPARSER_CHECKSUM_UNITS()) {
                    error(kv.key.lineno, "Invalid deparser checksum unit number");
                } else if (CHECKTYPE2(kv.value, tVEC, tMAP)) {
                    collapse_list_of_maps(kv.value);
                    int unit = kv.key[1].i;
                    if (unit < 0) error(kv.key.lineno, "Invalid checksum unit %d", unit);
                    for (auto &ent : kv.value.map) {
                        checksum_entries[gress][unit].emplace_back(gress, ent.key, ent.value);
                    }
                }
            } else if (kv.key == "full_checksum") {
                if (kv.key.type != tCMD || kv.key.vec.size != 2 || kv.key[1].type != tINT ||
                    kv.key[1].i < 0 || kv.key[1].i >= Target::DEPARSER_CHECKSUM_UNITS()) {
                    error(kv.key.lineno, "Invalid deparser checksum unit number");
                } else if (CHECKTYPE2(kv.value, tVEC, tMAP)) {
                    collapse_list_of_maps(kv.value);
                    int unit = kv.key[1].i;
                    if (unit < 0) error(kv.key.lineno, "Invalid checksum unit %d", unit);
                    for (auto &ent : kv.value.map) {
                        if (ent.key == "partial_checksum") {
                            full_checksum_unit[gress][unit].entries[ent.key[1].i] =
                                checksum_entries[gress][ent.key[1].i];
                            collapse_list_of_maps(ent.value);
                            for (auto &a : ent.value.map) {
                                if (a.key == "pov") {
                                    full_checksum_unit[gress][unit].pov[ent.key[1].i] =
                                        ::Phv::Ref(gress, DEPARSER_STAGE, a.value);
                                } else if (a.key == "invert") {
                                    full_checksum_unit[gress][unit].checksum_unit_invert.insert(
                                        ent.key[1].i);
                                }
                            }
                        } else if (ent.key == "clot") {
                            collapse_list_of_maps(ent.value);
                            for (auto &a : ent.value.map) {
                                if (a.key == "pov") {
                                    full_checksum_unit[gress][unit].clot_entries.emplace_back(
                                        gress, ent.key[1].i, a.value);
                                } else if (a.key == "invert") {
                                    full_checksum_unit[gress][unit].clot_tag_invert.insert(
                                        a.value.i);
                                }
                            }
                        } else if (ent.key == "zeros_as_ones") {
                            full_checksum_unit[gress][unit].zeros_as_ones_en = ent.value.i;
                        }
                    }
                }
            } else if (auto *itype = ::get(Intrinsic::Type::all[Target::register_set()][gress],
                                           value_desc(&kv.key))) {
                intrinsics.emplace_back(itype, kv.key.lineno);
                auto &intrin = intrinsics.back();
                collapse_list_of_maps(kv.value);
                if (kv.value.type == tVEC) {
                    for (auto &val : kv.value.vec) intrin.vals.emplace_back(gress, val);
                } else if (kv.value.type == tMAP) {
                    for (auto &el : kv.value.map) intrin.vals.emplace_back(gress, el.key, el.value);
                } else {
                    intrin.vals.emplace_back(gress, kv.value);
                }
            } else if (auto *digest = ::get(Digest::Type::all[Target::register_set()][gress],
                                            value_desc(&kv.key))) {
                if (CHECKTYPE(kv.value, tMAP))
                    digests.emplace_back(digest, kv.value.lineno, kv.value.map);
            } else {
                error(kv.key.lineno, "Unknown deparser tag %s", value_desc(&kv.key));
            }
        }
    }
}

template <class ENTRIES>
static void write_checksum_entry(ENTRIES &entry, unsigned mask, int swap, int id,
                                 const char *name = "entry") {
    BUG_CHECK(swap == 0 || swap == 1);
    BUG_CHECK(mask == 0 || mask & 3);
    if (entry.modified()) error(1, "%s appears multiple times in checksum %d", name, id);
    entry.swap = swap;
    // CSR: The order of operation: data is swapped or not and then zeroed or not
    if (swap) mask = (mask & 0x2) >> 1 | (mask & 0x1) << 1;
    switch (mask) {
        case 0:
            entry.zero_m_s_b = 1;
            entry.zero_l_s_b = 1;
            break;
        case 1:
            entry.zero_m_s_b = 1;
            entry.zero_l_s_b = 0;
            break;
        case 2:
            entry.zero_m_s_b = 0;
            entry.zero_l_s_b = 1;
            break;
        case 3:
            entry.zero_m_s_b = 0;
            entry.zero_l_s_b = 0;
            break;
        default:
            break;
    }
}

// Used for field dictionary logging and deparser resoureces.
// Using fd entry and pov, a json::map is filled with appropriate field names
void write_field_name_in_json(const Phv::Register *phv, const Phv::Register *pov, int povBit,
                              json::map &chunk_byte, json::map &fd_entry_chunk_byte, int stageno,
                              gress_t gress) {
    auto povName_ = Phv::get_pov_name(pov->mau_id(), povBit);
    std::string povName = povName_;
    std::string headerName;
    size_t pos = 0;
    if ((pos = povName.find("$valid")) != std::string::npos) {
        headerName = povName.substr(0, pos);
    }
    std::string fieldNames;
    auto allFields = Phv::aliases(phv, stageno);
    for (auto fieldName : allFields) {
        if (fieldName.find(headerName) != std::string::npos) fieldNames += (fieldName + ", ");
    }
    fd_entry_chunk_byte["phv_container"] = phv->uid;
    chunk_byte["PHV"] = phv->uid;
    chunk_byte["Field"] = fieldNames;
    return;
}

void write_pov_resources_in_json(ordered_map<const Phv::Register *, unsigned> &pov,
                                 json::map &pov_resources) {
    unsigned pov_size = 0;
    json::vector pov_bits;
    // ent will be tuple of (register ref, pov position start)
    for (auto const &ent : pov) {
        // Go through all the bits
        unsigned used_bits = 0;
        for (unsigned i = 0; i < ent.first->size; i++) {
            json::map pov_bit;
            std::string pov_name = Phv::get_pov_name(ent.first->uid, i);
            // Check if this POV bit is used
            if (pov_name.compare(" ") != 0) {
                pov_bit["pov_bit"] = ent.second + i;
                pov_bit["phv_container"] = ent.first->uid;
                pov_bit["phv_container_bit"] = i;
                pov_bit["pov_name"] = pov_name;
                pov_bits.push_back(std::move(pov_bit));
                used_bits++;
            }
        }
        if (pov_size < (ent.second + used_bits)) pov_size = ent.second + used_bits;
    }
    pov_resources["size"] = pov_size;
    pov_resources["pov_bits"] = std::move(pov_bits);
}

// Used for field dictionary logging. Using fd entry and pov, a json::map
// is filled with appropriate checksum or constant
void write_csum_const_in_json(int deparserPhvIdx, json::map &chunk_byte,
                              json::map &fd_entry_chunk_byte, gress_t gress) {
    if (options.target == Target::Tofino::tag) {
        if (deparserPhvIdx >= CHECKSUM_ENGINE_PHVID_TOFINO_LOW &&
            deparserPhvIdx <= CHECKSUM_ENGINE_PHVID_TOFINO_HIGH) {
            auto csum_id = deparserPhvIdx - CHECKSUM_ENGINE_PHVID_TOFINO_LOW -
                           (gress * CHECKSUM_ENGINE_PHVID_TOFINO_PER_GRESS);
            chunk_byte["Checksum"] = csum_id;
            fd_entry_chunk_byte["csum_engine"] = csum_id;
        }
    } else if (options.target == Target::JBay::tag) {
        if (deparserPhvIdx > CONSTANTS_PHVID_JBAY_LOW &&
            deparserPhvIdx < CONSTANTS_PHVID_JBAY_HIGH) {
            chunk_byte["Constant"] =
                Deparser::get_constant(gress, deparserPhvIdx - CONSTANTS_PHVID_JBAY_LOW);
            fd_entry_chunk_byte["phv_container"] = deparserPhvIdx;
        } else {
            auto csum_id = deparserPhvIdx - CONSTANTS_PHVID_JBAY_HIGH;
            chunk_byte["Checksum"] = csum_id;
            fd_entry_chunk_byte["csum_engine"] = csum_id;
        }
    }
    return;
}

/// Get JSON for deparser resources from digest of deparser table
/// @param tab_digest Digest for the deparser table, nullptr if the table does not exist
/// @return JSON node representation of the table for deparser resources
json::map deparser_table_digest_to_json(Deparser::Digest *tab_digest) {
    json::map dep_table;
    json::vector table_phv;

    // nullptr means the table is not used, create JSON node for empty table
    // and return it
    if (tab_digest == nullptr) {
        dep_table["nTables"] = 0;
        dep_table["maxBytes"] = 0;
        dep_table["table_phv"] = std::move(table_phv);
        return dep_table;
    }

    unsigned int max_bytes = 0;
    // Prepare tables of the deparser table type
    for (auto &set : tab_digest->layout) {
        json::map table;
        table["table_id"] = set.first;
        // TODO: field_list_name?
        json::vector bytes;
        unsigned byte_n = 0;
        for (auto &reg : set.second) {
            json::map byte;
            byte["byte_number"] = byte_n++;
            byte["phv_container"] = reg->reg.uid;
            bytes.push_back(std::move(byte));
        }
        if (byte_n > max_bytes) max_bytes = byte_n;
        table["bytes"] = std::move(bytes);
        table_phv.push_back(std::move(table));
    }
    dep_table["nTables"] = tab_digest->layout.size();
    dep_table["maxBytes"] = max_bytes;
    dep_table["index_phv"] = tab_digest->select->reg.uid;
    dep_table["table_phv"] = std::move(table_phv);
    // Now we have a digest
    return dep_table;
}

/// Create resources_deparser.json with the deparser node
/// for resources.json
/// @param fde_entries_i JSON vector of field dictionary entries from Ingress
/// @param fde_entries_e JSON vector of field dictionary entries from Egress
void Deparser::report_resources_deparser_json(json::vector &fde_entries_i,
                                              json::vector &fde_entries_e) {
    json::map resources_deparser_ingress;
    json::map resources_deparser_egress;
    // Set gress property
    resources_deparser_ingress["gress"] = "ingress";
    resources_deparser_egress["gress"] = "egress";
    // Fill out POV resource information for ingress
    json::map pov_resources;
    write_pov_resources_in_json(pov[INGRESS], pov_resources);
    resources_deparser_ingress["pov"] = std::move(pov_resources);
    // Fill out POV resoure information for egress
    write_pov_resources_in_json(pov[EGRESS], pov_resources);
    resources_deparser_egress["pov"] = std::move(pov_resources);
    // Fill out field dictionaries
    unsigned n_fde_entries = Target::DEPARSER_MAX_FD_ENTRIES();
    resources_deparser_ingress["nFdeEntries"] = n_fde_entries;
    resources_deparser_ingress["fde_entries"] = std::move(fde_entries_i);
    resources_deparser_egress["nFdeEntries"] = n_fde_entries;
    resources_deparser_egress["fde_entries"] = std::move(fde_entries_e);
    // Fill deparser tables
    Digest *learning_table[2] = {nullptr, nullptr};
    Digest *resubmit_table[2] = {nullptr, nullptr};
    Digest *mirror_table[2] = {nullptr, nullptr};
    for (auto &digest : digests) {
        // Check if this is egress/ingress
        if (digest.type->gress != INGRESS && digest.type->gress != EGRESS) continue;
        if (digest.type->name == "learning")
            learning_table[digest.type->gress] = &digest;
        else if (digest.type->name == "resubmit" ||
                 digest.type->name == "resubmit_preserving_field_list")
            resubmit_table[digest.type->gress] = &digest;
        else if (digest.type->name == "mirror")
            mirror_table[digest.type->gress] = &digest;
    }
    resources_deparser_ingress["mirror_table"] =
        deparser_table_digest_to_json(mirror_table[INGRESS]);
    resources_deparser_egress["mirror_table"] = deparser_table_digest_to_json(mirror_table[EGRESS]);
    resources_deparser_ingress["resubmit_table"] =
        deparser_table_digest_to_json(resubmit_table[INGRESS]);
    resources_deparser_egress["resubmit_table"] =
        deparser_table_digest_to_json(resubmit_table[EGRESS]);
    resources_deparser_ingress["learning_table"] =
        deparser_table_digest_to_json(learning_table[INGRESS]);
    resources_deparser_egress["learning_table"] =
        deparser_table_digest_to_json(learning_table[EGRESS]);

    // Create the main deparser resources node
    json::vector resources_deparser;
    resources_deparser.push_back(std::move(resources_deparser_ingress));
    resources_deparser.push_back(std::move(resources_deparser_egress));
    // Dump resources to file
    auto deparser_json_dump = open_output("logs/resources_deparser.json");
    *deparser_json_dump << &resources_deparser;
}

#include "tofino/deparser.cpp"  // NOLINT(build/include)
#if HAVE_JBAY
#include "jbay/deparser.cpp"  // NOLINT(build/include)
#endif                        /* HAVE_JBAY */

std::vector<Deparser::ChecksumVal> Deparser::merge_csum_entries(
    const std::vector<Deparser::ChecksumVal> &entries, int id) {
    std::vector<Deparser::ChecksumVal> rv;
    ordered_map<std::string, Deparser::ChecksumVal> merged_entries;

    for (auto &entry : entries) {
        if (entry.is_clot()) {
            rv.push_back(entry);
            continue;
        }
        auto name = entry.val.name();
        int hi = entry.val.hibit();
        int lo = entry.val.lobit();
        bool is_hi = hi >= 16;
        bool is_lo = lo < 16;

        if (!merged_entries.count(name)) {
            auto reg = Phv::reg(name);
            auto new_entry(entry);
            if (lo != 0 && hi != reg->size - 1) {
                new_entry.val = Phv::Ref(*reg, entry.val.gress(), 0, reg->size - 1);
            }
            merged_entries.emplace(name, new_entry);
        } else {
            auto &rv_entry = merged_entries[name];
            if (rv_entry.mask & entry.mask)
                error(entry.lineno, "bytes within %s appear multiple times in checksum %d", name,
                      id);
            if (is_hi) {
                if ((rv_entry.mask & 0xc) && (rv_entry.swap & 2) != (entry.swap & 2))
                    error(entry.lineno, "incompatible swap values for %s in checksum %d", name, id);
                rv_entry.mask |= entry.mask & 0xc;
                rv_entry.swap |= entry.swap & 2;
            }
            if (is_lo) {
                if ((rv_entry.mask & 0x3) && (rv_entry.swap & 1) != (entry.swap & 1))
                    error(entry.lineno, "incompatible swap values for %s in checksum %d", name, id);
                rv_entry.mask |= entry.mask & 0x3;
                rv_entry.swap |= entry.swap & 1;
            }
        }
    }

    for (auto &[_, entry] : merged_entries) rv.push_back(entry);

    return rv;
}

/* The following uses of specialized templates must be after the specialization... */
void Deparser::process() {
    bitvec pov_use[2];
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        for (auto &ent : pov_order[gress])
            if (ent.check()) {
                pov_use[gress][ent->reg.uid] = 1;
                phv_use[gress][ent->reg.uid] = 1;
            }
        for (auto &ent : dictionary[gress]) {
            ent.check(phv_use[gress]);
            for (auto &pov : ent.pov) {
                if (!pov.check()) continue;
                phv_use[gress][pov->reg.uid] = 1;
                if (pov->lo != pov->hi) error(pov.lineno, "POV bits should be single bits");
                if (!pov_use[gress][pov->reg.uid]) {
                    pov_order[gress].emplace_back(pov->reg, gress);
                    pov_use[gress][pov->reg.uid] = 1;
                }
            }
        }
        for (int i = 0; i < MAX_DEPARSER_CHECKSUM_UNITS; i++)
            for (auto &ent : full_checksum_unit[gress][i].entries) {
                for (auto entry : ent.second) {
                    if (!entry.check()) error(entry.lineno, "Invalid checksum entry");
                }
                ent.second = merge_csum_entries(ent.second, i);
            }
    }
    for (auto &intrin : intrinsics) {
        for (auto &el : intrin.vals) {
            if (el.check()) phv_use[intrin.type->gress][el->reg.uid] = 1;
            for (auto &pov : el.pov) {
                if (pov.check()) {
                    phv_use[intrin.type->gress][pov->reg.uid] = 1;
                    if (pov->lo != pov->hi) error(pov.lineno, "POV bits should be single bits");
                    if (!pov_use[intrin.type->gress][pov->reg.uid]) {
                        pov_order[intrin.type->gress].emplace_back(pov->reg, intrin.type->gress);
                        pov_use[intrin.type->gress][pov->reg.uid] = 1;
                    }
                }
            }
        }
        if (intrin.vals.size() > (size_t)intrin.type->max)
            error(intrin.lineno, "Too many values for %s", intrin.type->name.c_str());
    }
    if (phv_use[INGRESS].intersects(phv_use[EGRESS]))
        error(lineno[INGRESS], "Registers used in both ingress and egress in deparser: %s",
              Phv::db_regset(phv_use[INGRESS] & phv_use[EGRESS]).c_str());
    for (auto &digest : digests) {
        if (digest.select.check()) {
            phv_use[digest.type->gress][digest.select->reg.uid] = 1;
            if (digest.select->lo > 0 && !digest.type->can_shift)
                error(digest.select.lineno, "%s digest selector must be in bottom bits of phv",
                      digest.type->name.c_str());
        }
        for (auto &pov : digest.select.pov) {
            if (pov.check()) {
                phv_use[digest.type->gress][pov->reg.uid] = 1;
                if (pov->lo != pov->hi) error(pov.lineno, "POV bits should be single bits");
                if (!pov_use[digest.type->gress][pov->reg.uid]) {
                    pov_order[digest.type->gress].emplace_back(pov->reg, digest.type->gress);
                    pov_use[digest.type->gress][pov->reg.uid] = 1;
                }
            }
        }
        for (auto &set : digest.layout)
            for (auto &reg : set.second)
                if (reg.check()) phv_use[digest.type->gress][reg->reg.uid] = 1;
    }
    SWITCH_FOREACH_REGISTER_SET(Target::register_set(), TARGET *t = nullptr;
                                // process(t);
                                process((TARGET *)nullptr);)

    if (options.match_compiler || 1) { /* FIXME -- need proper liveness analysis */
        Phv::setuse(INGRESS, phv_use[INGRESS]);
        Phv::setuse(EGRESS, phv_use[EGRESS]);
    }
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        int pov_byte = 0, pov_size = 0;
        for (auto &ent : pov_order[gress])
            if (pov[gress].count(&ent->reg) == 0) {
                pov[gress][&ent->reg] = pov_size;
                pov_size += ent->reg.size;
            }
        if (pov_size > 8 * Target::DEPARSER_MAX_POV_BYTES())
            error(lineno[gress], "Ran out of space in POV in deparser");
    }
}

/* The following uses of specialized templates must be after the specialization... */
void Deparser::output(json::map &map) {
    SWITCH_FOREACH_TARGET(options.target, auto *regs = new TARGET::deparser_regs;
                          declare_registers(regs); write_config(*regs);
                          gen_learn_quanta(*regs, map["learn_quanta"]); return;)
    error(__LINE__, "Unsupported target %d", options.target);
}

/* this is a bit complicated since the output from compiler digest is as follows:
 context_json:
  0: [ [ ipv4.ihl, 0, 4, 0], [ ipv4.protocol, 0, 8, 1], [ ipv4.srcAddr, 0, 32, 2], [
 ethernet.srcAddr, 0, 48, 6], [ ethernet.dstAddr, 0, 48, 12], [ ipv4.fragOffset, 0, 13, 18     ], [
 ipv4.identification, 0, 16, 20], [ routing_metadata.learn_meta_1, 0, 20, 22], [
 routing_metadata.learn_meta_4, 0, 10, 26] ] 1: [ [ ipv4.ihl, 0, 4, 0], [ ipv4.identification, 0,
 16, 1], [ ipv4.protocol, 0, 8, 3], [ ipv4.srcAddr, 0, 32, 4], [ ethernet.srcAddr, 0, 48, 8], [
 ethernet.dstAddr, 0, 48,      14], [ ipv4.fragOffset, 0, 13, 20], [ routing_metadata.learn_meta_2,
 0, 24, 22], [ routing_metadata.learn_meta_3, 0, 25, 26] ] name: [ learn_1, learn_2 ]
*/
template <class REGS>
void Deparser::gen_learn_quanta(REGS &regs, json::vector &learn_quanta) {
    for (auto &digest : digests) {
        if (digest.type->name != "learning") continue;
        BUG_CHECK(digest.context_json);
        auto namevec = (*(digest.context_json))["name"];
        auto &names = *(namevec->as_vector());
        auto digentry = digest.context_json->begin();
        // Iterate on names. for each name, get the corresponding digest entry and fill in
        for (auto &tname : names) {
            BUG_CHECK(digentry != digest.context_json->end());
            json::map quanta;
            quanta["name"] = (*tname).c_str();
            quanta["lq_cfg_type"] = digentry->first->as_number()->val;
            quanta["handle"] = next_handle();
            auto *digfields = digentry->second->as_vector();
            if (digfields) {
                auto &digfields_vec = *digfields;
                json::vector &fields = quanta["fields"];
                for (auto &tup : digfields_vec) {
                    auto &one = *(tup->as_vector());
                    BUG_CHECK(one.size() == 5);
                    json::map anon;
                    anon["field_name"] = (*(one[0])).clone();
                    anon["start_byte"] = (*(one[1])).clone();
                    anon["field_width"] = (*(one[2])).clone();
                    anon["start_bit"] = (*(one[3])).clone();
                    anon["phv_offset"] = (*(one[4])).clone();
                    fields.push_back(std::move(anon));
                }
            }
            digentry++;
            learn_quanta.push_back(std::move(quanta));
        }
    }
}

unsigned Deparser::FDEntry::Checksum::encode() {
    SWITCH_FOREACH_TARGET(options.target, return encode<TARGET::register_type>(););
    return -1;
}

unsigned Deparser::FDEntry::Constant::encode() {
    SWITCH_FOREACH_TARGET(options.target, return encode<TARGET::register_type>(););
    return -1;
}

void Deparser::gtest_clear() {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < MAX_DEPARSER_CHECKSUM_UNITS; j++) checksum_entries[i][j].clear();
        dictionary[i].clear();
        pov_order[i].clear();
        pov[i].clear();
        phv_use[i].clear();
        constants[i].clear();
    }
    intrinsics.clear();
    digests.clear();
}
