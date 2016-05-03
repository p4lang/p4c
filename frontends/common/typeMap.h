#ifndef _FRONTENDS_COMMON_TYPEMAP_H_
#define _FRONTENDS_COMMON_TYPEMAP_H_

#include "ir/ir.h"

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
class TypeMap final {
 protected:
    // Map each node to its canonical type
    std::map<const IR::Node*, const IR::Type*> typeMap;
    // All left-values in the program.
    std::set<const IR::Expression*> leftValues;

    // checks some preconditions before setting the type
    void checkPrecondition(const IR::Node* element, const IR::Type* type) const;

    static const IR::Type_InfInt* canonInfInt;

 public:
    bool contains(const IR::Node* element) { return typeMap.count(element) != 0; }
    void setType(const IR::Node* element, const IR::Type* type);
    const IR::Type* getType(const IR::Node* element, bool notNull = false) const;
    void dbprint(std::ostream& out) const;
    void clear();
    bool isLeftValue(const IR::Expression* expression) const
    { return leftValues.count(expression) > 0; }

    // The following are only used by the type-checker
    void setLeftValue(const IR::Expression* expression);
    const IR::Type_InfInt* canonicalInfInt() const
    { return TypeMap::canonInfInt; }

    const IR::P4Program* program;  // program whose type map this is (if known)
};
}  // namespace P4

#endif /* _FRONTENDS_COMMON_TYPEMAP_H_ */
