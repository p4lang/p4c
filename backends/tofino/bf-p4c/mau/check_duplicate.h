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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_CHECK_DUPLICATE_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_CHECK_DUPLICATE_H_

#include "ir/dump.h"

using namespace P4;

class CheckTableNameDuplicate : public MauInspector {
    std::set<cstring> names;
    profile_t init_apply(const IR::Node *root) override {
        names.clear();
        return MauInspector::init_apply(root);
    }
    bool preorder(const IR::MAU::Table *t) override {
        auto name = t->name;
        if (t->is_placed()) name = t->unique_id().build_name();
        if (names.count(name)) BUG("Multiple tables named '%s'", name);
        names.insert(name);
        return true;
    }
};

class CheckDuplicateAttached : public Inspector {
    std::map<cstring, const IR::MAU::AttachedMemory *> attached;

 public:
    const char *pass = "";
    bool ok = true;
    profile_t init_apply(const IR::Node *root) {
        attached.clear();
        return Inspector::init_apply(root);
    }
    bool preorder(const IR::MAU::AttachedMemory *at) {
        if (attached.count(at->name)) {
            LOG1("Duplicated attached table " << at->name << " after " << pass);
            ok = false;
        }
        attached[at->name] = at;
        return true;
    }
    void end_apply(const IR::Node *root) {
        if (!ok && LOGGING(3)) {
            std::cout << "----------  After " << pass << "  ----------" << std::endl;
            dump(root);
        }
        BUG_CHECK(ok, "abort after %s", pass);
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_CHECK_DUPLICATE_H_ */
