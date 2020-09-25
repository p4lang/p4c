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

#ifndef _MIDEND_PREDICATION_H_
#define _MIDEND_PREDICATION_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

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
    class EmptyStatementRemover final : public Modifier {
     public:
        EmptyStatementRemover() {}
        bool preorder(IR::BlockStatement * block) override;
    };

    /**
     * Private Transformer only for Predication pass.
     * This pass operates on nested Mux expressions(?:). 
     * It replaces then and else expressions in Mux with 
     * the appropriate expression from assignment.
     */
    class ExpressionReplacer final : public Transform {
     private:
        const IR::AssignmentStatement * statement;
        const std::vector<bool>& travesalPath;
        std::vector<const IR::Expression*>& conditions;
        unsigned currentNestingLevel = 0;
     public:
        explicit ExpressionReplacer(const IR::AssignmentStatement * a,
                                    std::vector<bool>& t,
                                    std::vector<const IR::Expression*>& c)
        : statement(a), travesalPath(t), conditions(c)
        { CHECK_NULL(a); }
        const IR::Mux * preorder(IR::Mux * mux) override;
        void emplaceExpression(IR::Mux * mux);
        void visitBranch(IR::Mux * mux, bool then);
    };

    NameGenerator* generator;
    EmptyStatementRemover remover;
    std::vector<IR::BlockStatement*> blocks;
    bool inside_action;
    unsigned ifNestingLevel;
    // Tracking the nesting level of dependency assignment
    unsigned depNestingLevel;
    // To store dependent assignments.
    // If current statement is equal to any member of dependentNames,
    // isStatementdependent of the coresponding statement is set to true.
    std::vector<cstring> dependentNames;
    // Traverse path of nested if-else statements
    // true at the end of the vector means that you are currently visiting 'then' branch'
    // false at the end of the vector means that you are in the else branch of the if statement.
    // Size of this vector is the current if nesting level.
    std::vector<bool> travesalPath;
    std::vector<cstring> dependencies;
    // Collects assignment statements with transformed right expression.
    // liveAssignments are pushed at the back of liveAssigns vetor.
    std::map<cstring, const IR::AssignmentStatement *> liveAssignments;
    // Vector of assignment statements which collects assignments from
    // liveAssignments and dependencies in adequate order. In preorder
    // of if statements assignments from liveAssigns are pushed on rv block.
    std::vector<const IR::AssignmentStatement*> liveAssigns;
    // Map that shows if the current statement is dependent.
    // Bool value is true for dependent statements,
    // false for statements that are not dependent.
    std::map<cstring, bool> isStatementDependent;
    const IR::Statement* error(const IR::Statement* statement) const {
        if (inside_action && ifNestingLevel > 0)
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: Conditional execution in actions unsupported on this target",
                    statement);
        return statement;
    }

 public:
    explicit Predication(NameGenerator* gen) :
        generator(gen), inside_action(false), ifNestingLevel(0), depNestingLevel(0)
    { setName("Predication"); }
    const IR::Expression* clone(const IR::Expression* expression);
    const IR::Node* clone(const IR::AssignmentStatement* statement);
    const IR::Node* preorder(IR::IfStatement* statement) override;
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::AssignmentStatement* statement) override;
    // Assignment dependecy checkers
    const IR::Node* preorder(IR::PathExpression* pathExpr) override;
    const IR::Node* preorder(IR::Member* member) override;
    const IR::Node* preorder(IR::ArrayIndex* arrInd) override;
    // The presence of other statements makes predication impossible to apply
    const IR::Node* postorder(IR::MethodCallStatement* statement) override
    { return error(statement); }
    const IR::Node* postorder(IR::ReturnStatement* statement) override
    { return error(statement); }
    const IR::Node* postorder(IR::ExitStatement* statement) override
    { return error(statement); }
};

}  // namespace P4

#endif /* _MIDEND_PREDICATION_H_ */
