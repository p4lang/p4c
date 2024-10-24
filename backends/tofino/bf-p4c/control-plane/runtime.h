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

#ifndef BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_RUNTIME_H_
#define BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_RUNTIME_H_

#include "ir/ir.h"

namespace P4 {
namespace IR {
class P4Program;
}  // namespace IR
}  // namespace P4

using namespace P4;

class BFN_Options;

namespace BFN {

/*
 * Certain names are reserved for BF-RT and cannot be used on
 * controls. Pass checks for control names overlapping restricted ones, append
 * to the list as required.
 */
class CheckReservedNames : public Inspector {
    std::set<cstring> reservedNames = {"snapshot"_cs};
    bool preorder(const IR::Type_ArchBlock *b) override;

 public:
    CheckReservedNames() {}
};

/**
 * \ingroup midend
 * \brief Pass that sets default table size to 512 entries.
 *
 * If no 'size' parameter is set on the table, p4info picks a default value of 1024.
 * p4c/frontends/p4-14/fromv1.0/v1model.h: const unsigned defaultTableSize = 1024;
 * This pass is run again in the MidEnd to modify IR and set this value.
 * BF-RT does not modify program IR used by Midend.
 */
class SetDefaultSize : public Modifier {
    bool warn = false;
    bool preorder(IR::P4Table *table) override;

 public:
    explicit SetDefaultSize(bool warn) : warn(warn) {}
};

/// A convenience wrapper for P4::generateP4Runtime(). This must be called
/// before the translation pass and will generate the correct P4Info message
/// based on the original architecture (v1model, PSA, TNA or JNA).
void generateRuntime(const IR::P4Program *program, const BFN_Options &options);

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_RUNTIME_H_ */
