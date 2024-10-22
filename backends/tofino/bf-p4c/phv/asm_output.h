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

#ifndef BF_P4C_PHV_ASM_OUTPUT_H_
#define BF_P4C_PHV_ASM_OUTPUT_H_

#include <iosfwd>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/live_range_report.h"
#include "lib/ordered_set.h"

class PhvAsmOutput {
    const PhvInfo &phv;
    const FieldDefUse &defuse;
    const TableSummary &tbl_summary;
    const LiveRangeReport *live_range_report;
    bool have_ghost;

    struct FieldUse {
        cstring name;
        gress_t gress;
        int live_start;
        int live_end;
        bool parsed;
        bool deparsed;
        ordered_set<cstring> mutex_fields;
        FieldUse(cstring n, gress_t g, int s, int e, bool p, bool d, ordered_set<cstring> m)
            : name(n),
              gress(g),
              live_start(s),
              live_end(e),
              parsed(p),
              deparsed(d),
              mutex_fields(m) {}
        std::string get_live_stage(int stage, int maxStages) {
            if (stage == -1) return "parser";
            if (stage >= maxStages) return "deparser";
            return std::to_string(stage);
        }
        std::string get_live_start(int maxStages) {
            return (parsed ? "parser" : get_live_stage(live_start, maxStages));
        }
        std::string get_live_end(int maxStages) {
            return (deparsed ? "deparser" : get_live_stage(live_end, maxStages));
        }
    };

    /// Populate liveRanges.
    typedef ordered_map<cstring, std::vector<FieldUse>> FieldUses;
    typedef std::map<PHV::Container, FieldUses> LiveRangePerContainer[2];
    void getLiveRanges(LiveRangePerContainer &c) const;

    void emit_gress(std::ostream &out, gress_t gress) const;
    friend std::ostream &operator<<(std::ostream &, const PhvAsmOutput &);

    std::map<int, PHV::FieldUse> processUseDefSet(const FieldDefUse::LocPairSet &,
                                                  PHV::FieldUse) const;

 public:
    explicit PhvAsmOutput(const PhvInfo &p, const FieldDefUse &defuse,
                          const TableSummary &tbl_summary, const LiveRangeReport *live_range_report,
                          bool have_ghost = false);
};

#endif /* BF_P4C_PHV_ASM_OUTPUT_H_ */
