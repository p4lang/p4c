/*
Copyright 2022 VMware, Inc.

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

#ifndef P4_VALIDATEVALUESETS_H_
#define P4_VALIDATEVALUESETS_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/error.h"

namespace P4 {

/**
 * Checks that match annotations only have 1 argument which is of type match_kind.
 */
class ValidateValueSets final : public Inspector {
 public:
    ValidateValueSets() { setName("ValidateValueSets"); }
    void postorder(const IR::P4ValueSet *valueSet) override {
        if (!valueSet->size->is<IR::Constant>()) {
            ::error(ErrorType::ERR_EXPECTED, "%1%: value_set size must be constant",
                    valueSet->size);
        }
    }
};

}  // namespace P4

#endif /* P4_VALIDATEVALUESETS_H_ */
