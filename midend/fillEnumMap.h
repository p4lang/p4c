/*
 * Copyright 2019 RT-RK Computer Based Systems.
 * SPDX-FileCopyrightText: 2019 RT-RK Computer Based Systems.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_FILLENUMMAP_H_
#define MIDEND_FILLENUMMAP_H_

#include "convertEnums.h"

namespace P4 {

class FillEnumMap : public Transform {
 public:
    ConvertEnums::EnumMapping repr;
    ChooseEnumRepresentation *policy;
    TypeMap *typeMap;
    FillEnumMap(ChooseEnumRepresentation *policy, TypeMap *typeMap)
        : policy(policy), typeMap(typeMap) {
        CHECK_NULL(policy);
        CHECK_NULL(typeMap);
        setName("FillEnumMap");
    }
    const IR::Node *preorder(IR::Type_Enum *type) override;
};

}  // namespace P4

#endif /* MIDEND_FILLENUMMAP_H_ */
