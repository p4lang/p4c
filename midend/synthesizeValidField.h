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

#ifndef _MIDEND_SYNTHESIZEVALIDFIELD_H_
#define _MIDEND_SYNTHESIZEVALIDFIELD_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace P4 {

/**
 * Adds an explicit valid bit to each header type and replaces calls to
 * isValid() with references to the new field. This better reflects how header
 * validity is actually implemented on some backends, which can simplify later
 * passes.
 *
 * WARNING: This pass should be used carefully; it breaks invariants
 * about the IR that are assumed by other passes.  It should only be
 * used very late in the compilation tool-chain, if at all.
 *
 * In most situations, `foo.isValid()` is rewritten to `foo.$valid$ == 1`.
 * However, when `foo.isValid()` is a table key element (and isn't part of a
 * larger expression), it's replaced with a simple match against `foo.$valid$`
 * itself. This changes the type of the key element from boolean to bit<1>;
 * to accomodate that change, the corresponding constant table entry keys are
 * also rewritten to use bit<1> values instead of booleans.
 *
 * Calls to isValid() on header unions aren't rewritten; they can't be desugared
 * to a simple read of a hidden field.
 *
 * XXX(seth): It would be cleaner to rewrite setValid() and setInvalid() too,
 * but we currently can't if we want to use this pass with BMV2 (or at least,
 * we'd need to make it optional). Those compile down into special BMV2
 * primitives that are needed for certain features (e.g. header unions) to work
 * correctly.
 *
 * Example:
 *
 *   header h {
 *     bit<32> field;
 *   }
 *   control c(h h0, h h1) {
 *     table t {
 *       key { h0.isValid() : exact }
 *     }
 *     apply {
 *       if (h1.isValid()) t.apply();
 *     }
 *   }
 *
 * is replaced by:
 *
 *   header h {
 *     bit<32> field;
 *     bit<1> $valid$;
 *   }
 *   control c(h h0, h h1) {
 *     table t {
 *       key { h0.$valid$ : exact }
 *     }
 *     apply {
 *       if (h1.$valid$ == 1) t.apply();
 *     }
 *   }
 *
 *   @pre None.
 *   @post All header types have a new field which serves as a valid bit, and
 *         all calls to isValid() on headers are replaced with expressions
 *         involving the new field. When constant table entry keys use boolean
 *         values to match against the result of isValid(), they're rewritten to
 *         a constant bit<1> value. Afterwards, the ReferenceMap and TypeMap
 *         may be out of date.
 */
class SynthesizeValidField final : public PassManager {
 public:
    SynthesizeValidField(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);
};

}  // namespace P4

#endif  /* _MIDEND_SYNTHESIZEVALIDFIELD_H_ */
