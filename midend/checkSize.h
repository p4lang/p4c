/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_CHECKSIZE_H_
#define MIDEND_CHECKSIZE_H_

#include "ir/ir.h"

namespace P4 {

/// Checks some possible misuses of the table size property
class CheckTableSize : public Modifier {
 public:
    CheckTableSize() { setName("CheckTableSize"); }
    bool preorder(IR::P4Table *table) override {
        auto size = table->getSizeProperty();
        if (size == nullptr) return false;

        bool deleteSize = false;
        auto key = table->getKey();
        if (key == nullptr) {
            if (size->value != 1) {
                warn(ErrorType::WARN_MISMATCH, "%1%: size %2% specified for table without keys",
                     table, size);
                deleteSize = true;
            }
        }
        auto entries = table->properties->getProperty(IR::TableProperties::entriesPropertyName);
        if (entries != nullptr && entries->isConstant) {
            warn(ErrorType::WARN_MISMATCH,
                 "%1%: size %2% specified for table with constant entries", table, size);
            deleteSize = true;
        }
        if (deleteSize) {
            auto props = IR::IndexedVector<IR::Property>(table->properties->properties);
            props.removeByName(IR::TableProperties::sizePropertyName);
            table->properties = new IR::TableProperties(table->properties->srcInfo, props);
        }
        return false;
    }
};

}  // namespace P4

#endif /* MIDEND_CHECKSIZE_H_ */
