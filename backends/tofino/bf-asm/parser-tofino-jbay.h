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

#ifndef PARSER_TOFINO_JBAY_H_
#define PARSER_TOFINO_JBAY_H_

#include <map>
#include <set>
#include <vector>

#include "bitvec.h"
#include "parser.h"
#include "phv.h"
#include "sections.h"
#include "target.h"
#include "ubits.h"

enum {
    /* global constants related to parser */
    PARSER_STATE_MASK = 0xff,
    PARSER_TCAM_DEPTH = 256,
    PARSER_CHECKSUM_ROWS = 32,
    PARSER_CTRINIT_ROWS = 16,
    PARSER_INPUT_BUFFER_SIZE = 32,
    PARSER_SRC_MAX_IDX = 63,
    PARSER_MAX_CLOTS = 64,
    PARSER_MAX_CLOT_LENGTH = 64,
};

/**
 * @brief Representation of the Tofino 1/2 parser in assembler
 * @ingroup parde
 */
class Parser : public BaseParser, public Contextable {
    void write_config(RegisterSetBase &regs, json::map &json, bool legacy = true) override;
    template <class REGS>
    void write_config(REGS &, json::map &, bool legacy = true);
    struct CounterInit {
        gress_t gress;
        int lineno = -1, addr = -1;
        int add = 0, mask = 255, rot = 0, max = 255, src = -1;
        CounterInit(gress_t, pair_t);
        void pass1(Parser *) {}
        void pass2(Parser *);
        template <class REGS>
        void write_config(REGS &, gress_t, int);
        bool equiv(const CounterInit &) const;
    };
    struct PriorityUpdate {
        int lineno = -1, offset = -1, shift = -1, mask = -1;
        PriorityUpdate() {}
        explicit PriorityUpdate(const value_t &data);
        bool parse(const value_t &exp, int what = 0);
        explicit operator bool() const { return lineno >= 0; }
        template <class REGS>
        void write_config(REGS &);
    };
    struct RateLimit {
        int lineno = -1;
        int inc = -1, dec = -1, max = -1, interval = -1;
        void parse(const VECTOR(pair_t) &);
        explicit operator bool() const { return lineno >= 0; }
        template <class REGS>
        void write_config(REGS &, gress_t);
    };

 public:
    struct Checksum;

    struct State {
        struct Ref {
            int lineno;
            std::string name;
            match_t pattern;
            std::vector<State *> ptr;
            Ref() : lineno(-1) { pattern.word0 = pattern.word1 = 0; }
            Ref &operator=(const value_t &);
            explicit Ref(value_t &v) { *this = v; }
            operator bool() const { return ptr.size() > 0; }
            State *operator->() const {
                BUG_CHECK(ptr.size() == 1);
                return ptr[0];
            }
            State *operator*() const {
                BUG_CHECK(ptr.size() == 1);
                return ptr[0];
            }
            bool operator==(const Ref &a) const { return name == a.name && pattern == a.pattern; }
            void check(gress_t, Parser *, State *);
            std::vector<State *>::const_iterator begin() const { return ptr.begin(); }
            std::vector<State *>::const_iterator end() const { return ptr.end(); }
        };
        struct MatchKey {
            int lineno;
            struct {
                short bit, byte;
            } data[4];
            enum { USE_SAVED = 0x7fff }; /* magic number can be stored in 'byte' field */
            short specified;
            short ctr_zero, ctr_neg;
            short width;
            short save = 0;
            MatchKey() : lineno(0), specified(0), ctr_zero(-1), ctr_neg(-1), width(0) {
                for (auto &a : data) a.bit = a.byte = -1;
            }
            void setup(value_t &);
            int setup_match_el(int, value_t &);
            void preserve_saved(unsigned mask);
            template <class REGS>
            void write_config(REGS &, json::vector &);

         private:
            int add_byte(int, int, bool use_saved = false);
            int move_down(int);
        };
        struct OutputUse {
            unsigned b8 = 0, b16 = 0, b32 = 0;
            OutputUse &operator+=(const OutputUse &a) {
                b8 += a.b8;
                b16 += a.b16;
                b32 += a.b32;
                return *this;
            }
        };
        struct Match {
            int lineno;
            State *state = nullptr;
            match_t match;
            std::string value_set_name;
            int value_set_size = 0;
            int value_set_handle = -1;
            int offset_inc = 0, shift = 0, buf_req = -1;
            int disable_partial_hdr_err = -1, partial_hdr_err_proc = -1;
            bool offset_rst = false;
            int intr_md_bits = 0;

            int ctr_imm_amt = 0, ctr_ld_src = 0, ctr_load = 0;
            bool ctr_stack_push = false, ctr_stack_upd_w_top = false, ctr_stack_pop = false;

            CounterInit *ctr_instr = nullptr;

            PriorityUpdate priority;

            Ref next;
            MatchKey load;

            int row = -1;
            /// Data for narrow to wide extraction analysis, flag and
            /// vector of affected PHV locations
            bool has_narrow_to_wide_extract = false;
            // 32b narrow to wide extractions using 2x16 extractions
            std::vector<const Phv::Ref *> narrow_to_wide_32b_16;
            // 32b narrow to wide extractions using 4x8 extractions
            std::vector<const Phv::Ref *> narrow_to_wide_32b_8;
            // 16b narrow to wide extractions using 2x8 extractions
            std::vector<const Phv::Ref *> narrow_to_wide_16b_8;

            enum flags_t { OFFSET = 1, ROTATE = 2 };

            struct Save {
                Match *match;
                int lo, hi;
                Phv::Ref where, second;
                int flags;
                Save(gress_t, Match *m, int l, int h, value_t &data, int flgs = 0);
                template <class REGS>
                int write_output_config(REGS &, void *, unsigned &, int, int) const;
            };
            std::vector<Save *> save;

            struct Set {
                Match *match = nullptr;
                Phv::Ref where;
                unsigned what;
                int flags;
                Set(gress_t gress, Match *m, value_t &data, int v, int flgs = 0);
                template <class REGS>
                void write_output_config(REGS &, void *, unsigned &, int, int) const;
                bool merge(gress_t, const Set &a);
                bool operator==(const Set &a) const {
                    return where == a.where && what == a.what && flags == a.flags;
                }
            };
            std::vector<Set *> set;

            struct Clot {
                int lineno, tag;
                std::string name;
                bool load_length = false;
                int start = -1, length = -1, length_shift = -1, length_mask = -1;
                int max_length = -1;
                int csum_unit = -1;
                int stack_depth = 1;
                int stack_inc = 1;
                Clot(gress_t gress, const value_t &tag, const value_t &data);
                Clot(const Clot &) = delete;
                Clot(Clot &&) = delete;
                bool parse_length(const value_t &exp, int what = 0);
                template <class PO_ROW>
                void write_config(PO_ROW &, int, bool) const;

             private:
                Clot(gress_t, const Clot &, int);
            };
            std::vector<Clot *> clots;
            std::vector<Checksum> csum;

            struct FieldMapping {
                Phv::Ref where;
                std::string container_id;
                int lo = -1;
                int hi = -1;
                FieldMapping(Phv::Ref &ref, const value_t &a);
            };
            std::vector<FieldMapping> field_mapping;

            struct HdrLenIncStop {
                int lineno = -1;
                unsigned final_amt = 0;
                HdrLenIncStop() {}
                explicit HdrLenIncStop(const value_t &data);
                explicit operator bool() const { return lineno >= 0; }
                template <class PO_ROW>
                void write_config(PO_ROW &) const;
            } hdr_len_inc_stop;

            Match(int lineno, gress_t, State *s, match_t m, VECTOR(pair_t) & data);
            Match(int lineno, gress_t, State *n);
            ~Match() {
                if (ctr_instr) delete ctr_instr;
            }
            void unmark_reachable(Parser *, State *state, bitvec &unreach);
            void pass1(Parser *pa, State *state);
            void pass2(Parser *pa, State *state);
            template <class REGS>
            int write_load_config(REGS &, Parser *, State *, int) const;
            template <class REGS>
            void write_lookup_config(REGS &, State *, int) const;
            template <class EA_REGS>
            void write_counter_config(EA_REGS &) const;
            template <class REGS>
            void write_common_row_config(REGS &, Parser *, State *, int, Match *, json::map &);
            template <class REGS>
            void write_row_config(REGS &, Parser *, State *, int, Match *, json::map &);
            template <class REGS>
            void write_config(REGS &, Parser *, State *, Match *, json::map &);
            template <class REGS>
            void write_config(REGS &, json::vector &);

            template <class REGS>
            void write_saves(REGS &regs, Match *def, void *output_map, int &max_off, unsigned &used,
                             int csum_8b, int csum_16b);
            template <class REGS>
            void write_sets(REGS &regs, Match *def, void *output_map, unsigned &used, int csum_8b,
                            int csum_16b);

            std::set<Match *> get_all_preds();
            std::set<Match *> get_all_preds_impl(std::set<Match *> &visited);
        };

        std::string name;
        gress_t gress;
        match_t stateno;
        MatchKey key;
        std::vector<Match *> match;
        Match *def;
        std::set<Match *> pred;
        bool ignore_max_depth = false;
        int lineno = -1;
        int all_idx = -1;

        State(State &&) = default;
        State(int lineno, const char *name, gress_t, match_t stateno, const VECTOR(pair_t) & data);
        bool can_be_start();
        void unmark_reachable(Parser *, bitvec &);
        void pass1(Parser *);
        void pass2(Parser *);
        template <class REGS>
        int write_lookup_config(REGS &, Parser *, State *, int, const std::vector<State *> &);
        template <class REGS>
        void write_config(REGS &, Parser *, json::vector &);
    };

    struct Checksum {
        int lineno = -1, addr = -1, unit = -1;
        gress_t gress;
        Phv::Ref dest;
        int tag = -1;
        unsigned add = 0, mask = 0, swap = 0, mul_2 = 0;
        unsigned dst_bit_hdr_end_pos = 0;
        bool start = false, end = false, shift = false;
        unsigned type = 0;  // 0 = verify, 1 = residual, 2 = clot
        Checksum(gress_t, pair_t);
        bool equiv(const Checksum &) const;
        void pass1(Parser *);
        void pass2(Parser *);
        template <class REGS>
        void write_config(REGS &, Parser *);
        template <class REGS>
        void write_output_config(REGS &, Parser *, State::Match *, void *, unsigned &) const;

     private:
        template <typename ROW>
        void write_tofino_row_config(ROW &row);
        template <typename ROW>
        void write_row_config(ROW &row);
    };

 public:
    void input(VECTOR(value_t) args, value_t data);
    void process();
    void output(json::map &) override;
    void output_legacy(json::map &);
    gress_t gress;
    std::string name;
    std::map<std::string, State *> states;
    std::vector<State *> all;
    std::map<State::Match *, int> match_to_row;
    bitvec port_use;
    int parser_no;  // used to print cfg.json
    bitvec state_use;
    State::Ref start_state[4];
    int priority[4] = {0};
    int pri_thresh[4] = {0, 0, 0, 0};
    int tcam_row_use = 0;
    Phv::Ref parser_error;
    // the ghost "parser" extracts 32-bit value
    // this information is first extracted in AsmParser and passed to
    // individual Parser, because currently parse_merge register is programmed
    // in Parser class.
    // FIXME -- should move all merge reg handling into AsmParser.
    std::vector<Phv::Ref> ghost_parser;
    unsigned ghost_pipe_mask = 0xf;  // only set for JBAY
    bitvec (&phv_use)[2];
    bitvec phv_allow_bitwise_or, phv_allow_clear_on_write;
    bitvec phv_init_valid;
    int hdr_len_adj = 0, meta_opt = 0;
    std::vector<std::array<Checksum *, PARSER_CHECKSUM_ROWS>> checksum_use;
    std::array<CounterInit *, PARSER_CTRINIT_ROWS> counter_init = {};
    static std::map<gress_t, std::map<std::string, std::vector<State::Match::Clot *>>> clots;
    static std::array<std::vector<State::Match::Clot *>, PARSER_MAX_CLOTS> clot_use;
    static unsigned max_handle;
    int parser_handle = -1;
    RateLimit rate_limit;

    Parser(bitvec (&phv_use)[2], gress_t gr, int idx)
        : gress(gr), parser_no(idx), phv_use(phv_use) {
        if (gress == INGRESS) {
            parser_depth_max_bytes = Target::PARSER_DEPTH_MAX_BYTES_INGRESS();
            parser_depth_min_bytes = Target::PARSER_DEPTH_MIN_BYTES_INGRESS();
        } else {
            parser_depth_max_bytes = Target::PARSER_DEPTH_MAX_BYTES_EGRESS();
            parser_depth_min_bytes = Target::PARSER_DEPTH_MIN_BYTES_EGRESS();
        }
    }

    template <class REGS>
    void gen_configuration_cache(REGS &, json::vector &cfg_cache);
    static int clot_maxlen(gress_t gress, unsigned tag) {
        auto &vec = clot_use[tag];
        return vec.empty() ? -1 : vec.at(0)->max_length;
    }
    static int clot_maxlen(gress_t gress, std::string tag) {
        if (clots.count(gress) && clots.at(gress).count(tag))
            return clots.at(gress).at(tag).at(0)->max_length;
        return -1;
    }
    static int clot_tag(gress_t gress, std::string tag) {
        if (clots.count(gress) && clots.at(gress).count(tag))
            return clots.at(gress).at(tag).at(0)->tag;
        return -1;
    }

    static const char *match_key_loc_name(int loc);
    static int match_key_loc(const char *key);
    static int match_key_loc(value_t &key, bool errchk = true);
    static int match_key_size(const char *key);

    // Parser Handle Setup
    // ____________________________________________________
    // | Table Type | Pipe Id | Parser Handle | PVS Handle |
    // 31          24        20              12            0
    // PVS Handle = 12 bits
    // Parser Handle = 8 bits
    // Pipe ID = 4 bits
    // Table Type = 8 bits (Parser type is 15)
    static unsigned next_handle() {
        // unique_table_offset is to support multiple pipe.
        // assume parser type is 15, table type used 0 - 6
        return max_handle++ << 12 | unique_table_offset << 20 | 15 << 24;
    }
    // Store parser names to their handles. Used by phase0 match tables to link
    // parser handle
    static std::map<std::string, unsigned> parser_handles;
    static unsigned get_parser_handle(std::string phase0Table) {
        for (auto p : Parser::parser_handles) {
            auto parser_name = p.first;
            if (phase0Table.find(parser_name) != std::string::npos) return p.second;
        }
        return 0;
    }

    template <class REGS>
    void *setup_phv_output_map(REGS &, gress_t, int);

    State *get_start_state() {
        std::vector<std::string> startNames = {"start", "START", "$entry_point.start",
                                               "$entry_point"};
        for (auto n : startNames) {
            if (states.count(n)) return states.at(n);
        }
        return nullptr;
    }

    int get_prsr_max_dph();
    int get_header_stack_size_from_valid_bits(std::vector<State::Match::Set *> sets);

    // Debug
    void print_all_paths();

 private:
    template <class REGS>
    void mark_unused_output_map(REGS &, void *, unsigned);
    void define_state(gress_t gress, pair_t &kv);
    void output_default_ports(json::vector &vec, bitvec port_use);
    int state_prsr_dph_max(const State *s);
    int state_prsr_dph_max(const State *s, std::map<const State *, std::pair<int, int>> &visited,
                           int curr_dph_bits);
    int parser_depth_max_bytes, parser_depth_min_bytes;
};

class AsmParser : public BaseAsmParser {
    std::vector<Parser *> parser[2];     // INGRESS, EGRESS
    bitvec phv_use[2];                   // ingress/egress only
    std::vector<Phv::Ref> ghost_parser;  // the ghost "parser" extracts 32-bit value. This 32-bit
                                         // can be from a single 32-bit container or multiple
                                         // smaller one.
    unsigned ghost_pipe_mask = 0xf;      // only set for JBAY
    void start(int lineno, VECTOR(value_t) args) override;
    void input(VECTOR(value_t) args, value_t data) override;
    void process() override;
    void output(json::map &) override;
    void init_port_use(bitvec &port_use, const value_t &arg);

 public:
    AsmParser() : BaseAsmParser("parser"){};
    ~AsmParser() {}

    // For gtest
    std::vector<Parser *> test_get_parser(gress_t gress);
};

template <class REGS>
void Parser::PriorityUpdate::write_config(REGS &action_row) {
    if (offset >= 0) {
        action_row.pri_upd_type = 1;
        action_row.pri_upd_src = offset;
        action_row.pri_upd_en_shr = shift;
        action_row.pri_upd_val_mask = mask;
    } else {
        action_row.pri_upd_type = 0;
        action_row.pri_upd_en_shr = 1;
        action_row.pri_upd_val_mask = mask;
    }
}

// for jbay (tofino1 is specialized)
template <>
void Parser::RateLimit::write_config(::Tofino::regs_pipe &regs, gress_t gress);
template <class REGS>
void Parser::RateLimit::write_config(REGS &regs, gress_t gress) {
    if (gress == INGRESS) {
        auto &ctrl = regs.pardereg.pgstnreg.parbreg.left.i_phv_rate_ctrl;
        ctrl.inc = inc;
        ctrl.interval = interval;
        ctrl.max = max;
    } else if (gress == EGRESS) {
        auto &ctrl = regs.pardereg.pgstnreg.parbreg.right.e_phv_rate_ctrl;
        ctrl.inc = inc;
        ctrl.interval = interval;
        ctrl.max = max;
    }
}

template <class REGS>
void Parser::State::MatchKey::write_config(REGS &, json::vector &) {
    // FIXME -- TBD -- probably needs to be different for tofino/jbay, so there will be
    // FIXME -- template specializations for this in those files
}

template <class REGS>
void Parser::State::Match::write_saves(REGS &regs, Match *def, void *output_map, int &max_off,
                                       unsigned &used, int csum_8b, int csum_16b) {
    if (offset_inc)
        for (auto s : save) s->flags |= OFFSET;
    for (auto s : save)
        max_off =
            std::max(max_off, s->write_output_config(regs, output_map, used, csum_8b, csum_16b));
    if (def)
        for (auto &s : def->save)
            max_off = std::max(max_off,
                               s->write_output_config(regs, output_map, used, csum_8b, csum_16b));
}

template <class REGS>
void Parser::State::Match::write_sets(REGS &regs, Match *def, void *output_map, unsigned &used,
                                      int csum_8b, int csum_16b) {
    if (offset_inc)
        for (auto s : set) s->flags |= ROTATE;
    for (auto s : set) s->write_output_config(regs, output_map, used, csum_8b, csum_16b);
    if (def)
        for (auto s : def->set) s->write_output_config(regs, output_map, used, csum_8b, csum_16b);
}

template <class REGS>
void Parser::State::Match::write_common_row_config(REGS &regs, Parser *pa, State *state, int row,
                                                   Match *def, json::map &ctxt_json) {
    int max_off = -1;
    write_lookup_config(regs, state, row);

    auto &ea_row = regs.memory[state->gress].ml_ea_row[row];
    if (ctr_instr || ctr_load || ctr_imm_amt || ctr_stack_pop) {
        write_counter_config(ea_row);
    } else if (def) {
        def->write_counter_config(ea_row);
    }
    if (shift)
        max_off = std::max(max_off, int(ea_row.shift_amt = shift) - 1);
    else if (def)
        max_off = std::max(max_off, int(ea_row.shift_amt = def->shift) - 1);
    max_off = std::max(max_off, write_load_config(regs, pa, state, row));
    if (auto &next = (!this->next && def) ? def->next : this->next) {
        std::vector<State *> prev;
        for (auto n : next) {
            max_off = std::max(max_off, n->write_lookup_config(regs, pa, state, row, prev));
            prev.push_back(n);
        }
        const match_t &n = next.pattern ? next.pattern : next->stateno;
        ea_row.nxt_state = n.word1;
        ea_row.nxt_state_mask = ~(n.word0 & n.word1) & PARSER_STATE_MASK;
    } else {
        ea_row.done = 1;
    }

    auto &action_row = regs.memory[state->gress].po_action_row[row];
    for (auto &c : csum) {
        action_row.csum_en[c.unit] = 1;
        action_row.csum_addr[c.unit] = c.addr;
    }
    if (offset_inc || offset_rst) {
        action_row.dst_offset_inc = offset_inc;
        action_row.dst_offset_rst = offset_rst;
    } else if (def) {
        action_row.dst_offset_inc = def->offset_inc;
        action_row.dst_offset_rst = def->offset_rst;
    }
    if (priority) priority.write_config(action_row);
    if (hdr_len_inc_stop) hdr_len_inc_stop.write_config(action_row);

    void *output_map = pa->setup_phv_output_map(regs, state->gress, row);
    unsigned used = 0;
    int csum_8b = 0;
    int csum_16b = 0;
    for (auto &c : csum) {
        c.write_output_config(regs, pa, this, output_map, used);
        if (c.type == 0 && c.dest) {
            if (c.dest->reg.size == 8)
                ++csum_8b;
            else if (c.dest->reg.size == 16)
                ++csum_16b;
        }
    }

    if (options.target == TOFINO) {
        write_sets(regs, def, output_map, used, csum_8b, csum_16b);
        write_saves(regs, def, output_map, max_off, used, csum_8b, csum_16b);
    } else {
        write_sets(regs, def, output_map, used, 0, 0);
        write_saves(regs, def, output_map, max_off, used, 0, 0);
    }

    int clot_unit = 0;
    for (auto *c : clots) c->write_config(action_row, clot_unit++, offset_inc > 0);
    if (def)
        for (auto *c : def->clots) c->write_config(action_row, clot_unit++, offset_inc > 0);
    pa->mark_unused_output_map(regs, output_map, used);

    if (buf_req < 0) {
        buf_req = max_off + 1;
        BUG_CHECK(buf_req <= 32);
    }
    ea_row.buf_req = buf_req;
}

template <class REGS>
void Parser::State::Match::write_row_config(REGS &regs, Parser *pa, State *state, int row,
                                            Match *def, json::map &ctxt_json) {
    write_common_row_config(regs, pa, state, row, def, ctxt_json);
}

template <class REGS>
void Parser::State::Match::write_config(REGS &regs, Parser *pa, State *state, Match *def,
                                        json::map &ctxt_json) {
    int row, count = 0;
    do {
        if ((row = --pa->tcam_row_use) < 0) {
            if (row == -1)
                error(state->lineno, "Ran out of tcam space in %sgress parser",
                      state->gress ? "e" : "in");
            return;
        }
        ctxt_json["tcam_rows"].to<json::vector>().push_back(row);
        write_row_config(regs, pa, state, row, def, ctxt_json);
        pa->match_to_row[this] = row;
    } while (++count < value_set_size);
}

template <class REGS>
void Parser::State::Match::write_config(REGS &regs, json::vector &vec) {
    int select_statement_bit = 0;
    for (auto f : field_mapping) {
        json::map container_cjson;
        container_cjson["container_width"] = Parser::match_key_size(f.container_id.c_str());

        int container_hardware_id = Parser::match_key_loc(f.container_id.c_str());
        container_cjson["container_hardware_id"] = container_hardware_id;

        container_cjson["mask"] = (1 << (f.hi - f.lo + 1)) - 1;
        json::vector field_mapping_cjson;
        for (auto i = f.lo; i <= f.hi; i++) {
            json::map field_map;
            field_map["register_bit"] = i;
            field_map["field_name"] = f.where.name();
            field_map["start_bit"] = i;
            field_map["select_statement_bit"] = select_statement_bit++;
            field_mapping_cjson.push_back(field_map.clone());
        }
        container_cjson["field_mapping"] = field_mapping_cjson.clone();
        vec.push_back(container_cjson.clone());
    }
}

template <class REGS>
void Parser::State::write_config(REGS &regs, Parser *pa, json::vector &ctxt_json) {
    LOG2(gress << " state " << name << " (" << stateno << ')');
    for (auto i : match) {
        bool uses_pvs = false;
        json::map state_cjson;
        state_cjson["parser_name"] = name;
        i->write_config(regs, state_cjson["match_registers"]);
        if (i->value_set_size > 0) uses_pvs = true;
        i->write_config(regs, pa, this, def, state_cjson);
        state_cjson["uses_pvs"] = uses_pvs;
        if (def) def->write_config(regs, pa, this, 0, state_cjson);
        if (uses_pvs) {
            state_cjson["pvs_name"] = i->value_set_name;
            if (i->value_set_handle < 0)
                error(lineno, "Invalid handle for parser value set %s", i->value_set_name.c_str());
            auto pvs_handle_full = i->value_set_handle;
            state_cjson["pvs_handle"] = pvs_handle_full;
        }
        for (auto idx : MatchIter(stateno)) {
            state_cjson["parser_state_id"] = idx;
            ctxt_json.push_back(state_cjson.clone());
        }
    }
}

template <typename ROW>
void Parser::Checksum::write_tofino_row_config(ROW &row) {
    row.add = add;
    if (dest)
        row.dst = dest->reg.parser_id();
    else if (tag >= 0)
        row.dst = tag;
    row.dst_bit_hdr_end_pos = dst_bit_hdr_end_pos;
    row.hdr_end = end;
    int rsh = 0;
    for (auto &el : row.mask) el = (mask >> rsh++) & 1;
    row.shr = shift;
    row.start = start;
    rsh = 0;
    for (auto &el : row.swap) el = (swap >> rsh++) & 1;
    row.type = type;
}

template <class ROW>
void Parser::Checksum::write_row_config(ROW &row) {
    write_tofino_row_config(row);
    int rsh = 0;
    for (auto &el : row.mul_2) el = (mul_2 >> rsh++) & 1;
}

// Used with JBay
bitvec expand_parser_groups(bitvec phvs);
bitvec remove_nonparser(bitvec phvs);
void setup_jbay_ownership(bitvec phv_use[2], checked_array<128, ubits<1>> &left,
                          checked_array<128, ubits<1>> &right, checked_array<256, ubits<1>> &main_i,
                          checked_array<256, ubits<1>> &main_e);
void setup_jbay_no_multi_write(bitvec phv_allow_bitwise_or, bitvec phv_allow_clear_on_write,
                               checked_array<256, ubits<1>> &nmw_i,
                               checked_array<256, ubits<1>> &nmw_e);
void setup_jbay_clear_on_write(bitvec phv_allow_clear_on_write, checked_array<128, ubits<1>> &left,
                               checked_array<128, ubits<1>> &right,
                               checked_array<256, ubits<1>> &main_i,
                               checked_array<256, ubits<1>> &main_e);

#endif /* PARSER_TOFINO_JBAY_H_ */
