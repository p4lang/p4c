/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
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
        return empty; }
    virtual std::pair<ssize_t, ssize_t> get_live_range_for_lb_with_dest(UniqueId) const {
        return { -1, -1 }; }
    int long_branch_tag_for(UniqueId from, UniqueId to) const {
        for (auto &lb : long_branches(from))
            if (lb.second.count(to))
                return lb.first;
        return -1; }
};

class DynamicNextTable : public DynamicVisitor, public NextTable, public IHasDbPrint {
    NextTable   *pass = nullptr;

 public:
    ordered_set<UniqueId> next_for(const IR::MAU::Table *tbl, cstring what) const override {
        return pass->next_for(tbl, what); }
    bool uses_next_table(const IR::MAU::Table *tbl) const override {
        return pass->uses_next_table(tbl); }
    void dbprint(std::ostream &out) const override { pass->dbprint(out); }
    const std::unordered_map<int, std::set<UniqueId>> &long_branches(UniqueId id) const override {
        return pass->long_branches(id); }
    std::pair<ssize_t, ssize_t> get_live_range_for_lb_with_dest(UniqueId id) const override {
        return pass->get_live_range_for_lb_with_dest(id); }
    void setVisitor(NextTable *v) { DynamicVisitor::setVisitor((pass = v)); }
    DynamicNextTable *clone() const override { BUG("DynamicNextTable not cloneable"); }
};

#endif /* BF_P4C_MAU_NEXT_TABLE_H_ */
