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

#ifndef TYPECHECKING_TYPECHECKER_H_
#define TYPECHECKING_TYPECHECKER_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeSubstitution.h"
#include "frontends/p4/typeChecking/typeSubstitutionVisitor.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "typeUnification.h"

namespace P4 {

// This pass only clears the typeMap if the program has changed
// or the 'force' flag is set.
// This is needed if the types of some objects in the program change.
class ClearTypeMap : public Inspector {
    TypeMap *typeMap;
    bool force;

 public:
    explicit ClearTypeMap(TypeMap *typeMap, bool force = false) : typeMap(typeMap), force(force) {
        CHECK_NULL(typeMap);
    }
    bool preorder(const IR::P4Program *program) override {
        // Clear map only if program has not changed from last time
        // otherwise we can reuse it.  The 'force' flag is needed
        // because the program is saved only *after* typechecking,
        // so if the program changes during type-checking, the
        // typeMap may not be complete.
        if (force || !typeMap->checkMap(program)) typeMap->clear();
        return false;  // prune()
    }
};

/// Performs together reference resolution and type checking by calling
/// TypeInference.  If updateExpressions is true, after type checking
/// it will update all Expression objects, writing the result type into
/// the Expression::type field.
class TypeChecking : public PassManager {
 public:
    TypeChecking(/* out */ ReferenceMap *refMap, /* out */ TypeMap *typeMap,
                 bool updateExpressions = false);
};

template <typename... T>
void typeError(const char *format, T... args) {
    ::error(ErrorType::ERR_TYPE_ERROR, format, args...);
}
/// True if the type contains any varbit or header_union subtypes
bool hasVarbitsOrUnions(const TypeMap *typeMap, const IR::Type *type);

// Actual type checking algorithm.
// In general this pass should not be called directly; call TypeChecking instead.
// It is a transform because it may convert implicit casts into explicit casts.
// But in general it operates like an Inspector; in fact, if it is instantiated
// with readOnly = true, it will assert that the program is not changed.
// It is expected that once a program has been type-checked and all casts have
// been inserted it will not need to change ever again during type-checking.
// In fact, several passes do modify the program such that types are invalidated.
// For example, enum elimination converts enum values into integers.  After such
// changes the typemap has to be cleared and types must be recomputed from scratch.
class TypeInference : public Transform {
    // Input: reference map
    ReferenceMap *refMap;
    // Output: type map
    TypeMap *typeMap;
    const IR::Node *initialNode;

 public:
    // @param readOnly If true it will assert that it behaves like
    //        an Inspector.
    TypeInference(ReferenceMap *refMap, TypeMap *typeMap, bool readOnly = false,
                  bool checkArrays = true);

 protected:
    // If true we expect to leave the program unchanged
    bool readOnly;
    bool checkArrays = true;
    const IR::Type *getType(const IR::Node *element) const;
    const IR::Type *getTypeType(const IR::Node *element) const;
    void setType(const IR::Node *element, const IR::Type *type);
    void setLeftValue(const IR::Expression *expression) { typeMap->setLeftValue(expression); }
    bool isLeftValue(const IR::Expression *expression) const {
        return typeMap->isLeftValue(expression) || expression->is<IR::DefaultExpression>();
    }
    void setCompileTimeConstant(const IR::Expression *expression) {
        typeMap->setCompileTimeConstant(expression);
    }
    bool isCompileTimeConstant(const IR::Expression *expression) const {
        return typeMap->isCompileTimeConstant(expression);
    }

    // This is needed because sometimes we invoke visitors recursively on subtrees explicitly.
    // (visitDagOnce cannot take care of this).
    bool done() const;

    TypeVariableSubstitution *unifyBase(bool allowCasts, const IR::Node *errorPosition,
                                        const IR::Type *destType, const IR::Type *srcType,
                                        cstring errorFormat,
                                        std::initializer_list<const IR::Node *> errorArgs);

    /// Unifies two types.  Returns nullptr if unification fails.
    /// Populates the typeMap with values for the type variables.
    /// This allows an implicit cast from the right type to the left type.
    TypeVariableSubstitution *unifyCast(const IR::Node *errorPosition, const IR::Type *destType,
                                        const IR::Type *srcType, cstring errorFormat = nullptr,
                                        std::initializer_list<const IR::Node *> errorArgs = {}) {
        return unifyBase(true, errorPosition, destType, srcType, errorFormat, errorArgs);
    }
    /// Same as above, not allowing casts
    TypeVariableSubstitution *unify(const IR::Node *errorPosition, const IR::Type *destType,
                                    const IR::Type *srcType, cstring errorFormat = nullptr,
                                    std::initializer_list<const IR::Node *> errorArgs = {}) {
        return unifyBase(false, errorPosition, destType, srcType, errorFormat, errorArgs);
    }

    /** Tries to assign sourceExpression to a destination with type destType.
        This may rewrite the sourceExpression, in particular converting InfInt values
        to values with concrete types.
        @returns new sourceExpression. */
    const IR::Expression *assignment(const IR::Node *errorPosition, const IR::Type *destType,
                                     const IR::Expression *sourceExpression);
    const IR::SelectCase *matchCase(const IR::SelectExpression *select,
                                    const IR::Type_BaseList *selectType,
                                    const IR::SelectCase *selectCase, const IR::Type *caseType);
    bool canCastBetween(const IR::Type *dest, const IR::Type *src) const;
    bool checkAbstractMethods(const IR::Declaration_Instance *inst, const IR::Type_Extern *type);
    void addSubstitutions(const TypeVariableSubstitution *tvs);

    const IR::Expression *constantFold(const IR::Expression *expression);

    /** Converts each type to a canonical representation.
     *  Made virtual to enable private midend passes to extend standard IR with custom IR classes.
     */
    virtual const IR::Type *canonicalize(const IR::Type *type);
    const IR::Type *canonicalizeFields(
        const IR::Type_StructLike *type,
        std::function<const IR::Type *(const IR::IndexedVector<IR::StructField> *)> constructor);
    virtual const IR::ParameterList *canonicalizeParameters(const IR::ParameterList *params);

    // various helpers
    bool onlyBitsOrBitStructs(const IR::Type *type) const;
    bool containsHeader(const IR::Type *canonType);
    bool validateFields(const IR::Type *type, std::function<bool(const IR::Type *)> checker) const;
    const IR::Node *binaryBool(const IR::Operation_Binary *op);
    const IR::Node *binaryArith(const IR::Operation_Binary *op);
    const IR::Node *unsBinaryArith(const IR::Operation_Binary *op);
    const IR::Node *shift(const IR::Operation_Binary *op);
    const IR::Node *typeSet(const IR::Operation_Binary *op);

    const IR::Type *cloneWithFreshTypeVariables(const IR::IMayBeGenericType *type);
    std::pair<const IR::Type *, const IR::Vector<IR::Argument> *> containerInstantiation(
        const IR::Node *node, const IR::Vector<IR::Argument> *args,
        const IR::IContainer *container);
    const IR::Expression *actionCall(
        bool inActionList,  // if true this "call" is in the action list of a table
        const IR::MethodCallExpression *actionCall);
    std::pair<const IR::Type *, const IR::Vector<IR::Argument> *> checkExternConstructor(
        const IR::Node *errorPosition, const IR::Type_Extern *ext,
        const IR::Vector<IR::Argument> *arguments);

    static constexpr bool forbidModules = true;
    static constexpr bool forbidPackages = true;
    bool checkParameters(const IR::ParameterList *paramList, bool forbidModules = false,
                         bool forbidPackage = false) const;
    virtual const IR::Type *setTypeType(const IR::Type *type, bool learn = true);

    /// Action list of the current table.
    const IR::ActionList *currentActionList;
    /// This is used to validate the initializer for the default_action
    /// or for actions in the entries list.  Returns the action list element
    /// on success.
    const IR::ActionListElement *validateActionInitializer(const IR::Expression *actionCall);
    bool containsActionEnum(const IR::Type *type) const;

    //////////////////////////////////////////////////////////////

 public:
    using Transform::postorder;
    using Transform::preorder;

    static const IR::Type *specialize(const IR::IMayBeGenericType *type,
                                      const IR::Vector<IR::Type> *arguments);
    const IR::Node *pruneIfDone(const IR::Node *node) {
        if (done()) {
            prune();
        }
        return node;
    }
    const IR::Node *preorder(IR::Expression *expression) override {
        return pruneIfDone(expression);
    }
    const IR::Node *preorder(IR::Type *type) override { return pruneIfDone(type); }

    struct Comparison {
        const IR::Expression *left;
        const IR::Expression *right;
    };

    // Helper function to handle comparisons
    bool compare(const IR::Node *errorPosition, const IR::Type *ltype, const IR::Type *rtype,
                 Comparison *compare);

    // do functions pre-order so we can check the prototype
    // before the returns
    const IR::Node *preorder(IR::Function *function) override;
    const IR::Node *preorder(IR::P4Program *program) override;
    const IR::Node *preorder(IR::Declaration_Instance *decl) override;
    // check invariants for entire list before checking the entries
    const IR::Node *preorder(IR::EntriesList *el) override;
    const IR::Node *preorder(IR::Type_SerEnum *type) override;

    const IR::Node *postorder(IR::Declaration_MatchKind *decl) override;
    const IR::Node *postorder(IR::Declaration_Variable *decl) override;
    const IR::Node *postorder(IR::Declaration_Constant *constant) override;
    const IR::Node *postorder(IR::P4Control *cont) override;
    const IR::Node *postorder(IR::P4Parser *cont) override;
    const IR::Node *postorder(IR::Method *method) override;

    const IR::Node *postorder(IR::Type_Type *type) override;
    const IR::Node *postorder(IR::Type_Table *type) override;
    const IR::Node *postorder(IR::Type_Error *decl) override;
    const IR::Node *postorder(IR::Type_InfInt *type) override;
    const IR::Node *postorder(IR::Type_Method *type) override;
    const IR::Node *postorder(IR::Type_Action *type) override;
    const IR::Node *postorder(IR::Type_Name *type) override;
    const IR::Node *postorder(IR::Type_Base *type) override;
    const IR::Node *postorder(IR::Type_Var *type) override;
    const IR::Node *postorder(IR::Type_Enum *type) override;
    const IR::Node *postorder(IR::Type_Extern *type) override;
    const IR::Node *postorder(IR::StructField *field) override;
    const IR::Node *postorder(IR::Type_Header *type) override;
    const IR::Node *postorder(IR::Type_Stack *type) override;
    const IR::Node *postorder(IR::Type_Struct *type) override;
    const IR::Node *postorder(IR::Type_HeaderUnion *type) override;
    const IR::Node *postorder(IR::Type_Typedef *type) override;
    const IR::Node *postorder(IR::Type_Specialized *type) override;
    const IR::Node *postorder(IR::Type_SpecializedCanonical *type) override;
    const IR::Node *postorder(IR::Type_Tuple *type) override;
    const IR::Node *postorder(IR::Type_P4List *type) override;
    const IR::Node *postorder(IR::Type_List *type) override;
    const IR::Node *postorder(IR::Type_Set *type) override;
    const IR::Node *postorder(IR::Type_ArchBlock *type) override;
    const IR::Node *postorder(IR::Type_Newtype *type) override;
    const IR::Node *postorder(IR::Type_Package *type) override;
    const IR::Node *postorder(IR::Type_ActionEnum *type) override;
    const IR::Node *postorder(IR::P4Table *type) override;
    const IR::Node *postorder(IR::P4Action *type) override;
    const IR::Node *postorder(IR::P4ValueSet *type) override;
    const IR::Node *postorder(IR::Key *key) override;
    const IR::Node *postorder(IR::Entry *e) override;

    const IR::Node *postorder(IR::Dots *expression) override;
    const IR::Node *postorder(IR::Argument *arg) override;
    const IR::Node *postorder(IR::SerEnumMember *member) override;
    const IR::Node *postorder(IR::Parameter *param) override;
    const IR::Node *postorder(IR::Constant *expression) override;
    const IR::Node *postorder(IR::BoolLiteral *expression) override;
    const IR::Node *postorder(IR::StringLiteral *expression) override;
    const IR::Node *postorder(IR::Operation_Relation *expression) override;
    const IR::Node *postorder(IR::Concat *expression) override;
    const IR::Node *postorder(IR::ArrayIndex *expression) override;
    const IR::Node *postorder(IR::LAnd *expression) override { return binaryBool(expression); }
    const IR::Node *postorder(IR::LOr *expression) override { return binaryBool(expression); }
    const IR::Node *postorder(IR::Add *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::Sub *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::AddSat *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::SubSat *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::Mul *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::Div *expression) override { return unsBinaryArith(expression); }
    const IR::Node *postorder(IR::Mod *expression) override { return unsBinaryArith(expression); }
    const IR::Node *postorder(IR::Shl *expression) override { return shift(expression); }
    const IR::Node *postorder(IR::Shr *expression) override { return shift(expression); }
    const IR::Node *postorder(IR::BXor *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::BAnd *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::BOr *expression) override { return binaryArith(expression); }
    const IR::Node *postorder(IR::Mask *expression) override { return typeSet(expression); }
    const IR::Node *postorder(IR::Range *expression) override { return typeSet(expression); }
    const IR::Node *postorder(IR::LNot *expression) override;
    const IR::Node *postorder(IR::Neg *expression) override;
    const IR::Node *postorder(IR::UPlus *expression) override;
    const IR::Node *postorder(IR::Cmpl *expression) override;
    const IR::Node *postorder(IR::Cast *expression) override;
    const IR::Node *postorder(IR::Mux *expression) override;
    const IR::Node *postorder(IR::Slice *expression) override;
    const IR::Node *postorder(IR::PathExpression *expression) override;
    const IR::Node *postorder(IR::Member *expression) override;
    const IR::Node *postorder(IR::TypeNameExpression *expression) override;
    const IR::Node *postorder(IR::ListExpression *expression) override;
    const IR::Node *postorder(IR::InvalidHeader *expression) override;
    const IR::Node *postorder(IR::InvalidHeaderUnion *expression) override;
    const IR::Node *postorder(IR::Invalid *expression) override;
    const IR::Node *postorder(IR::P4ListExpression *expression) override;
    const IR::Node *postorder(IR::StructExpression *expression) override;
    const IR::Node *postorder(IR::HeaderStackExpression *expression) override;
    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
    const IR::Node *postorder(IR::ConstructorCallExpression *expression) override;
    const IR::Node *postorder(IR::SelectExpression *expression) override;
    const IR::Node *postorder(IR::DefaultExpression *expression) override;
    const IR::Node *postorder(IR::This *expression) override;
    const IR::Node *postorder(IR::AttribLocal *local) override;
    const IR::Node *postorder(IR::ActionList *al) override;

    const IR::Node *postorder(IR::ReturnStatement *stat) override;
    const IR::Node *postorder(IR::IfStatement *stat) override;
    const IR::Node *postorder(IR::SwitchStatement *stat) override;
    const IR::Node *postorder(IR::AssignmentStatement *stat) override;
    const IR::Node *postorder(IR::ActionListElement *elem) override;
    const IR::Node *postorder(IR::KeyElement *elem) override;
    const IR::Node *postorder(IR::Property *elem) override;
    const IR::Node *postorder(IR::SelectCase *elem) override;
    const IR::Node *postorder(IR::Annotation *annotation) override;

    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *Node) override;

    TypeInference *clone() const override;
    // Apply recursively the typechecker to the newly created node
    // to add all component subtypes in the typemap.
    // Return 'true' if errors were discovered in the learning process.
    bool learn(const IR::Node *node, Visitor *caller);
};

// Copy types from the typeMap to expressions.  Updates the typeMap with newly created nodes
class ApplyTypesToExpressions : public Transform {
    TypeMap *typeMap;
    IR::Node *postorder(IR::Node *n) override {
        const IR::Node *orig = getOriginal();
        if (auto type = typeMap->getType(orig)) {
            if (*orig != *n) typeMap->setType(n, type);
        }
        return n;
    }
    IR::Expression *postorder(IR::Expression *e) override {
        auto orig = getOriginal<IR::Expression>();
        if (auto type = typeMap->getType(orig)) {
            e->type = type;
            if (*orig != *e) {
                typeMap->setType(e, type);
                if (typeMap->isLeftValue(orig)) typeMap->setLeftValue(e);
                if (typeMap->isCompileTimeConstant(orig)) typeMap->setCompileTimeConstant(e);
            }
        }
        return e;
    }

 public:
    explicit ApplyTypesToExpressions(TypeMap *typeMap) : typeMap(typeMap) {}
};

}  // namespace P4

#endif /* TYPECHECKING_TYPECHECKER_H_ */
