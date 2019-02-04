/*
Copyright 2018 VMware, Inc.

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

#ifndef _MIDEND_CHECKSIZE_H_
#define _MIDEND_CHECKSIZE_H_

#include "ir/ir.h"

namespace P4 {

/// Checks some possible misuses of the table size property
class CheckTableSize : public Modifier {
 public:
    CheckTableSize() { setName("CheckTableSize"); }
    bool preorder(IR::P4Table* table) override {
        auto size = table->getSizeProperty();
        if (size == nullptr)
            return false;

        bool deleteSize = false;
        auto key = table->getKey();
        if (key == nullptr) {
            if (size->value != 1) {
                ::warning(ErrorType::WARN_MISMATCH,
                          "%1%: size %2% specified for table without keys", table, size);
                deleteSize = true;
            }
        }
        auto entries = table->properties->getProperty(IR::TableProperties::entriesPropertyName);
        if (entries != nullptr && entries->isConstant) {
            ::warning(ErrorType::WARN_MISMATCH,
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

#endif /* _MIDEND_CHECKSIZE_H_ */
