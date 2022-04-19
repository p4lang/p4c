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

#ifndef BACKENDS_DPDK_DPDKPNADIRECTION_H_
#define BACKENDS_DPDK_DPDKPNADIRECTION_H_

#include "ir/ir.h"

namespace DPDK {

class AddNewMetadataFields : public Transform {
 public:
     static IR::IndexedVector<IR::StructField> newMetadataFields;
     const IR::Node* preorder(IR::DpdkStructType *st) override;
};

class DirectionToRegRead : public Transform {
    ordered_map<cstring, cstring> dirToInput;
    IR::IndexedVector<IR::DpdkAsmStatement> newStmts;
    IR::IndexedVector<IR::StructField> &newMetadataFields = AddNewMetadataFields::newMetadataFields;
    ordered_set<cstring> newFieldName;
    cstring reg_read_tmp;
    cstring left_shift_tmp;
    cstring registerInstanceName;

 public:
    DirectionToRegRead() {
    reg_read_tmp = "reg_read_tmp";
    left_shift_tmp = "left_shift_tmp";
    registerInstanceName = "network_port_mask";
    // direction to input metadata field name mapping
    dirToInput.insert(std::make_pair(cstring("pna_main_input_metadata_direction"),
                                     cstring("pna_main_input_metadata_input_port")));
    dirToInput.insert(std::make_pair(cstring("pna_pre_input_metadata_direction"),
                                     cstring("pna_pre_input_metadata_input_port")));
    dirToInput.insert(std::make_pair(cstring("pna_main_parser_input_metadata_direction"),
                                     cstring("pna_main_parser_input_metadata_input_port")));
    }

    const IR::Node* preorder(IR::DpdkAsmProgram *p) override;

    // create and add register declaration instance to program
    IR::DpdkExternDeclaration*
    addRegDeclInstance(cstring instanceName);
    // add new fields in metadata structure
    void addMetadataField(cstring fieldName);
    // check member expression using metadata direction field
    bool isDirection(const IR::Member *m);
    const IR::Node *postorder(IR::DpdkAction *a);
    const IR::Node *postorder(IR::DpdkListStatement *l) override;
    // replace direction field uses with register read i.e.
    // istd.direction -> (direction_port_mask.read(0) & (32w0x1 << istd.input_port))
    void replaceDirection(const IR::Member *m);
    IR::IndexedVector<IR::DpdkAsmStatement>
    replaceDirectionWithRegRead(IR::IndexedVector<IR::DpdkAsmStatement> stmts);
};

}  // namespace DPDK
#endif  // BACKENDS_DPDK_DPDKPNADIRECTION_H_
