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

#ifndef BF_P4C_MAU_NEXT_TABLE_H_
#define BF_P4C_MAU_NEXT_TABLE_H_

#include "ir/ir.h"

using namespace P4;

class NextTable : public virtual Visitor {
 public:
    virtual ordered_set<UniqueId> next_for(const IR::MAU::Table *tbl, cstring what) const = 0;
    virtual bool uses_next_table(const IR::MAU::Table *tbl) const = 0;
    virtual void dbprint(std::ostream &) const = 0;
    virtual const std::unordered_map<int, std::set<UniqueId>> &long_branches(UniqueId) const {
        static std::unordered_map<int, std::set<UniqueId>> empty;
        return empty;
    }
    virtual std::pair<ssize_t, ssize_t> get_live_range_for_lb_with_dest(UniqueId) const {
        return {-1, -1};
    }
    int long_branch_tag_for(UniqueId from, UniqueId to) const {
        for (auto &lb : long_branches(from))
            if (lb.second.count(to)) return lb.first;
        return -1;
    }
};

class DynamicNextTable : public DynamicVisitor, public NextTable, public IHasDbPrint {
    NextTable *pass = nullptr;

 public:
    ordered_set<UniqueId> next_for(const IR::MAU::Table *tbl, cstring what) const override {
        return pass->next_for(tbl, what);
    }
    bool uses_next_table(const IR::MAU::Table *tbl) const override {
        return pass->uses_next_table(tbl);
    }
    void dbprint(std::ostream &out) const override { pass->dbprint(out); }
    const std::unordered_map<int, std::set<UniqueId>> &long_branches(UniqueId id) const override {
        return pass->long_branches(id);
    }
    std::pair<ssize_t, ssize_t> get_live_range_for_lb_with_dest(UniqueId id) const override {
        return pass->get_live_range_for_lb_with_dest(id);
    }
    void setVisitor(NextTable *v) { DynamicVisitor::setVisitor((pass = v)); }
    DynamicNextTable *clone() const override { BUG("DynamicNextTable not cloneable"); }
};

#endif /* BF_P4C_MAU_NEXT_TABLE_H_ */
