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
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"

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

template <typename... Args>
void typeError(const char *format, Args &&...args) {
    ::P4::error(ErrorType::ERR_TYPE_ERROR, format, std::forward<Args>(args)...);
}
/// True if the type contains any varbit or header_union subtypes
bool hasVarbitsOrUnions(const TypeMap *typeMap, const IR::Type *type);

class ReadOnlyTypeInference;

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
class TypeInferenceBase : public virtual Visitor, public ResolutionContext {
    // Output: type map
    TypeMap *typeMap;
    const IR::Node *initialNode;
    std::shared_ptr<MinimalNameGenerator> nameGen;

 public:
    // @param readOnly If true it will assert that it behaves like
    //        an Inspector.
    explicit TypeInferenceBase(TypeMap *typeMap, bool readOnly = false, bool checkArrays = true,
                               bool errorOnNullDecls = false);

 protected:
    TypeInferenceBase(TypeMap *typeMap, std::shared_ptr<MinimalNameGenerator> nameGen);

    // If true we expect to leave the program unchanged
    bool readOnly = false;
    bool checkArrays = true;
    bool errorOnNullDecls = false;
    const IR::Node *getInitialNode() const { return initialNode; }
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
                                        std::string_view errorFormat,
                                        std::initializer_list<const IR::Node *> errorArgs);

    /// Unifies two types.  Returns nullptr if unification fails.
    /// Populates the typeMap with values for the type variables.
    /// This allows an implicit cast from the right type to the left type.
    TypeVariableSubstitution *unifyCast(const IR::Node *errorPosition, const IR::Type *destType,
                                        const IR::Type *srcType, std::string_view errorFormat = {},
                                        std::initializer_list<const IR::Node *> errorArgs = {}) {
        return unifyBase(true, errorPosition, destType, srcType, errorFormat, errorArgs);
    }
    /// Same as above, not allowing casts
    TypeVariableSubstitution *unify(const IR::Node *errorPosition, const IR::Type *destType,
                                    const IR::Type *srcType, std::string_view errorFormat = {},
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
    template <class Ctor>
    const IR::Type *canonicalizeFields(const IR::Type_StructLike *type, Ctor constructor);
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

    /// Check if the underlying type for enum is bit<> or int<> and emit error if it is not.
    /// @returns the resolved type, or nullptr if the type is invalid
    const IR::Type_Bits *checkUnderlyingEnumType(const IR::Type *enumType);

    //////////////////////////////////////////////////////////////

 public:
    static const IR::Type *specialize(const IR::IMayBeGenericType *type,
                                      const IR::Vector<IR::Type> *arguments,
                                      const Visitor::Context *ctxt);

    struct Comparison {
        const IR::Expression *left;
        const IR::Expression *right;
    };

    using PreorderResult = std::pair<const IR::Node *, bool>;

    // Helper function to handle comparisons
    bool compare(const IR::Node *errorPosition, const IR::Type *ltype, const IR::Type *rtype,
                 Comparison *compare);

    // do functions pre-order so we can check the prototype
    // before the returns
    PreorderResult preorder(const IR::Function *function);
    PreorderResult preorder(const IR::P4Program *program);
    PreorderResult preorder(const IR::Declaration_Instance *decl);
    // check invariants for entire list before checking the entries
    PreorderResult preorder(const IR::EntriesList *el);
    PreorderResult preorder(const IR::Type_SerEnum *type);

    const IR::Node *postorder(const IR::Declaration_MatchKind *decl);
    const IR::Node *postorder(const IR::Declaration_Variable *decl);
    const IR::Node *postorder(const IR::Declaration_Constant *constant);
    const IR::Node *postorder(const IR::P4Control *cont);
    const IR::Node *postorder(const IR::P4Parser *cont);
    const IR::Node *postorder(const IR::Method *method);

    const IR::Node *postorder(const IR::Type_Type *type);
    const IR::Node *postorder(const IR::Type_Table *type);
    const IR::Node *postorder(const IR::Type_Error *decl);
    const IR::Node *postorder(const IR::Type_InfInt *type);
    const IR::Node *postorder(const IR::Type_Method *type);
    const IR::Node *postorder(const IR::Type_Action *type);
    const IR::Node *postorder(const IR::Type_Name *type);
    const IR::Node *postorder(const IR::Type_Base *type);
    const IR::Node *postorder(const IR::Type_Var *type);
    const IR::Node *postorder(const IR::Type_Enum *type);
    const IR::Node *postorder(const IR::Type_Extern *type);
    const IR::Node *postorder(const IR::StructField *field);
    const IR::Node *postorder(const IR::Type_Header *type);
    const IR::Node *postorder(const IR::Type_Stack *type);
    const IR::Node *postorder(const IR::Type_Struct *type);
    const IR::Node *postorder(const IR::Type_HeaderUnion *type);
    const IR::Node *postorder(const IR::Type_Typedef *type);
    const IR::Node *postorder(const IR::Type_Specialized *type);
    const IR::Node *postorder(const IR::Type_SpecializedCanonical *type);
    const IR::Node *postorder(const IR::Type_Tuple *type);
    const IR::Node *postorder(const IR::Type_P4List *type);
    const IR::Node *postorder(const IR::Type_List *type);
    const IR::Node *postorder(const IR::Type_Set *type);
    const IR::Node *postorder(const IR::Type_ArchBlock *type);
    const IR::Node *postorder(const IR::Type_Newtype *type);
    const IR::Node *postorder(const IR::Type_Package *type);
    const IR::Node *postorder(const IR::Type_ActionEnum *type);
    const IR::Node *postorder(const IR::P4Table *type);
    const IR::Node *postorder(const IR::P4Action *type);
    const IR::Node *postorder(const IR::P4ValueSet *type);
    const IR::Node *postorder(const IR::Key *key);
    const IR::Node *postorder(const IR::Entry *e);

    const IR::Node *postorder(const IR::Dots *expression);
    const IR::Node *postorder(const IR::Argument *arg);
    const IR::Node *postorder(const IR::SerEnumMember *member);
    const IR::Node *postorder(const IR::Parameter *param);
    const IR::Node *postorder(const IR::Constant *expression);
    const IR::Node *postorder(const IR::BoolLiteral *expression);
    const IR::Node *postorder(const IR::StringLiteral *expression);
    const IR::Node *postorder(const IR::Operation_Relation *expression);
    const IR::Node *postorder(const IR::Concat *expression);
    const IR::Node *postorder(const IR::ArrayIndex *expression);
    const IR::Node *postorder(const IR::LAnd *expression) { return binaryBool(expression); }
    const IR::Node *postorder(const IR::LOr *expression) { return binaryBool(expression); }
    const IR::Node *postorder(const IR::Add *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::Sub *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::AddSat *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::SubSat *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::Mul *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::Div *expression) { return unsBinaryArith(expression); }
    const IR::Node *postorder(const IR::Mod *expression) { return unsBinaryArith(expression); }
    const IR::Node *postorder(const IR::Shl *expression) { return shift(expression); }
    const IR::Node *postorder(const IR::Shr *expression) { return shift(expression); }
    const IR::Node *postorder(const IR::BXor *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::BAnd *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::BOr *expression) { return binaryArith(expression); }
    const IR::Node *postorder(const IR::Mask *expression) { return typeSet(expression); }
    const IR::Node *postorder(const IR::Range *expression) { return typeSet(expression); }
    const IR::Node *postorder(const IR::LNot *expression);
    const IR::Node *postorder(const IR::Neg *expression);
    const IR::Node *postorder(const IR::UPlus *expression);
    const IR::Node *postorder(const IR::Cmpl *expression);
    const IR::Node *postorder(const IR::Cast *expression);
    const IR::Node *postorder(const IR::Mux *expression);
    const IR::Node *postorder(const IR::Slice *expression);
    const IR::Node *postorder(const IR::PathExpression *expression);
    const IR::Node *postorder(const IR::Member *expression);
    const IR::Node *postorder(const IR::TypeNameExpression *expression);
    const IR::Node *postorder(const IR::ListExpression *expression);
    const IR::Node *postorder(const IR::InvalidHeader *expression);
    const IR::Node *postorder(const IR::InvalidHeaderUnion *expression);
    const IR::Node *postorder(const IR::Invalid *expression);
    const IR::Node *postorder(const IR::P4ListExpression *expression);
    const IR::Node *postorder(const IR::StructExpression *expression);
    const IR::Node *postorder(const IR::HeaderStackExpression *expression);
    const IR::Node *postorder(const IR::MethodCallStatement *mcs);
    const IR::Node *postorder(const IR::MethodCallExpression *expression);
    const IR::Node *postorder(const IR::ConstructorCallExpression *expression);
    const IR::Node *postorder(const IR::SelectExpression *expression);
    const IR::Node *postorder(const IR::DefaultExpression *expression);
    const IR::Node *postorder(const IR::This *expression);
    const IR::Node *postorder(const IR::AttribLocal *local);
    const IR::Node *postorder(const IR::ActionList *al);

    const IR::Node *postorder(const IR::ReturnStatement *stat);
    const IR::Node *postorder(const IR::IfStatement *stat);
    const IR::Node *postorder(const IR::SwitchStatement *stat);
    const IR::Node *postorder(const IR::AssignmentStatement *stat);
    const IR::Node *postorder(const IR::ForInStatement *stat);
    const IR::Node *postorder(const IR::ActionListElement *elem);
    const IR::Node *postorder(const IR::KeyElement *elem);
    const IR::Node *postorder(const IR::Property *elem);
    const IR::Node *postorder(const IR::SelectCase *elem);
    const IR::Node *postorder(const IR::Annotation *annotation);

    void start(const IR::Node *node);
    void finish(const IR::Node *Node);

    ReadOnlyTypeInference *readOnlyClone() const;
    // Apply recursively the typechecker to the newly created node
    // to add all component subtypes in the typemap.
    // Return 'true' if errors were discovered in the learning process.
    bool learn(const IR::Node *node, Visitor *caller, const Visitor::Context *ctxt);
};

class ReadOnlyTypeInference : public virtual Inspector, public TypeInferenceBase {
    friend class TypeInferenceBase;

    ReadOnlyTypeInference(TypeMap *typeMap, std::shared_ptr<MinimalNameGenerator> nameGen)
        : TypeInferenceBase(typeMap, std::move(nameGen)) {}

 public:
    using Inspector::postorder;
    using Inspector::preorder;

    explicit ReadOnlyTypeInference(TypeMap *typeMap, bool checkArrays = true,
                                   bool errorOnNullDecls = false)
        : TypeInferenceBase(typeMap, true, checkArrays, errorOnNullDecls) {}

    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *Node) override;

    bool preorder(const IR::Expression *) override { return !done(); }
    bool preorder(const IR::Type *) override { return !done(); }

    // do functions pre-order so we can check the prototype
    // before the returns
    bool preorder(const IR::Function *function) override;
    bool preorder(const IR::P4Program *program) override;
    bool preorder(const IR::Declaration_Instance *decl) override;
    // check invariants for entire list before checking the entries
    bool preorder(const IR::EntriesList *el) override;
    bool preorder(const IR::Type_SerEnum *type) override;

    void postorder(const IR::Declaration_MatchKind *decl) override;
    void postorder(const IR::Declaration_Variable *decl) override;
    void postorder(const IR::Declaration_Constant *constant) override;
    void postorder(const IR::P4Control *cont) override;
    void postorder(const IR::P4Parser *cont) override;
    void postorder(const IR::Method *method) override;

    void postorder(const IR::Type_Type *type) override;
    void postorder(const IR::Type_Table *type) override;
    void postorder(const IR::Type_Error *decl) override;
    void postorder(const IR::Type_InfInt *type) override;
    void postorder(const IR::Type_Method *type) override;
    void postorder(const IR::Type_Action *type) override;
    void postorder(const IR::Type_Name *type) override;
    void postorder(const IR::Type_Base *type) override;
    void postorder(const IR::Type_Var *type) override;
    void postorder(const IR::Type_Enum *type) override;
    void postorder(const IR::Type_Extern *type) override;
    void postorder(const IR::StructField *field) override;
    void postorder(const IR::Type_Header *type) override;
    void postorder(const IR::Type_Stack *type) override;
    void postorder(const IR::Type_Struct *type) override;
    void postorder(const IR::Type_HeaderUnion *type) override;
    void postorder(const IR::Type_Typedef *type) override;
    void postorder(const IR::Type_Specialized *type) override;
    void postorder(const IR::Type_SpecializedCanonical *type) override;
    void postorder(const IR::Type_Tuple *type) override;
    void postorder(const IR::Type_P4List *type) override;
    void postorder(const IR::Type_List *type) override;
    void postorder(const IR::Type_Set *type) override;
    void postorder(const IR::Type_ArchBlock *type) override;
    void postorder(const IR::Type_Newtype *type) override;
    void postorder(const IR::Type_Package *type) override;
    void postorder(const IR::Type_ActionEnum *type) override;
    void postorder(const IR::P4Table *type) override;
    void postorder(const IR::P4Action *type) override;
    void postorder(const IR::P4ValueSet *type) override;
    void postorder(const IR::Key *key) override;
    void postorder(const IR::Entry *e) override;

    void postorder(const IR::Dots *expression) override;
    void postorder(const IR::Argument *arg) override;
    void postorder(const IR::SerEnumMember *member) override;
    void postorder(const IR::Parameter *param) override;
    void postorder(const IR::Constant *expression) override;
    void postorder(const IR::BoolLiteral *expression) override;
    void postorder(const IR::StringLiteral *expression) override;
    void postorder(const IR::Operation_Relation *expression) override;
    void postorder(const IR::Concat *expression) override;
    void postorder(const IR::ArrayIndex *expression) override;
    void postorder(const IR::LAnd *expression) override;
    void postorder(const IR::LOr *expression) override;
    void postorder(const IR::Add *expression) override;
    void postorder(const IR::Sub *expression) override;
    void postorder(const IR::AddSat *expression) override;
    void postorder(const IR::SubSat *expression) override;
    void postorder(const IR::Mul *expression) override;
    void postorder(const IR::Div *expression) override;
    void postorder(const IR::Mod *expression) override;
    void postorder(const IR::Shl *expression) override;
    void postorder(const IR::Shr *expression) override;
    void postorder(const IR::BXor *expression) override;
    void postorder(const IR::BAnd *expression) override;
    void postorder(const IR::BOr *expression) override;
    void postorder(const IR::Mask *expression) override;
    void postorder(const IR::Range *expression) override;
    void postorder(const IR::LNot *expression) override;
    void postorder(const IR::Neg *expression) override;
    void postorder(const IR::UPlus *expression) override;
    void postorder(const IR::Cmpl *expression) override;
    void postorder(const IR::Cast *expression) override;
    void postorder(const IR::Mux *expression) override;
    void postorder(const IR::Slice *expression) override;
    void postorder(const IR::PathExpression *expression) override;
    void postorder(const IR::Member *expression) override;
    void postorder(const IR::TypeNameExpression *expression) override;
    void postorder(const IR::ListExpression *expression) override;
    void postorder(const IR::InvalidHeader *expression) override;
    void postorder(const IR::InvalidHeaderUnion *expression) override;
    void postorder(const IR::Invalid *expression) override;
    void postorder(const IR::P4ListExpression *expression) override;
    void postorder(const IR::StructExpression *expression) override;
    void postorder(const IR::HeaderStackExpression *expression) override;
    void postorder(const IR::MethodCallStatement *mcs) override;
    void postorder(const IR::MethodCallExpression *expression) override;
    void postorder(const IR::ConstructorCallExpression *expression) override;
    void postorder(const IR::SelectExpression *expression) override;
    void postorder(const IR::DefaultExpression *expression) override;
    void postorder(const IR::This *expression) override;
    void postorder(const IR::AttribLocal *local) override;
    void postorder(const IR::ActionList *al) override;

    void postorder(const IR::ReturnStatement *stat) override;
    void postorder(const IR::IfStatement *stat) override;
    void postorder(const IR::SwitchStatement *stat) override;
    void postorder(const IR::AssignmentStatement *stat) override;
    void postorder(const IR::ForInStatement *stat) override;
    void postorder(const IR::ActionListElement *elem) override;
    void postorder(const IR::KeyElement *elem) override;
    void postorder(const IR::Property *elem) override;
    void postorder(const IR::SelectCase *elem) override;
    void postorder(const IR::Annotation *annotation) override;
};

class TypeInference : public virtual Transform, public TypeInferenceBase {
 public:
    using Transform::postorder;
    using Transform::preorder;

    explicit TypeInference(TypeMap *typeMap, bool readOnly = true, bool checkArrays = true,
                           bool errorOnNullDecls = false)
        : TypeInferenceBase(typeMap, readOnly, checkArrays, errorOnNullDecls) {}

    Visitor::profile_t init_apply(const IR::Node *node) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = nullptr) override;
    void end_apply(const IR::Node *Node) override;

    const IR::Node *pruneIfDone(const IR::Node *node) {
        if (done()) Transform::prune();
        return node;
    }
    const IR::Node *preorder(IR::Expression *expression) override {
        return pruneIfDone(expression);
    }
    const IR::Node *preorder(IR::Type *type) override { return pruneIfDone(type); }

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
    const IR::Node *postorder(IR::LAnd *expression) override;
    const IR::Node *postorder(IR::LOr *expression) override;
    const IR::Node *postorder(IR::Add *expression) override;
    const IR::Node *postorder(IR::Sub *expression) override;
    const IR::Node *postorder(IR::AddSat *expression) override;
    const IR::Node *postorder(IR::SubSat *expression) override;
    const IR::Node *postorder(IR::Mul *expression) override;
    const IR::Node *postorder(IR::Div *expression) override;
    const IR::Node *postorder(IR::Mod *expression) override;
    const IR::Node *postorder(IR::Shl *expression) override;
    const IR::Node *postorder(IR::Shr *expression) override;
    const IR::Node *postorder(IR::BXor *expression) override;
    const IR::Node *postorder(IR::BAnd *expression) override;
    const IR::Node *postorder(IR::BOr *expression) override;
    const IR::Node *postorder(IR::Mask *expression) override;
    const IR::Node *postorder(IR::Range *expression) override;
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
    const IR::Node *postorder(IR::MethodCallStatement *mcs) override;
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
    const IR::Node *postorder(IR::ForInStatement *stat) override;
    const IR::Node *postorder(IR::ActionListElement *elem) override;
    const IR::Node *postorder(IR::KeyElement *elem) override;
    const IR::Node *postorder(IR::Property *elem) override;
    const IR::Node *postorder(IR::SelectCase *elem) override;
    const IR::Node *postorder(IR::Annotation *annotation) override;
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
