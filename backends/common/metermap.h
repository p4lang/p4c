/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_COMMON_METERMAP_H_
#define BACKENDS_COMMON_METERMAP_H_

#include "ir/ir.h"

namespace P4 {

// TODO: This file should be in the BMv2 folder, but ProgramStructure still depends on it.
class DirectMeterMap final {
 public:
    struct DirectMeterInfo {
        const IR::Expression *destinationField;
        const IR::P4Table *table;
        unsigned tableSize;

        DirectMeterInfo() : destinationField(nullptr), table(nullptr), tableSize(0) {}
    };

 private:
    /// The key is a direct meter declaration.
    std::map<const IR::IDeclaration *, DirectMeterInfo *> directMeter;
    DirectMeterInfo *createInfo(const IR::IDeclaration *meter);

 public:
    DirectMeterInfo *getInfo(const IR::IDeclaration *meter);
    void setDestination(const IR::IDeclaration *meter, const IR::Expression *destination);
    void setTable(const IR::IDeclaration *meter, const IR::P4Table *table);
    void setSize(const IR::IDeclaration *meter, unsigned size);
};

}  // namespace P4

#endif /* BACKENDS_COMMON_METERMAP_H_ */
