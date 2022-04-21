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

#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"

namespace DPDK {

// If any pass requires to add metadata field to metadata structure
// then just have to create and append struct field to static member newMetadataFields
// In passmanager's passes list AddNewMetadataFields should follow the pass which
// added fields into newMetadataFields
class AddNewMetadataFields : public Transform {
 public:
     static IR::IndexedVector<IR::StructField> newMetadataFields;
     const IR::Node* preorder(IR::DpdkStructType *st) override;
};

// This pass adds decl instance of Register extern in dpdk pna program which will
// be used by dpdk backend for initializing the mask for calculating packet direction
// and all the use point of istd.direction will follow below calculation and assignment
// istd.direction = network_port_mask.read(0) & (32w0x1 << istd.input_port)
class DirectionToRegRead : public Transform {
    ordered_map<cstring, cstring> dirToInput;
    IR::IndexedVector<IR::DpdkAsmStatement> newStmts;
    IR::IndexedVector<IR::StructField> &newMetadataFields = AddNewMetadataFields::newMetadataFields;
    ordered_set<cstring> usedNames;
    cstring reg_read_tmp;
    cstring left_shift_tmp;
    cstring registerInstanceName;

 public:
    DirectionToRegRead() {
    // direction to input metadata field name mapping
    dirToInput.insert(std::make_pair(cstring("pna_main_input_metadata_direction"),
                                     cstring("pna_main_input_metadata_input_port")));
    dirToInput.insert(std::make_pair(cstring("pna_pre_input_metadata_direction"),
                                     cstring("pna_pre_input_metadata_input_port")));
    dirToInput.insert(std::make_pair(cstring("pna_main_parser_input_metadata_direction"),
                                     cstring("pna_main_parser_input_metadata_input_port")));
    }
    void uniqueNames(IR::DpdkAsmProgram *p);
    const IR::Node* preorder(IR::DpdkAsmProgram *p) override;

    IR::DpdkExternDeclaration*
    addRegDeclInstance(cstring instanceName);
    void addMetadataField(cstring fieldName);
    bool isDirection(const IR::Member *m);
    const IR::Node *postorder(IR::DpdkAction *a);
    const IR::Node *postorder(IR::DpdkListStatement *l) override;
    void replaceDirection(const IR::Member *m);
    IR::IndexedVector<IR::DpdkAsmStatement>
    replaceDirectionWithRegRead(IR::IndexedVector<IR::DpdkAsmStatement> stmts);
};

class PrependPassRecircId : public Transform {
    IR::IndexedVector<IR::DpdkAsmStatement> newStmts;

 public:
    PrependPassRecircId() {}
    bool isPass(const IR::Member *m);
    const IR::Node *postorder(IR::DpdkAction *a);
    const IR::Node *postorder(IR::DpdkListStatement *l) override;
    IR::IndexedVector<IR::DpdkAsmStatement>
    prependPassWithRecircid(IR::IndexedVector<IR::DpdkAsmStatement> stmts);
};

}  // namespace DPDK
#endif  // BACKENDS_DPDK_DPDKMETADATA_H_
