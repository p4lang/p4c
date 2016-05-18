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

#ifndef _FRONTENDS_COMMON_TYPEMAP_H_
#define _FRONTENDS_COMMON_TYPEMAP_H_

#include "ir/ir.h"
#include "frontends/common/programMap.h"

namespace P4 {
/*
 * Maps nodes to their canonical types.
 * Not all Node objects have types.
 * The type of each node is maintained in the typeMap.
 * Objects that have a type in the map:
 * - all Expression objects (includes Literals)
 * - function parameters
 * - variables and constant declarations
 * - functors (control, parser, etc.)
 * - functions
 * - interfaces
 * - enum fields (pointing to the enclosing enum)
 * - error (pointing to the error type)
 * - prototypes (function and functor prototypes); map name to the actual type
 */
class TypeMap final : public ProgramMap {
 protected:
    // Map each node to its canonical type
    std::map<const IR::Node*, const IR::Type*> typeMap;
    // All left-values in the program.
    std::set<const IR::Expression*> leftValues;
    // All compile-time constants.  A compile-time constant
    // is not necessarily a constant - it could be a directionless
    // parameter as well.
    std::set<const IR::Expression*> constants;
    // checks some preconditions before setting the type
    void checkPrecondition(const IR::Node* element, const IR::Type* type) const;

    static const IR::Type_InfInt* canonInfInt;

    friend class TypeInference;
    friend class ConstantTypeSubstitution;

 public:
    TypeMap() : ProgramMap("TypeMap") {}

    bool contains(const IR::Node* element) { return typeMap.count(element) != 0; }
    void setType(const IR::Node* element, const IR::Type* type);
    const IR::Type* getType(const IR::Node* element, bool notNull = false) const;
    void dbprint(std::ostream& out) const;
    void clear();
    bool isLeftValue(const IR::Expression* expression) const
    { return leftValues.count(expression) > 0; }
    bool isCompileTimeConstant(const IR::Expression* expression) const;
    size_t size() const
    { return typeMap.size(); }

 private:
    // The following are only used by TypeInference
    void setLeftValue(const IR::Expression* expression);
    void setCompileTimeConstant(const IR::Expression* expression);
    const IR::Type_InfInt* canonicalInfInt() const
    { return TypeMap::canonInfInt; }
};
}  // namespace P4

#endif /* _FRONTENDS_COMMON_TYPEMAP_H_ */
