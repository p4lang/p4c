/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_VALIDATEPROPERTIES_H_
#define _MIDEND_VALIDATEPROPERTIES_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

/**
 * Checks to see if there are any unknown properties.
 *
 * @pre none
 * @post raise an error if there are invalid table properties in P4 program.
 */
class ValidateTableProperties : public Inspector {
    std::set<cstring> legalProperties;
 public:
    ValidateTableProperties(const std::initializer_list<cstring> legal) {
        setName("ValidateTableProperties");
        legalProperties.emplace("actions");
        legalProperties.emplace("default_action");
        legalProperties.emplace("key");
        legalProperties.emplace("entries");
        for (auto l : legal)
            legalProperties.emplace(l);
    }
    void postorder(const IR::Property* property) override;
    // don't check properties in externs (Declaration_Instances)
    bool preorder(const IR::Declaration_Instance *) override { return false; }
};

}  // namespace P4

#endif /* _MIDEND_VALIDATEPROPERTIES_H_ */
