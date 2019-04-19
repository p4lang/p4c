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

#ifndef _FRONTENDS_P4_CLONER_H_
#define _FRONTENDS_P4_CLONER_H_

#include "ir/ir.h"

namespace P4 {

// Inserting an IR dag in several places does not work,
// because PathExpressions must be unique.  The Cloner takes
// care of this.
class ClonePathExpressions : public Transform {
 public:
    ClonePathExpressions()
    { visitDagOnce = false; setName("ClonePathExpressions"); }
    const IR::Node* postorder(IR::PathExpression* path) override
    { return new IR::PathExpression(path->path->clone()); }

    template<typename T>
    const T* clone(const IR::Node* node)
    { return node->apply(*this)->to<T>(); }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CLONER_H_ */
