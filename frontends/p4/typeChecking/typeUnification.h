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

#ifndef _TYPECHECKING_TYPEUNIFICATION_H_
#define _TYPECHECKING_TYPEUNIFICATION_H_

#include "ir/ir.h"

namespace P4 {

class TypeConstraints;
class EqualityConstraint;

/**
Hindley-Milner type unification algorithm
(See for example, http://cs.brown.edu/~sk/Publications/Books/ProgLangs/2007-04-26/plai-2007-04-26.pdf, page 280.)
Attempts to unify two types.  As a consequence it generates constraints on other sub-types.
Constraints are solved at the end.
Solving a constraint generates type-variable substitutions
(where a type variable is replaced with another type - which could still contain type variables inside).
All substitutions are composed together.
Constraint solving can fail, which means that the program does not type-check.
*/
class TypeUnification final {
    TypeConstraints*  constraints;
    const TypeMap* typeMap;

    bool unifyCall(const EqualityConstraint* constraint);
    bool unifyFunctions(const EqualityConstraint* constraint,
                        bool skipReturnValues = false);
    bool unifyBlocks(const EqualityConstraint* constraint);
 public:
    TypeUnification(TypeConstraints* constraints, const P4::TypeMap* typeMap) :
            constraints(constraints), typeMap(typeMap) {}
    /**
     * Return false if unification fails right away.
     * Generates a set of type constraints.
     * If it returns true unification could still fail later.
     * @return     False if unification fails immediately.
     */
    bool unify(const EqualityConstraint* constraint);
};

}  // namespace P4

#endif /* _TYPECHECKING_TYPEUNIFICATION_H_ */
