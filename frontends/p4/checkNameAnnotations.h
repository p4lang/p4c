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

#ifndef _FRONTENDS_P4_CHECKNAMEANNOTATIONS_H_
#define _FRONTENDS_P4_CHECKNAMEANNOTATIONS_H_

#include "ir/ir.h"

namespace P4 {

/// Check if two objects in the initial program have identical name
/// annotations and report a warning.
class CheckNameAnnotations : public Inspector {
    /// List of all annotated objects.
    std::map<cstring, const IR::Node*> annotated;
 public:
    CheckNameAnnotations() {
        setName("CheckNameAnnotations");
    }

    bool preorder(const IR::Annotation* anno) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CHECKNAMEANNOTATIONS_H_ */
