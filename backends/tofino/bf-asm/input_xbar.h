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

#ifndef BACKENDS_TOFINO_BF_ASM_INPUT_XBAR_H_
#define BACKENDS_TOFINO_BF_ASM_INPUT_XBAR_H_

#include <fstream>

#include "backends/tofino/bf-utils/dynamic_hash/dynamic_hash.h"
#include "constants.h"
#include "lib/ordered_map.h"
#include "phv.h"

class Table;
class HashExpr;

struct HashCol {
    int lineno = -1;
    HashExpr *fn = 0;
    int bit = 0;
    bitvec data;
    unsigned valid = 0;  // Used only in Tofino
    void dbprint(std::ostream &out) const;
};

inline std::ostream &operator<<(std::ostream &out, HashCol &col) {
    col.dbprint(out);
    return out;
}

struct DynamicIXbar {
    int bit = -1;
    std::vector<std::pair<Phv::Ref, match_t>> match_phv;
    match_t match;

    DynamicIXbar() = default;
    DynamicIXbar(const DynamicIXbar &) = default;
    DynamicIXbar(DynamicIXbar &&) = default;
    DynamicIXbar &operator=(const DynamicIXbar &) = default;
    DynamicIXbar &operator=(DynamicIXbar &&) = default;
    DynamicIXbar(const Table *, const pair_t &);
};

class InputXbar {
 public:
    struct Group {
        short index;
        enum type_t { INVALID, EXACT, TERNARY, BYTE, GATEWAY, XCMP } type;
        Group() : index(-1), type(INVALID) {}
        Group(Group::type_t t, int i) : index(i), type(t) {}
        explicit operator bool() const { return type != INVALID; }
        bool operator==(const Group &a) const { return type == a.type && index == a.index; }
        bool operator<(const Group &a) const {
            return (type << 16) + index < (a.type << 16) + a.index;
        }
    };
    struct HashTable {
        short index;
        enum type_t { INVALID, EXACT, XCMP } type;
        HashTable() : index(-1), type(INVALID) {}
        HashTable(type_t t, int i) : index(i), type(t) {}
        explicit operator bool() const { return type != INVALID; }
        bool operator==(const HashTable &a) const { return type == a.type && index == a.index; }
        bool operator<(const HashTable &a) const {
            return (type << 16) + index < (a.type << 16) + a.index;
        }
        std::string toString() const;
        unsigned uid() const;
    };

 protected:
    struct Input {
        Phv::Ref what;
        int lo, hi;
        explicit Input(const Phv::Ref &a) : what(a), lo(-1), hi(-1) {}
        Input(const Phv::Ref &a, int s) : what(a), lo(s), hi(-1) {}
        Input(const Phv::Ref &a, int l, int h) : what(a), lo(l), hi(h) {}
        unsigned size() const { return hi - lo + 1; }
    };
    struct HashGrp {
        int lineno = -1;
        unsigned tables = 0;  // Bit set for table index
        uint64_t seed = 0;
        bool seed_parity = false;  // Parity to be set on the seed value
    };
    Table *table;
    ordered_map<Group, std::vector<Input>> groups;
    std::map<HashTable, std::map<int, HashCol>> hash_tables;
    // Map of hash table index to parity bit set on the table
    std::map<HashTable, unsigned> hash_table_parity;
    std::map<unsigned, HashGrp> hash_groups;
    static bool conflict(const std::vector<Input> &a, const std::vector<Input> &b);
    static bool conflict(const std::map<int, HashCol> &, const std::map<int, HashCol> &, int * = 0);
    static bool conflict(const HashGrp &a, const HashGrp &b);
    bool copy_existing_hash(HashTable ht, std::pair<const int, HashCol> &col);
    uint64_t hash_columns_used(HashTable hash);
    uint64_t hash_columns_used(unsigned id) {
        BUG_CHECK(id < Target::EXACT_HASH_TABLES(), "%d out of range for exact hash", id);
        return hash_columns_used(HashTable(HashTable::EXACT, id));
    }
    bool can_merge(HashGrp &a, HashGrp &b);
    void add_use(unsigned &byte_use, std::vector<Input> &a);
    virtual int hash_num_columns(HashTable ht) const { return 52; }
    virtual int group_max_index(Group::type_t t) const;
    virtual Group group_name(bool ternary, const value_t &value) const;
    virtual int group_size(Group::type_t t) const;
    const char *group_type(Group::type_t t) const;
    void parse_group(Table *t, Group gr, const value_t &value);
    virtual bool parse_hash(Table *t, const pair_t &kv) { return false; }
    void parse_hash_group(HashGrp &hash_group, const value_t &value);
    void parse_hash_table(Table *t, HashTable ht, const value_t &value);
    virtual bool parse_unit(Table *t, const pair_t &kv) { return false; }
    void setup_hash(std::map<int, HashCol> &, HashTable ht, gress_t, int stage, value_t &,
                    int lineno, int lo, int hi);
    struct TcamUseCache {
        std::map<int, std::pair<const Input &, int>> tcam_use;
        std::set<InputXbar *> ixbars_added;
    };
    virtual void check_input(Group group, Input &input, TcamUseCache &tcam_use);
    int tcam_input_use(int out_byte, int phv_byte, int phv_size);
    void tcam_update_use(TcamUseCache &use);
    void gen_hash_column(std::pair<const int, HashCol> &col,
                         std::pair<const HashTable, std::map<int, HashCol>> &hash);

    struct GroupSet : public IHasDbPrint {
        Group group;
        const std::vector<InputXbar *> &use;
        GroupSet(const std::vector<InputXbar *> &u, Group g) : group(g), use(u) {}
        GroupSet(ordered_map<Group, std::vector<InputXbar *>> &u, Group g) : group(g), use(u[g]) {}
        void dbprint(std::ostream &) const;
        const Input *find(Phv::Slice sl) const;
        std::vector<const Input *> find_all(Phv::Slice sl) const;
    };

    InputXbar() = delete;
    InputXbar(const InputXbar &) = delete;
    void input(Table *table, bool ternary, const VECTOR(pair_t) & data);
    InputXbar(Table *table, int lineno) : table(table), lineno(lineno) {}

 public:
    const int lineno;
    int random_seed = -1;
    static std::unique_ptr<InputXbar> create(Table *table, const value_t *key = nullptr);
    static std::unique_ptr<InputXbar> create(Table *table, bool tern, const value_t &key,
                                             const VECTOR(pair_t) & data);
    void pass1();
    virtual void pass2();
    template <class REGS>
    void write_regs(REGS &regs);
    template <class REGS>
    void write_xmu_regs(REGS &regs);
    template <class REGS>
    void write_galois_matrix(REGS &regs, HashTable id, const std::map<int, HashCol> &mat);
    bool have_exact() const {
        for (auto &grp : groups)
            if (grp.first.type == Group::EXACT) return true;
        return false;
    }
    bool have_ternary() const {
        for (auto &grp : groups)
            if (grp.first.type != Group::EXACT) return true;
        return false;
    }
    int hash_group() const {
        /* used by gateways to get the associated hash group */
        if (hash_groups.size() != 1) return -1;
        return hash_groups.begin()->first;
    }
    bitvec hash_group_bituse(int grp = -1) const;
    std::vector<const HashCol *> hash_column(int col, int grp = -1) const;
    int match_group() {
        /* used by gateways and stateful to get the associated match group */
        if (groups.size() != 1 || groups.begin()->first.type != Group::EXACT) return -1;
        return groups.begin()->first.index;
    }
    bitvec bytemask();
    /* functions for tcam ixbar that take into account funny byte/word group stuff */
    unsigned tcam_width();
    int tcam_byte_group(int n);
    int tcam_word_group(int n);
    std::map<HashTable, std::map<int, HashCol>> &get_hash_tables() { return hash_tables; }
    const std::map<int, HashCol> &get_hash_table(HashTable id);
    const std::map<int, HashCol> &get_hash_table(unsigned id = 0) {
        return get_hash_table(HashTable(HashTable::EXACT, id));
    }

    // which Group provides the input for a given HashTable
    virtual Group hashtable_input_group(HashTable ht) const {
        BUG_CHECK(ht.type == HashTable::EXACT, "not an exact hash table");
        return Group(Group::EXACT, ht.index / 2);
    }
    virtual Phv::Ref get_hashtable_bit(HashTable id, unsigned bit) const {
        BUG_CHECK(id.type == HashTable::EXACT, "not an exact hash table");
        return get_group_bit(Group(Group::EXACT, id.index / 2), bit + 64 * (id.index & 0x1));
    }
    Phv::Ref get_hashtable_bit(unsigned id, unsigned bit) const {
        return get_hashtable_bit(HashTable(HashTable::EXACT, id), bit);
    }
    Phv::Ref get_group_bit(Group grp, unsigned bit) const {
        if (groups.count(grp))
            for (auto &in : groups.at(grp))
                if (bit >= unsigned(in.lo) && bit <= unsigned(in.hi))
                    return Phv::Ref(in.what, bit - in.lo, bit - in.lo);
        return Phv::Ref();
    }
    std::string get_field_name(int bit) {
        for (auto &g : groups) {
            for (auto &p : g.second) {
                if (bit <= p.hi && bit >= p.lo) return p.what.name();
            }
        }
        return "";
    }
    bool is_p4_param_bit_in_hash(std::string p4_param_name, unsigned bit) {
        for (auto &g : groups) {
            for (auto &p : g.second) {
                std::string phv_field_name = p.what.name();
                auto phv_field_lobit = remove_name_tail_range(phv_field_name);
                phv_field_lobit += p.what.fieldlobit();
                auto phv_field_hibit = phv_field_lobit + p.size() - 1;
                if (p4_param_name == phv_field_name && bit <= phv_field_hibit &&
                    bit >= phv_field_lobit)
                    return true;
            }
        }
        return false;
    }
    unsigned get_seed_bit(unsigned group, unsigned bit) const {
        if (hash_groups.count(group)) return ((hash_groups.at(group).seed >> bit) & 0x1);
        return 0;
    }
    HashGrp *get_hash_group(unsigned group = -1) { return ::getref(hash_groups, group); }
    HashGrp *get_hash_group_from_hash_table(int hash_table) {
        if (hash_table < 0 || hash_table >= Target::EXACT_HASH_TABLES()) return nullptr;
        for (auto &hg : hash_groups) {
            if (hg.second.tables & (1U << hash_table)) return &hg.second;
        }
        return nullptr;
    }
    bool log_hashes(std::ofstream &out) const;
    virtual unsigned exact_physical_ids() const { return -1; }

    class all_iter {
        decltype(groups)::const_iterator outer, outer_end;
        bool inner_valid;
        std::vector<Input>::const_iterator inner;
        void mk_inner_valid() {
            if (!inner_valid) {
                if (outer == outer_end) return;
                inner = outer->second.begin();
            }
            while (inner == outer->second.end()) {
                if (++outer == outer_end) return;
                inner = outer->second.begin();
            }
            inner_valid = true;
        }
        struct iter_deref : public std::pair<Group, const Input &> {
            explicit iter_deref(const std::pair<Group, const Input &> &a)
                : std::pair<Group, const Input &>(a) {}
            iter_deref *operator->() { return this; }
        };

     public:
        all_iter(decltype(groups)::const_iterator o, decltype(groups)::const_iterator oend)
            : outer(o), outer_end(oend), inner_valid(false) {
            mk_inner_valid();
        }
        bool operator==(const all_iter &a) {
            if (outer != a.outer) return false;
            if (inner_valid != a.inner_valid) return false;
            return inner_valid ? inner == a.inner : true;
        }
        all_iter &operator++() {
            if (inner_valid && ++inner == outer->second.end()) {
                ++outer;
                inner_valid = false;
                mk_inner_valid();
            }
            return *this;
        }
        std::pair<Group, const Input &> operator*() {
            return std::pair<Group, const Input &>(outer->first, *inner);
        }
        iter_deref operator->() { return iter_deref(**this); }
    };
    all_iter begin() const { return all_iter(groups.begin(), groups.end()); }
    all_iter end() const { return all_iter(groups.end(), groups.end()); }

    const Input *find(Phv::Slice sl, Group grp, Group *found = nullptr) const;
    const Input *find_exact(Phv::Slice sl, int group) const {
        return find(sl, Group(Group::EXACT, group));
    }
    virtual int find_offset(const MatchSource *, Group grp, int offset) const;
    int find_gateway_offset(const MatchSource *ms, int offset) const {
        return find_offset(ms, Group(Group::GATEWAY, 0), offset);
    }
    int find_match_offset(const MatchSource *ms, int offset = -1) const {
        return find_offset(ms, Group(Group::EXACT, -1), offset);
    }

    std::vector<const Input *> find_all(Phv::Slice sl, Group grp) const;
    virtual std::vector<const Input *> find_hash_inputs(Phv::Slice sl, HashTable ht) const;
    virtual int global_bit_position_adjust(HashTable ht) const {
        BUG_CHECK(ht.type == HashTable::EXACT, "not an exact hash table");
        return (ht.index / 2) * 128;
    }
    virtual bitvec global_column0_extract(
        HashTable ht, const hash_column_t matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN]) const {
        BUG_CHECK(ht.type == HashTable::EXACT, "not an exact hash table");
        return bitvec(matrix[ht.index][0].column_value);
    }
    virtual void setup_match_key_cfg(const MatchSource *) {}  // noop for tofino1/2
};

inline std::ostream &operator<<(std::ostream &out, InputXbar::Group gr) {
    switch (gr.type) {
        case InputXbar::Group::EXACT:
            out << "exact";
            break;
        case InputXbar::Group::TERNARY:
            out << "ternary";
            break;
        case InputXbar::Group::BYTE:
            out << "byte";
            break;
        case InputXbar::Group::GATEWAY:
            out << "gateway";
            break;
        case InputXbar::Group::XCMP:
            out << "xcmp";
            break;
        default:
            out << "<type=" << static_cast<int>(gr.type) << ">";
    }
    return out << " ixbar group " << gr.index;
}

inline std::ostream &operator<<(std::ostream &out, InputXbar::HashTable ht) {
    switch (ht.type) {
        case InputXbar::HashTable::EXACT:
            out << "exact";
            break;
        case InputXbar::HashTable::XCMP:
            out << "xcmp";
            break;
        default:
            out << "<type=" << static_cast<int>(ht.type) << ">";
    }
    return out << " hashtable " << ht.index;
}

#endif /* BACKENDS_TOFINO_BF_ASM_INPUT_XBAR_H_ */
