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

#ifndef BF_P4C_PHV_UTILS_LIVE_RANGE_REPORT_H_
#define BF_P4C_PHV_UTILS_LIVE_RANGE_REPORT_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "lib/log.h"

class LiveRangeReport : public Inspector {
 private:
    static constexpr unsigned READ = PHV::FieldUse::READ;
    static constexpr unsigned WRITE = PHV::FieldUse::WRITE;
    static constexpr unsigned LIVE = PHV::FieldUse::LIVE;

    const PhvInfo &phv;
    const TableSummary &alloc;
    const FieldDefUse &defuse;

    std::map<int, int> stageToReadBits;
    std::map<int, int> stageToWriteBits;
    std::map<int, int> stageToAccessBits;
    std::map<int, int> stageToLiveBits;

    int maxStages = -1;

    typedef ordered_map<const PHV::Field *, std::map<int, PHV::FieldUse>> LiveMap;
    LiveMap livemap;

    std::map<const PHV::Field *, const PHV::Field *> aliases;

    profile_t init_apply(const IR::Node *root) override;

    std::map<int, PHV::FieldUse> processUseDefSet(const FieldDefUse::LocPairSet &defuseSet,
                                                  PHV::FieldUse useDef) const;

    void setFieldLiveMap(const PHV::Field *f);

    std::vector<std::string> createStatRow(std::string title, const std::map<int, int> &data) const;

    cstring printFieldLiveness();
    cstring printBitStats() const;
    cstring printAliases() const;

 public:
    explicit LiveRangeReport(const PhvInfo &p, const TableSummary &t, const FieldDefUse &d)
        : phv(p), alloc(t), defuse(d) {}

    const LiveMap &get_livemap() const { return livemap; }
    int get_max_stages() const { return maxStages; }
};

#endif /*  BF_P4C_PHV_UTILS_LIVE_RANGE_REPORT_H_  */
