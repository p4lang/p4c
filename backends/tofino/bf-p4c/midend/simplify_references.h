/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MIDEND_SIMPLIFY_REFERENCES_H_
#define _MIDEND_SIMPLIFY_REFERENCES_H_

#include <vector>

#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

class ParamBinding;

/**
 * Simplify complex references to headers, header stacks, structs, and structs.
 *
 * This pass simplifies references so that the backend doesn't have to deal with
 * complicated cases. The simplifications include:
 *   - References to variables and package parameters are bound to concrete
 *     objects.
 *   - Member expressions are flattened to a single level.
 *   - Operations on structs and header stacks are split so that they only
 *     operate on a single header at a time. (This is mainly meant for the
 *     `extract()` and `emit()` methods.
 *   - Calls to the `isValid()` header method are replaced with references to
 *     the header's `$valid` POV bit field.
 */
struct SimplifyReferences : public PassManager {
    /**
     * Create a SimplifyReferences instance.
     *
     * @param bindings  Known bindings to package parameters and declarations.
     *                  References to these entities (i.e., via member or path
     *                  expressions) will be bound to concrete objects (i.e.,
     *                  HeaderRefs or HeaderStackItemRefs). Any additional
     *                  declarations encountered during the simplification
     *                  process will be added to be bindings for us by future
     *                  passes; this is an in-out parameter.
     * @param refMap    Resolved references for the P4 program.
     * @param typeMap   Resolved types for the P4 program.
     */
    SimplifyReferences(ParamBinding *bindings, P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
};

#endif /* _MIDEND_SIMPLIFY_REFERENCES_H_ */
