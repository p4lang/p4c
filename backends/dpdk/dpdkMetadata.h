/*
Copyright 2022 Intel Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef BACKENDS_DPDK_DPDKMETADATA_H_
#define BACKENDS_DPDK_DPDKMETADATA_H_

#include <string>
#include <utility>
#include <vector>

#include "dpdkUtils.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace DPDK {

class IsDirectionMetadataUsed : public Inspector {
    bool &is_direction_used;

 public:
    explicit IsDirectionMetadataUsed(bool &is_direction_used)
        : is_direction_used(is_direction_used) {}
    bool preorder(const IR::Member *m) override {
        if (!is_direction_used && isDirection(m)) {
            is_direction_used = true;
        }
        return false;
    }
};

// This pass adds decl instance of Register extern in dpdk pna program which will
// be used by dpdk backend for initializing the mask for calculating packet direction
// at beginning of pipeline
class DirectionToRegRead : public Transform {
    ordered_map<cstring, cstring> dirToDirMapping;
    ordered_map<cstring, cstring> inputToInputPortMapping;
    ordered_set<cstring> usedNames;
    cstring registerInstanceName;

 public:
    DirectionToRegRead() {
        // dpdk currently only uses pna_main_input_metadata_direction
        // and pna_main_input_metadata_input_port
        // so even when we use any other direction or input port metadata we
        // replace it like below
        dirToDirMapping.insert(std::make_pair(cstring("pna_main_input_metadata_direction"),
                                              cstring("pna_main_input_metadata_direction")));
        dirToDirMapping.insert(std::make_pair(cstring("pna_pre_input_metadata_direction"),
                                              cstring("pna_main_input_metadata_direction")));
        dirToDirMapping.insert(std::make_pair(cstring("pna_main_parser_input_metadata_direction"),
                                              cstring("pna_main_input_metadata_direction")));
        inputToInputPortMapping.insert(
            std::make_pair(cstring("pna_main_input_metadata_input_port"),
                           cstring("pna_main_input_metadata_input_port")));
        inputToInputPortMapping.insert(
            std::make_pair(cstring("pna_pre_input_metadata_direction"),
                           cstring("pna_main_input_metadata_input_port")));
        inputToInputPortMapping.insert(
            std::make_pair(cstring("pna_main_parser_input_metadata_direction"),
                           cstring("pna_main_input_metadata_input_port")));
    }
    const IR::Node *preorder(IR::DpdkAsmProgram *p) override;
    const IR::Node *preorder(IR::Member *m) override;

    void uniqueNames(IR::DpdkAsmProgram *p);
    IR::DpdkExternDeclaration *addRegDeclInstance(cstring instanceName);
    IR::DpdkListStatement *replaceDirection(IR::DpdkListStatement *l);
    IR::IndexedVector<IR::DpdkAsmStatement> addRegReadStmtForDirection(
        IR::IndexedVector<IR::DpdkAsmStatement> stmts);
};

// DPDK implements pass metadata using "recircid" instruction.
// All occurrences of pass metadata usage should be prepended with recircid instruction to fetch
// the latest pass_id from the target.
class PrependPassRecircId : public Transform {
    IR::IndexedVector<IR::DpdkAsmStatement> newStmts;

 public:
    PrependPassRecircId() {}
    bool isPass(const IR::Member *m);
    const IR::Node *postorder(IR::DpdkAction *a) override;
    const IR::Node *postorder(IR::DpdkListStatement *l) override;
    IR::IndexedVector<IR::DpdkAsmStatement> prependPassWithRecircid(
        IR::IndexedVector<IR::DpdkAsmStatement> stmts);
};

}  // namespace DPDK
#endif  // BACKENDS_DPDK_DPDKMETADATA_H_
