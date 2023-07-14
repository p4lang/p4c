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

#ifndef MIDEND_PREDICATION_H_
#define MIDEND_PREDICATION_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
This pass operates on action bodies.  It converts 'if' statements to
'?:' expressions, if possible.  Otherwise this pass will signal an
error.  This pass should be used only on architectures that do not
support conditionals in actions.
For this to work all statements must be assignments or other ifs.
if (e)
   a = f(b);
else
   c = f(d);
becomes (actual implementatation is slightly optimized):
{
    a = e ? f(b) : a;
    c = e ? c : f(d);
}
*/
class Predication final : public Transform {
    /** Private Transformer only for Predication pass.
     *  Used to remove EmptyStatements and empty BlockStatements from the code.
     */
    class EmptyStatementRemover final : public Transform {
     public:
        EmptyStatementRemover() {}
        const IR::Node *postorder(IR::EmptyStatement *statement) override;
        const IR::Node *postorder(IR::BlockStatement *statement) override;
    };
    /**
     * Private Transformer only for Predication pass.
     * This pass operates on nested Mux expressions(?:).
     * It replaces then and else expressions in Mux with
     * the appropriate expression from assignment.
     */
    class ExpressionReplacer final : public Transform {
     private:
        // Original assignment that the replacer works on
        const IR::AssignmentStatement *statement;
        // To keep track of the path used while traversing nested if-else statements:
        //      IF - true / ELSE - false
        const std::vector<bool> &traversalPath;
        // To store the condition used in every if-else statement
        std::vector<const IR::Expression *> &conditions;
        // Nesting level of Mux expressions
        unsigned currentNestingLevel = 0;
        // An indicator used to implement the logic for ArrayIndex transformation
        bool visitingIndex = false;

     public:
        explicit ExpressionReplacer(const IR::AssignmentStatement *a, std::vector<bool> &t,
                                    std::vector<const IR::Expression *> &c)
            : statement(a), traversalPath(t), conditions(c) {
            CHECK_NULL(a);
        }
        const IR::Mux *preorder(IR::Mux *mux) override;
        void emplaceExpression(IR::Mux *mux);
        void visitBranch(IR::Mux *mux, bool then);
        void setVisitingIndex(bool val);
    };

    // Used to dynamically generate names for variables in parts of code
    NameGenerator *generator;
    // Used to remove empty statements and empty block statements that appear in the code
    EmptyStatementRemover remover;
    bool inside_action;
    // Used to indicate whether or not an ArrayIndex should be modified.
    bool modifyIndex = false;
    // Tracking the nesting level of IF-ELSE statements
    unsigned ifNestingLevel;
    // Tracking the nesting level of dependency assignment
    unsigned depNestingLevel;
    // To store dependent assignments.
    // If current statement is equal to any member of dependentNames,
    // isStatementdependent of the coresponding statement is set to true.
    std::vector<cstring> dependentNames;
    // Traverse path of nested if-else statements.
    // Size of this vector is the current IF nesting level. IF - true / ELSE - false
    std::vector<bool> traversalPath;
    std::vector<cstring> dependencies;
    // Collects assignment statements with transformed right expression.
    // liveAssignments are pushed at the back of liveAssigns vector.
    std::map<cstring, const IR::AssignmentStatement *> liveAssignments;
    // Vector of assignment statements which collects assignments from
    // liveAssignments and dependencies in adequate order. In preorder
    // of if statements assignments from liveAssigns are pushed on rv block.
    std::vector<const IR::AssignmentStatement *> liveAssigns;
    // Vector of ArrayIndex declarations which is used to temporary
    // store these declarations so they can later be pushed on the 'rv' block.
    std::vector<const IR::Declaration *> indexDeclarations;
    // Map that shows if the current statement is dependent.
    // Bool value is true for dependent statements,
    // false for statements that are not dependent.
    std::map<cstring, bool> isStatementDependent;
    const IR::Statement *error(const IR::Statement *statement) const {
        if (inside_action && ifNestingLevel > 0)
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: Conditional execution in actions unsupported on this target", statement);
        return statement;
    }

 public:
    explicit Predication(NameGenerator *gen)
        : generator(gen), inside_action(false), ifNestingLevel(0), depNestingLevel(0) {
        setName("Predication");
    }
    const IR::Expression *clone(const IR::Expression *expression);
    const IR::Node *clone(const IR::AssignmentStatement *statement);
    const IR::Node *preorder(IR::IfStatement *statement) override;
    const IR::Node *preorder(IR::P4Action *action) override;
    const IR::Node *postorder(IR::P4Action *action) override;
    const IR::Node *preorder(IR::AssignmentStatement *statement) override;
    // Assignment dependecy checkers
    const IR::Node *preorder(IR::PathExpression *pathExpr) override;
    const IR::Node *preorder(IR::Member *member) override;
    const IR::Node *preorder(IR::ArrayIndex *arrInd) override;
    // The presence of other statements makes predication impossible to apply
    const IR::Node *postorder(IR::MethodCallStatement *statement) override {
        return error(statement);
    }
    const IR::Node *postorder(IR::ReturnStatement *statement) override { return error(statement); }
    const IR::Node *postorder(IR::ExitStatement *statement) override { return error(statement); }
};

}  // namespace P4

#endif /* MIDEND_PREDICATION_H_ */
