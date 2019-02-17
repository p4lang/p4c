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
#include "frontends/p4/typeMap.h"

namespace P4 {

class TypeConstraints;

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
    TypeMap*          typeMap;

    bool unifyCall(const IR::Node* errorPosition,
                   const IR::Type_MethodBase* dest,
                   const IR::Type_MethodCall* src,
                   bool reportErrors);
    bool unifyFunctions(const IR::Node* errorPosition,
                        const IR::Type_MethodBase* dest,
                        const IR::Type_MethodBase* src,
                        bool reportErrors,
                        bool skipReturnValues = false);
    bool unifyBlocks(const IR::Node* errorPosition,
                     const IR::Type_ArchBlock* dest,
                     const IR::Type_ArchBlock* src,
                     bool reportErrors);

 public:
    explicit TypeUnification(TypeConstraints* constraints, TypeMap *typeMap) :
                             constraints(constraints), typeMap(typeMap) {}
    /**
     * Return false if unification fails right away.
     * Generates a set of type constraints.
     * If it returns true unification could still fail later.
     * @param dest         Destination type.
     * @param src          Source type.
     * @param reportErrors If true report errors caused by unification.
     * @return     False if unification fails immediately.
     */
    bool unify(const IR::Node* errorPosition, const IR::Type* dest,
               const IR::Type* src, bool reportErrors);
};

}  // namespace P4

#endif /* _TYPECHECKING_TYPEUNIFICATION_H_ */
