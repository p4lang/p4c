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

#ifndef DEPARSER_H_
#define DEPARSER_H_

#include <vector>

#include <boost/optional.hpp>

#include "constants.h"
#include "lib/bitops.h"
#include "lib/ordered_set.h"
#include "phv.h"
#include "sections.h"

enum {
    // limits over all targets
    MAX_DEPARSER_CHECKSUM_UNITS = 8,
    DEPARSER_STAGE = INT_MAX,  // greater than the number of stages
};

/**
 * \ingroup parde
 */
class Deparser : public Section {
    static Deparser singleton_object;

 public:
    struct Val {
        /* a phv or clot reference with optional associated POV phv reference */
        Phv::Ref val;
        int tag = -1;
        ordered_set<Phv::Ref> pov;
        const int &lineno = val.lineno;
        Val() {}
        virtual ~Val() {}
        Val(gress_t gr, const value_t &v) : val(gr, DEPARSER_STAGE, v) {}
        Val(gress_t gr, const value_t &v, const value_t &p) : val(gr, DEPARSER_STAGE, v) {
            pov.emplace(gr, DEPARSER_STAGE, p);
        }
        Val(gress_t gr, int tag, const value_t &p) : tag(tag) {
            pov.emplace(gr, DEPARSER_STAGE, p);
        }
        Val &operator=(const Val &a) {
            val = a.val;
            tag = a.tag;
            pov = a.pov;
            return *this;
        }
        explicit operator bool() const { return is_phv() || is_clot(); }
        Phv::Slice operator*() const { return *val; }
        Phv::Slice operator->() const { return *val; }
        bool is_phv() const { return bool(val); }
        bool is_clot() const { return tag >= 0; }
        virtual bool check() const {
            if (is_phv() && is_clot()) {
                error(lineno, "Reference cannot be phv and clot at the same time");
                return false;
            }
            if (is_phv()) {
                return val.check();
            } else if (is_clot()) {
                if (pov.empty()) {
                    error(lineno, "Clot requires a pov bit");
                    return false;
                }
            } else {
                error(lineno, "Unknown val");
                return false;
            }
            return true;
        }
    };

    struct ChecksumVal : public Val {
        int mask = 0;
        int swap = 0;
        ChecksumVal(gress_t gr, const value_t &v, const value_t &m) : Val(gr, v) {
            if ((val->lo % 8 != 0) || (val->hi % 8 != 7))
                error(lineno, "Can only do checksums on byte-aligned container slices");
            mask = ((1 << (val->hi + 1) / 8) - 1) ^ ((1 << val->lo / 8) - 1);

            if (CHECKTYPE(m, tMAP)) {
                for (auto &kv : m.map) {
                    if (kv.key == "pov") {
                        if (!pov.empty()) error(kv.value.lineno, "Duplicate POV");
                        pov.emplace_back(gr, DEPARSER_STAGE, kv.value);
                    } else if (kv.key == "swap" && CHECKTYPE(kv.value, tINT)) {
                        swap = kv.value.i;
                    } else {
                        error(m.lineno, "Unknown key for checksum: %s", value_desc(kv.key));
                    }
                }
            }
        }
        ChecksumVal(gress_t gr, int tag, const value_t &p) : Val(gr, tag, p) {}
        ChecksumVal &operator=(const ChecksumVal &a) {
            Val::operator=(a);
            mask = a.mask;
            swap = a.swap;
            return *this;
        }
        ChecksumVal() : Val() {}

        bool check() const override {
            if (is_phv()) {
                if (mask == 0) error(lineno, "mask is 0 for phv checkum value?");
                if (swap < 0 || swap > 3) error(lineno, "Invalid swap for phv checksum");
            }
            return Val::check();
        }
    };

    struct FullChecksumUnit {
        std::map<int, std::vector<ChecksumVal>> entries;
        std::map<int, Phv::Ref> pov;
        std::set<int> checksum_unit_invert;
        std::set<int> clot_tag_invert;
        std::vector<ChecksumVal> clot_entries;
        bool zeros_as_ones_en = false;
    };

    struct FDEntry;
    std::vector<ChecksumVal> checksum_entries[2][MAX_DEPARSER_CHECKSUM_UNITS];
    FullChecksumUnit full_checksum_unit[2][MAX_DEPARSER_CHECKSUM_UNITS];
    int lineno[2];
    std::vector<FDEntry> dictionary[2];
    std::vector<Phv::Ref> pov_order[2];
    ordered_map<const Phv::Register *, unsigned> pov[2];
    bitvec phv_use[2];
    std::set<int> constants[2];

    struct Intrinsic {
        struct Type;
        Type *type;
        int lineno;
        std::vector<Val> vals;
        Intrinsic(Type *t, int l) : type(t), lineno(l) {}
    };
    std::vector<Intrinsic> intrinsics;
    struct Digest {
        struct Type {
            target_t target;
            gress_t gress;
            std::string name;
            int count;
            bool can_shift = false;
            static std::map<std::string, Type *> all[TARGET_INDEX_LIMIT][2];

         protected:
            Type(target_t t, gress_t gr, const char *n, int cnt)
                : target(t), gress(gr), name(n), count(cnt) {
                BUG_CHECK(!all[target][gress].count(name));
                all[target][gress][name] = this;
            }
            ~Type() { all[target][gress].erase(name); }

         public:
#define VIRTUAL_TARGET_METHODS(TARGET)                                            \
    virtual void setregs(Target::TARGET::deparser_regs &regs, Deparser &deparser, \
                         Deparser::Digest &data) {                                \
        BUG_CHECK(!"target mismatch");                                            \
    }
            FOR_ALL_REGISTER_SETS(VIRTUAL_TARGET_METHODS)
#undef VIRTUAL_TARGET_METHODS
        };

        Type *type;
        int lineno;
        Val select;
        int shift = 0;
        std::map<int, std::vector<Phv::Ref>> layout;
        std::unique_ptr<json::map> context_json;
        Digest(Type *t, int lineno, VECTOR(pair_t) & data);
    };
    std::vector<Digest> digests;
    Deparser();
    ~Deparser();
    void start(int lineno, VECTOR(value_t) args);
    void input(VECTOR(value_t) args, value_t data);
    void process();
    std::vector<ChecksumVal> merge_csum_entries(const std::vector<ChecksumVal> &, int);
    template <class TARGET>
    void process(TARGET *);
    void output(json::map &);
    template <class REGS>
    void gen_learn_quanta(REGS &, json::vector &);
    template <class REGS>
    void write_config(REGS &);

    static const bitvec &PhvUse(gress_t gr) { return singleton_object.phv_use[gr]; }

    static bool add_constant(gress_t gr, int c) {
        if (!singleton_object.constants[gr].count(c)) {
            singleton_object.constants[gr].insert(c);
            if (int(singleton_object.constants[gr].size()) > Target::DEPARSER_CONSTANTS())
                return false;
        }
        return true;
    }

    static int constant_idx(gress_t gr, int c) {
        if (singleton_object.constants[gr].count(c))
            return std::distance(singleton_object.constants[gr].begin(),
                                 singleton_object.constants[gr].find(c));
        return -1;
    }

    // @return constant value that will be deparsed
    static int get_constant(gress_t gr, int phv_idx) {
        int i = 0;
        for (auto constant : singleton_object.constants[gr]) {
            if ((phv_idx - 224) == i) {
                return constant;
            } else {
                i++;
            }
        }
        return -1;
    }

    // Writes POV information in json used for field dictionary logging
    // and deparser resources
    static void write_pov_in_json(json::map &fd, json::map &fd_entry, const Phv::Register *phv,
                                  int bit, int offset) {
        auto povName = Phv::get_pov_name(phv->uid, offset);
        // Field dictionary logging
        fd["POV PHV"] = phv->uid;
        fd["POV Field bit"] = bit;
        fd["POV Field Name"] = povName;
        // Deparser resources
        fd_entry["pov_bit"] = bit;
        fd_entry["pov_name"] = povName;
        return;
    }

    // Digest Handle Setup
    // ------------------------------------------------------
    // | Pipe ID | Field Type |  Field List Handle          |
    // 31 ...   28           24                             0
    // Field List Handle = 24 bits
    // Field List Type = 4 bits (Field list is 0x9)
    // Pipe ID = 4 bits
    static unsigned unique_field_list_handle;
    static unsigned next_handle() {
        return unique_table_offset << PIPE_ID_SHIFT | FIELD_HANDLE_START |
               unique_field_list_handle++;
    }

    // gtest methods

    /// @brief Get the singleton object for use in gtest
    static Deparser *gtest_get_deparser() { return &singleton_object; }

    /// @brief Clear/reset the deparser object
    void gtest_clear();

 private:
    // Report deparser resources to JSON file
    void report_resources_deparser_json(json::vector &fde_entries_i, json::vector &fde_entries_e);
};

#endif /* DEPARSER_H_ */
