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

#ifndef BACKENDS_TOFINO_BF_ASM_P4_TABLE_H_
#define BACKENDS_TOFINO_BF_ASM_P4_TABLE_H_

#include <map>
#include <string>

#include "asm-types.h"
#include "backends/tofino/bf-asm/json.h"

class Table;
class P4Table;

struct alpm_t {
    std::string partition_field_name = "";
    unsigned alpm_atcam_table_handle = 0;
    unsigned alpm_pre_classifier_table_handle = 0;
    std::set<unsigned> set_partition_action_handle;
    json::map *alpm_atcam_table_cfg = 0;           // handle to cjson alpm table
    json::map *alpm_pre_classifier_table_cfg = 0;  // handle to cjson ternary pre classifier table
};

class P4Table {
    int lineno = -1;
    std::string name, preferred_match_type;
    std::string stage_table_type;
    unsigned handle = 0;
    bool explicit_size = false;
    bool hidden = false;
    json::map *config = 0;
    P4Table() {}

 public:
    bool disable_atomic_modify = false;
    unsigned size = 0;
    std::string match_type, action_profile, how_referenced;
    enum type {
        None = 0,
        MatchEntry = 1,
        ActionData = 2,
        Selection = 3,
        Statistics = 4,
        Meter = 5,
        Stateful = 6,
        NUM_TABLE_TYPES = 7
    };
    enum alpm_type { PreClassifier = 1, Atcam = 2 };
    static const char *type_name[];

 private:
    static std::map<unsigned, P4Table *> by_handle;
    static std::map<type, std::map<std::string, P4Table *>> by_name;
    static unsigned max_handle[];

 public:
    static P4Table *get(type t, VECTOR(pair_t) & d);
    static P4Table *alloc(type t, Table *tbl);
    void check(Table *tbl);
    const char *p4_name() const { return name.empty() ? nullptr : name.c_str(); }
    unsigned get_handle() { return handle; }
    unsigned p4_size() { return size; }
    std::string p4_stage_table_type() { return stage_table_type; }
    json::map *base_tbl_cfg(json::vector &out, int size, const Table *table) const;
    void base_alpm_tbl_cfg(json::map &out, int size, const Table *table,
                           P4Table::alpm_type atype) const;
    bool is_alpm() const {
        if (match_type == "alpm") return true;
        return false;
    }
    void set_partition_action_handle(unsigned handle);
    void set_partition_field_name(std::string name);
    std::string get_partition_field_name() const;
    std::set<unsigned> get_partition_action_handle() const;
    unsigned get_alpm_atcam_table_handle() const;
    static std::string direction_name(gress_t);
};

#endif /* BACKENDS_TOFINO_BF_ASM_P4_TABLE_H_ */
