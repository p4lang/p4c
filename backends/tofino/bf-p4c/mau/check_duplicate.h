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

#ifndef EXTENSIONS_BF_P4C_MAU_CHECK_DUPLICATE_H_
#define EXTENSIONS_BF_P4C_MAU_CHECK_DUPLICATE_H_

#include "ir/dump.h"

using namespace P4;

class CheckTableNameDuplicate : public MauInspector {
    std::set<cstring>        names;
    profile_t init_apply(const IR::Node *root) override {
        names.clear();
        return MauInspector::init_apply(root); }
    bool preorder(const IR::MAU::Table *t) override {
        auto name = t->name;
        if (t->is_placed())
            name = t->unique_id().build_name();
        if (names.count(name))
            BUG("Multiple tables named '%s'", name);
        names.insert(name);
        return true; }
};

class CheckDuplicateAttached : public Inspector {
    std::map<cstring, const IR::MAU::AttachedMemory *> attached;
 public:
    const char *pass = "";
    bool ok = true;
    profile_t init_apply(const IR::Node *root) {
        attached.clear();
        return Inspector::init_apply(root); }
    bool preorder(const IR::MAU::AttachedMemory *at) {
        if (attached.count(at->name)) {
            LOG1("Duplicated attached table " << at->name << " after " << pass);
            ok = false; }
        attached[at->name] = at;
        return true; }
    void end_apply(const IR::Node *root) {
        if (!ok && LOGGING(3)) {
            std::cout << "----------  After " << pass << "  ----------" << std::endl;
            dump(root); }
        BUG_CHECK(ok, "abort after %s", pass); }
};

#endif /* EXTENSIONS_BF_P4C_MAU_CHECK_DUPLICATE_H_ */
