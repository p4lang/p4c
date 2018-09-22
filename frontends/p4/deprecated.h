/*
Copyright 2017 VMware, Inc.

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

#ifndef _FRONTENDS_P4_DEPRECATED_H_
#define _FRONTENDS_P4_DEPRECATED_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Checks for the use of symbols that are marked as @deprecated and
 * gives warnings.
 */
class CheckDeprecated : public Inspector {
    const ReferenceMap* refMap;
 public:
    explicit CheckDeprecated(const ReferenceMap* refMap): refMap(refMap)
    { CHECK_NULL(refMap); setName("CheckDeprecated"); }

    void warnIfDeprecated(const IR::IAnnotated* declaration, const IR::Node* errorNode);

    bool preorder(const IR::PathExpression* path) override;
    bool preorder(const IR::Type_Name* name) override;
};

class Deprecated : public PassManager {
 public:
    explicit Deprecated(ReferenceMap* refMap) {
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new CheckDeprecated(refMap));
        setName("Deprecated");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_DEPRECATED_H_ */
