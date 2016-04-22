#ifndef _TYPECHECKING_TYPECHECKER_H_
#define _TYPECHECKING_TYPECHECKER_H_

#include "ir/ir.h"
#include "../../common/typeMap.h"
#include "../../common/resolveReferences/referenceMap.h"
#include "lib/exceptions.h"
#include "lib/cstring.h"
#include "ir/substitution.h"
#include "ir/substitutionVisitor.h"
#include "typeConstraints.h"
#include "typeUnification.h"

namespace P4 {

// Type checking algorithm.
// It is a transform because it may convert implicit casts into explicit casts.
// But in general it operates like an Inspector; in fact, if it is instantiated
// with readOnly = true, it will assert that the program is not changed.
// It is expected that once a program has been type-checked and all casts have
// been inserted it will not need to change ever again during type-checking.
class TypeChecker : public Transform {
    // Input: reference map
    ReferenceMap* refMap;
    // Output: type map
    TypeMap* typeMap;
    // If true clear the typeMap when starting
    bool clearMap;
    // If true we expect to leave the program unchanged
    bool readOnly;

    // Stack: Save here method arguments count on each method visit.
    // They are used in type resolution.
    std::vector<int> methodArguments;

    const IR::Node* initialNode;

 public:
    // If readOnly=true it will assert that it behaves like
    // an Inspector.
    // clearMap=true will clear the typeMap on start.
    TypeChecker(ReferenceMap* refMap, TypeMap* typeMap,
                bool clearMap = false, bool readOnly = false);

 protected:
    const IR::Type* getType(const IR::Node* element) const;
    void setType(const IR::Node* element, const IR::Type* type);
    void setLeftValue(const IR::Expression* expression)
    { typeMap->setLeftValue(expression); }
    bool isLeftValue(const IR::Expression* expression) const
    { return typeMap->isLeftValue(expression); }

    // This is needed because sometimes we invoke visitors recursively on subtrees explicitly.
    // (visitDagOnce cannot take care of this).
    bool done() const;
    IR::TypeVariableSubstitution* unify(
        const IR::Node* errorPosition, const IR::Type* destType,
        const IR::Type* srcType, bool reportErrors) const;

    // Tries to assign sourceExpression to a destination with type destType.
    // This may rewrite the sourceExpression, in particular converting InfInt values
    // to values with concrete types.  Returns new sourceExpression.
    const IR::Expression* assignment(const IR::Node* errorPosition, const IR::Type* destType,
                                     const IR::Expression* sourceExpression);
    const IR::SelectCase* matchCase(const IR::SelectExpression* select,
                                    const IR::Type_Tuple* selectType,
                                    const IR::SelectCase* selectCase,
                                    const IR::Type* caseType) const;
    bool canCastBetween(const IR::Type* dest, const IR::Type* src) const;

    // converts each type to a canonical pointer,
    // so we can check just pointer equality in the map
    const IR::Type* canonicalize(const IR::Type* type);
    const IR::NameMap<IR::StructField, ordered_map>*
            canonicalizeFields(const IR::Type_StructLike* type);
    const IR::ParameterList* canonicalize(const IR::ParameterList* params);
    const IR::TypeParameters* canonicalize(const IR::TypeParameters* params);

    // various helpers
    bool validateFields(const IR::Type_StructLike* type,
                        std::function<bool(const IR::Type*)> checker) const;
    const IR::Node* binaryBool(const IR::Operation_Binary* op);
    const IR::Node* binaryArith(const IR::Operation_Binary* op);
    const IR::Node* unsBinaryArith(const IR::Operation_Binary* op);
    const IR::Node* shift(const IR::Operation_Binary* op);
    const IR::Node* bitwise(const IR::Operation_Binary* op);
    const IR::Node* typeSet(const IR::Operation_Binary* op);
    const IR::Type* containerInstantiation(const IR::Node* node,
                                           const IR::Vector<IR::Expression>* args,
                                           const IR::IContainer* container);
    const IR::Type_Action* actionCallType(const IR::Type_Action* action,
                                          size_t removedArguments) const;
    const IR::Vector<IR::Expression>*
            checkExternConstructor(const IR::Node* errorPosition,
                                   const IR::Type_Extern* ext,
                                   const IR::Vector<IR::Expression> *arguments);

    //////////////////////////////////////////////////////////////

 public:
    using Transform::postorder;
    using Transform::preorder;

    const IR::Node* preorder(IR::Type* type) override;
    
    const IR::Node* postorder(IR::Declaration_MatchKind* decl) override;
    const IR::Node* postorder(IR::Declaration_Errors* decl) override;
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::Declaration_Constant* constant) override;
    const IR::Node* postorder(IR::Declaration_Instance* decl) override;
    const IR::Node* postorder(IR::P4Control* cont) override;
    const IR::Node* postorder(IR::P4Parser* cont) override;
    const IR::Node* postorder(IR::Method* method) override;

    const IR::Node* postorder(IR::Type_InfInt* type) override;
    const IR::Node* postorder(IR::Type_Method* type) override;
    const IR::Node* postorder(IR::Type_Action* type) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
    const IR::Node* postorder(IR::Type_Base* type) override;
    const IR::Node* postorder(IR::Type_Var* type) override;
    const IR::Node* postorder(IR::Type_Enum* type) override;
    const IR::Node* postorder(IR::Type_Extern* type) override;
    const IR::Node* postorder(IR::StructField* field) override;
    const IR::Node* postorder(IR::Type_Header* type) override;
    const IR::Node* postorder(IR::Type_Stack* type) override;
    const IR::Node* postorder(IR::Type_Struct* type) override;
    const IR::Node* postorder(IR::Type_Union* type) override;
    const IR::Node* postorder(IR::Type_Typedef* type) override;
    const IR::Node* postorder(IR::Type_Specialized* type) override;
    const IR::Node* postorder(IR::Type_SpecializedCanonical* type) override;
    const IR::Node* postorder(IR::Type_Tuple* type) override;
    const IR::Node* postorder(IR::Type_ArchBlock* type) override;
    const IR::Node* postorder(IR::Type_Package* type) override;
    const IR::Node* postorder(IR::Type_ActionEnum* type) override;
    const IR::Node* postorder(IR::P4Table* type) override;
    const IR::Node* postorder(IR::P4Action* type) override;

    const IR::Node* postorder(IR::Parameter* param) override;
    const IR::Node* postorder(IR::Constant* expression) override;
    const IR::Node* postorder(IR::BoolLiteral* expression) override;
    const IR::Node* postorder(IR::Operation_Relation* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* postorder(IR::ArrayIndex* expression) override;
    const IR::Node* postorder(IR::LAnd* expression) override { return binaryBool(expression); }
    const IR::Node* postorder(IR::LOr* expression) override { return binaryBool(expression); }
    const IR::Node* postorder(IR::Add* expression) override { return binaryArith(expression); }
    const IR::Node* postorder(IR::Sub* expression) override { return binaryArith(expression); }
    const IR::Node* postorder(IR::Mul* expression) override { return binaryArith(expression); }
    const IR::Node* postorder(IR::Div* expression) override { return unsBinaryArith(expression); }
    const IR::Node* postorder(IR::Mod* expression) override { return unsBinaryArith(expression); }
    const IR::Node* postorder(IR::Shl* expression) override { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override { return shift(expression); }
    const IR::Node* postorder(IR::BXor* expression) override { return bitwise(expression); }
    const IR::Node* postorder(IR::BAnd* expression) override { return bitwise(expression); }
    const IR::Node* postorder(IR::BOr* expression) override { return bitwise(expression); }
    const IR::Node* postorder(IR::Mask* expression) override { return typeSet(expression); }
    const IR::Node* postorder(IR::Range* expression) override { return typeSet(expression); }
    const IR::Node* postorder(IR::LNot* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Cmpl* expression) override;
    const IR::Node* postorder(IR::Cast* expression) override;
    const IR::Node* postorder(IR::Mux* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::PathExpression* expression) override;
    const IR::Node* postorder(IR::Member* expression) override;
    const IR::Node* postorder(IR::TypeNameExpression* expression) override;
    const IR::Node* postorder(IR::ListExpression* expression) override;
    const IR::Node* preorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::ConstructorCallExpression* expression) override;
    const IR::Node* postorder(IR::SelectExpression* expression) override;
    const IR::Node* postorder(IR::DefaultExpression* expression) override;

    const IR::Node* postorder(IR::IfStatement* stat) override;
    const IR::Node* postorder(IR::SwitchStatement* stat) override;
    const IR::Node* postorder(IR::AssignmentStatement* stat) override;
    const IR::Node* postorder(IR::ActionListElement* elem) override;
    const IR::Node* postorder(IR::KeyElement* elem) override;
    const IR::Node* postorder(IR::TableProperty* elem) override;
    const IR::Node* postorder(IR::SelectCase* elem) override;

    const IR::Node* postorder(IR::P4Program* program) override;

    Visitor::profile_t init_apply(const IR::Node* node) override;
};

}  // namespace P4

#endif /* _TYPECHECKING_TYPECHECKER_H_ */
