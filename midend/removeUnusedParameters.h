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

#ifndef _MIDEND_REMOVEUNUSEDPARAMETERS_H_
#define _MIDEND_REMOVEUNUSEDPARAMETERS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

/**
 * Removes action parameters which are never referenced.
 *
 * This can reduce the amount of space required to represent a table, because it
 * reduces the amount of action data. However, in the absence of special
 * target-specific features to support it, this optimization changes the control
 * plane API. For that reason, it's generally not applied for P4-16 programs,
 * since it can be quite surprising if commenting out a block of code changes
 * the control plane API. Some P4-14 compilers *did* apply this optimization,
 * though, and in those cases it's necessary for backwards compatibility.
 *
 * \code{.cpp}
 *    action a(bit<32> x, bit<32> y) { some_extern(x); }
 * \endcode
 *
 * is converted to
 *
 * \code{.cpp}
 *    action a(bit<32> x) { some_extern(x); }
 * \endcode
 *
 * @pre No preconditions.
 * @post Unused action parameters are removed.
 */
class RemoveUnusedActionParameters : public Transform {
 public:
    explicit RemoveUnusedActionParameters(ReferenceMap* refMap) : refMap(refMap)
    { CHECK_NULL(refMap); setName("RemoveUnusedActionParameters"); }

    const IR::Node* postorder(IR::P4Action* action) override;

 private:
  ReferenceMap* refMap;
};

}  // namespace P4

#endif /* _MIDEND_REMOVEUNUSEDPARAMETERS_H_ */
