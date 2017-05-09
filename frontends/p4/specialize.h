/*
Copyright 2016 VMware, Inc.

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

#ifndef _FRONTENDS_P4_SPECIALIZE_H_
#define _FRONTENDS_P4_SPECIALIZE_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/// Describes how a parser or control is specialized.
struct SpecializationInfo {
    /// Name to use for specialized object.
    cstring name;
    /// Actual parser or control that is being specialized.
    const IR::IContainer*              specialized;
    /// values to substitute for type arguments.
    const IR::Vector<IR::Type>*        typeArguments;
    /// values to substitute for constructor arguments.
    IR::Vector<IR::Expression>*        constructorArguments;
    /// declarations to insert in the list of locals.
    IR::IndexedVector<IR::Declaration>* declarations;
    /// Invocation which causes this specialization.
    const IR::Node*                    invocation;
    /// Where in the program should the specialization be inserted.
    const IR::Node*                    insertBefore;

    SpecializationInfo(const IR::Node* invocation, const IR::IContainer* cont,
                       const IR::Node* insertion) :
            specialized(cont), typeArguments(nullptr),
            constructorArguments(new IR::Vector<IR::Expression>()),
            declarations(new IR::IndexedVector<IR::Declaration>()),
            invocation(invocation), insertBefore(insertion)
    { CHECK_NULL(cont); CHECK_NULL(invocation); CHECK_NULL(insertion); }
    const IR::Type_Declaration* synthesize(ReferenceMap* refMap) const;
};

/// Maintains a map from invocation to a SpecializationInfo object.
class SpecializationMap {
    /// Maps invocation to specialization info.
    ordered_map<const IR::Node*, SpecializationInfo*> specializations;
    const IR::Expression* convertArgument(const IR::Expression* arg, SpecializationInfo* info);

 public:
    TypeMap*      typeMap;
    ReferenceMap* refMap;
    /** Add a specialization instance.
     *
     * @param invocation The constructor invocation.
     * @param container The object (eg. parser or control) being specialized.
     * @param insertion Where the specialization should be inserted.
     */
    void addSpecialization(
        const IR::ConstructorCallExpression* invocation,
        const IR::IContainer* container,
        const IR::Node* insertion);
    /** Add a specialization instance.
     *
     * @param invocation The constructor invocation.
     * @param container The object (eg. parser or control) being specialized.
     * @param insertion Where the specialization should be inserted.
     */
    void addSpecialization(
        const IR::Declaration_Instance* invocation,
        const IR::IContainer* container,
        const IR::Node* insertion);
    IR::Vector<IR::Node>* getSpecializations(const IR::Node* insertion) const;
    cstring getName(const IR::Node* insertion) const {
        auto s = ::get(specializations, insertion);
        if (s == nullptr)
            return nullptr;
        return s->name;
    }
    void clear() { specializations.clear(); }
};

/// Builds a SpecializationMap.
class FindSpecializations : public Inspector {
    SpecializationMap* specMap;

 public:
    explicit FindSpecializations(SpecializationMap* specMap) : specMap(specMap) {
        CHECK_NULL(specMap);
        setName("FindSpecializations");
    }

    const IR::Node* findInsertionPoint() const;
    bool isSimpleConstant(const IR::Expression* expression) const;
    Visitor::profile_t init_apply(const IR::Node* node) override
    { specMap->clear(); return Inspector::init_apply(node); }
    // True if this container does not have constructor or type
    // parameters i.e., we can look inside for invocations to
    // specialize.
    bool noParameters(const IR::IContainer* container);

    bool preorder(const IR::P4Parser* parser) override
    { return noParameters(parser); }
    bool preorder(const IR::P4Control* control) override
    { return noParameters(control); }
    void postorder(const IR::ConstructorCallExpression* expression) override;
    void postorder(const IR::Declaration_Instance* decl) override;
};

/** @brief Specializes each Parser and Control by substituting type arguments
 * and constructor parameters.
 *
 * Specifically, each instantiation of each Parser (or Control) with
 * constructor parameters will cause
 *  - the creation of a new Parser (Control) with the actual arguments
 *    substituted for the constructor parameters, and
 *  - the replacement of the instantiation with a new instantiation using the
 *    newly-specialized Parser (or Control).
 *
 * Note that this pass handles type substition for instantiating generic Parser
 * or Control types, which is an experimental feature.
 * 
 * For example:
 *
```
control c<T>(in T data)(bit<32> size) { ... }
c<bit<32>>(16) c_inst;
```
 *
 * is converted to
 *
```
control cspec(in bit<32> data) { ... }
cspec() c_inst;
```
 *
 * A different specialization is made for each constructor invocation
 * and Declaration_Instance.
 *
 * @pre Actual arguments are suitable for substitution, meaning that it must
 * run after ConstantFolding and SimplifyExpressions.
 *
 * @post No instantiations of Parsers or Controls with constructor parameters
 * remain.
 */
class Specialize : public Transform {
    SpecializationMap* specMap;
    const IR::Node* instantiate(const IR::Node* node);
 public:
    explicit Specialize(SpecializationMap* specMap) : specMap(specMap)
    { CHECK_NULL(specMap); setName("Specialize"); }
    const IR::Node* postorder(IR::P4Parser* parser) override
    { return instantiate(parser); }
    const IR::Node* postorder(IR::P4Control* control) override
    { return instantiate(control); }
    const IR::Node* postorder(IR::ConstructorCallExpression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance*) override;
};

/** @brief Repeats ConstantFolding, TypeChecking, FindSpecializations, and
 * Specialize followed by RemoveAllUnusedDeclarations until convergence.
 *
 * The primary goal of this pass is to specialize Control and Parser types with
 * instantiations that supply constructor parameters, which is accomplished via
 * Specialize.
 *
 * Specialization may also  enable new opportunities for ConstantFolding (after
 * actual arguments are substituted for constructor parameters) and
 * RemoveAllUnusedDeclarations (after all instantiations of Control and Parser
 * types with constructors are replaced).
 *
 * @pre See preconditions for the passes SpecializeAll invokes.
 * @post See postconditions for the passes SpecializeAll invokes.
 */
class SpecializeAll : public PassRepeated {
    SpecializationMap specMap;
 public:
    SpecializeAll(ReferenceMap* refMap, TypeMap* typeMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SPECIALIZE_H_ */
