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
x = y;
becomes (actual implementatation is slightly optimized):
{
    a = e ? f(b) : a;
    c = e ? c : f(d);
}
x = y;
Not the most efficient conversion currently.
This could be made better by looking on both the "then" and "else"
branches, but in this way we cannot have two side-effects in the same
conditional statement.
*/
class Predication final : public Transform {

    class ReplaceChecker final : public Inspector {
        const IR::AssignmentStatement * assignmentStatement;
        bool conflictDetected = false;
        unsigned currentNestingLevel = 0;
        const std::vector<bool>& travesalPath;
    public:

        explicit ReplaceChecker(std::vector<bool>& t)
        : travesalPath(t)
        { }

        const bool& isConflictDetected() const{ return conflictDetected; }
        bool preorder(const IR::Mux * mux) override;
        bool preorder(const IR::AssignmentStatement * statement) override;
    };

    /**
     * Private Transformer only for Predication pass.
     * This pass operates on nested Mux expressions(?:). 
     * It replaces then and else expressions in mux with 
     * the appropriate expression from assignment.
     */
    class ExpressionReplacer final : public Transform {
     private:
        IR::AssignmentStatement * statement;
        const IR::Expression * rightExpression;
        const std::vector<bool>& travesalPath;
        const std::vector<const IR::Expression *>& conditions;
        unsigned currentNestingLevel = 0;
     public:
        explicit ExpressionReplacer(const IR::Expression * e, std::vector<bool>& t, std::vector<const IR::Expression *>& conds)
        : rightExpression(e), travesalPath(t), conditions(conds)
        { CHECK_NULL(e); }
        const IR::Mux * preorder(IR::Mux * mux) override;
        const IR::AssignmentStatement * preorder(IR::AssignmentStatement * statement) override;
    };

    NameGenerator* generator;
    bool inside_action;
    unsigned ifNestingLevel;
    // Traverse path of nested if-else statements
    // true at the end of the vector means that you are currently visiting 'then' branch'
    // false at the end of the vector means that you are in the else branch of the if statement.
    // Size of this vector is the current if nesting level.
    std::vector<bool> travesalPath;
    std::vector<const IR::Expression *> nestedConditions;
    std::map<cstring, const IR::AssignmentStatement *> liveAssignments;
    std::map<cstring, const IR::Expression *> assignments;

    const IR::Statement* error(const IR::Statement* statement) const {
        if (inside_action && ifNestingLevel > 0)
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: Conditional execution in actions unsupported on this target",
                    statement);
        return statement;
    }

 public:
    explicit Predication(NameGenerator* generator) : generator(generator),
            inside_action(false), ifNestingLevel(0)
    { CHECK_NULL(generator); setName("Predication"); }

    const IR::Expression* clone(const IR::Expression* expression);
    const IR::Node* preorder(IR::IfStatement* statement) override;
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::AssignmentStatement* statement) override;
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
