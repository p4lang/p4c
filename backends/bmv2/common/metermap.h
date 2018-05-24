/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef BACKENDS_BMV2_COMMON_METERMAP_H_
#define BACKENDS_BMV2_COMMON_METERMAP_H_

#include "ir/ir.h"

namespace BMV2 {

class DirectMeterMap final {
 public:
    struct DirectMeterInfo {
        const IR::Expression* destinationField;
        const IR::P4Table* table;
        unsigned tableSize;

        DirectMeterInfo() : destinationField(nullptr), table(nullptr), tableSize(0) {}
    };

 private:
    // key is declaration of direct meter
    std::map<const IR::IDeclaration*, DirectMeterInfo*> directMeter;
    DirectMeterInfo* createInfo(const IR::IDeclaration* meter);
 public:
    DirectMeterInfo* getInfo(const IR::IDeclaration* meter);
    void setDestination(const IR::IDeclaration* meter, const IR::Expression* destination);
    void setTable(const IR::IDeclaration* meter, const IR::P4Table* table);
    void setSize(const IR::IDeclaration* meter, unsigned size);
};

}  // namespace BMV2

#endif  /* BACKENDS_BMV2_COMMON_METERMAP_H_ */
