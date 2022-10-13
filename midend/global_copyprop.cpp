#include "global_copyprop.h"

namespace P4 {

/// Convert an expression into a string that uniqely identifies the lvalue referenced.
/// Return null cstring if not a reference to a lvalue.
static cstring lvalue_name(const IR::Expression *exp) {
    if (auto p = exp->to<IR::PathExpression>())
        return p->path->name;
    if (auto m = exp->to<IR::Member>()) {
        if (auto base = lvalue_name(m->expr))
            return base + "." + m->member;
    } else if (auto a = exp->to<IR::ArrayIndex>()) {
        if (auto k = a->right->to<IR::Constant>()) {
            if (auto base = lvalue_name(a->left))
                return base + "[" + std::to_string(k->asInt()) + "]";
        }
    } else if (auto sl = exp->to<IR::Slice>()) {
        if (auto e0 = lvalue_name(sl->e0))
            return e0;
    }
    return cstring();
}

/// Test to see if names denote overlapping locations.
bool names_overlap(cstring name1, cstring name2) {
    if (name1 == name2) return true;
    if (name1.startsWith(name2) && strchr(".[", name1.get(name2.size())))
        return true;
    if (name2.startsWith(name1) && strchr(".[", name2.get(name1.size())))
        return true;
    return false;
}

// Removes outdated values for variables.
void removeVarsContaining(std::map<cstring, const IR::Expression*> *vars, cstring name) {
    LOG6("removeVarsContaining(" << name << ")");
    for (auto &var : *vars) {
        LOG7("  checking entry: " << var.first << " = " << var.second);
        if (names_overlap(var.first, name)) {
            LOG4("  dropping the value for '" << var.first << "' as '"
                     << name << "' is being assigned to");
            var.second = nullptr;
        }
    }
}

// Removes values if they have changed
void compareValuesInMaps(std::map<cstring, const IR::Expression*> *oldValues,
                            std::map<cstring, const IR::Expression*> *newValues) {
    for (auto it : *newValues) {
        auto oldValue = (*oldValues)[it.first];
        if (((it.second == nullptr) ^ (oldValue == nullptr)) ||
            (it.second && oldValue && !(it.second->equiv(*oldValue))))
            removeVarsContaining(oldValues, it.first);
    }
}

// Removes values if they are used as Out/InOut parameter
void checkParametersForMap(const IR::ParameterList *params,
                            std::map<cstring, const IR::Expression*> *vars) {
    for (auto param : params->parameters)
        if (param->hasOut())
            removeVarsContaining(vars, param->name.name);
}

bool FindVariableValues::preorder(const IR::P4Control *ctrl) {
    LOG2("FindVariableValues working on control: " << ctrl->name);
    working = true;

    return true;
}

void FindVariableValues::postorder(const IR::P4Control *ctrl) {
    LOG2("FindVariableValues finished working on control: " << ctrl->name);
    // Clear map as the pass works on every control block separately
    vars.clear();
    working = false;
}

bool FindVariableValues::preorder(const IR::P4Action*) {
    return false;
}

bool FindVariableValues::preorder(const IR::P4Table*) {
    return false;
}

// When visiting IfStatement nodes the literal values assigned to variables in those
// 'ifTrue' and 'ifFalse' blocks can't be retained in the 'vars' container since the
// path taken can't be determined in compile time. Therefore a copy is made to preserve
// the state of the map before those blocks, and all variables that are possibly changed
// in these blocks are removed from the original map.
bool FindVariableValues::preorder(const IR::IfStatement *stat) {
    std::map<cstring, const IR::Expression*> copyOfVars(vars);
    LOG3("Working on 'IfStatement->ifTrue' block: " << stat->ifTrue);
    visit(stat->ifTrue);
    // Check if some variables had their values changed when visiting 'ifTrue' block.
    compareValuesInMaps(&copyOfVars, &vars);
    // Restore old state of map
    vars = copyOfVars;
    LOG3("Finished 'IfStatement->ifTrue' block: " << stat->ifTrue);
    LOG3("Working on 'IfStatement->ifFalse' block: " << stat->ifFalse);
    visit(stat->ifFalse);
    // Check if some variables had their values changed when visiting 'ifFalse' block.
    compareValuesInMaps(&copyOfVars, &vars);
    // Restore old state of map
    vars = copyOfVars;
    LOG3("Finished 'IfStatement->ifFalse' block: " << stat->ifFalse);

    return false;
}

// Switch statement is equivalent to a series of If stataments
// That's why implementation for visiting SwitchStatement is the same as for visiting IfStatement
bool FindVariableValues::preorder(const IR::SwitchStatement *stat) {
    std::map<cstring, const IR::Expression*> copyOfVars(vars);
    for (auto caseStatement : stat->cases) {
        LOG3("Working on case: " << caseStatement->label->toString()
            << " block: " << caseStatement->statement);
        visit(caseStatement->statement);
        compareValuesInMaps(&copyOfVars, &vars);
        vars = copyOfVars;
        LOG3("Finished case: " << caseStatement->label->toString()
            << " block: " << caseStatement->statement);
    }

    return false;
}

// Update the value for the 'stat->left' variable.
bool FindVariableValues::preorder(const IR::AssignmentStatement *stat) {
    if (!working || lvalue_name(stat->left).isNullOrEmpty())
        return false;

    LOG5("Working on statement: " << stat);
    // Remove old values
    if (vars[lvalue_name(stat->left)] == nullptr ||
            !(stat->right->equiv(*(vars[lvalue_name(stat->left)]))))
        removeVarsContaining(&vars, lvalue_name(stat->left));
    // Set value
    if (auto lit = stat->right->to<IR::Literal>()) {
        if (stat->left->is<IR::Slice>())
            return false;
        vars[lvalue_name(stat->left)] = lit;
        LOG5("  Setting value: " << lit << ", for: " << stat->left);
    } else if (auto v = vars[lvalue_name(stat->right)]) {
        auto lit = v->to<IR::Literal>();
        if (lit == nullptr)
            return false;
        if (stat->left->is<IR::Slice>() || stat->right->is<IR::Slice>())
            return false;
        vars[lvalue_name(stat->left)] = lit;
        LOG5("  Setting value: " << lit << ", for: " << stat->left);
    }

    return false;
}

// The core idea of this pass is represented here, it works on 'ActionCall' nodes by accessing
// the body of the action using the pointer acquired by resolving the 'MethodCallExpression'.
// An entry in the 'actions' map is set for the action node that was acquired by resolving
// the 'ActionCall'.
void FindVariableValues::postorder(const IR::MethodCallExpression *mc) {
    if (!working || mc->method->is<IR::Member>())
        return;

    LOG5("Working on 'MethodCallexpression': " << mc);
    auto *mi = MethodInstance::resolve(mc, refMap, typeMap, true);
    // Remove entries in the 'vars' map for variables that are used as 'Out' or 'InOut' parameters.
    if (auto aCall = mi->to<ActionCall>()) {
        // Check to see if an entry already exists for this action, this should not happen
        // if the preconditions of this pass have been met.
        if (actions->find(aCall->action) == actions->end()) {
            // Add an entry in the 'actions' map for the action being called.
            LOG6("  Is 'ActionCall'. Adding entry for action: " << aCall->action);
            (*actions)[aCall->action] = new std::map<cstring, const IR::Expression*>(vars);
            visit(aCall->action->body);
        } else {
            LOG6("  Is 'ActionCall'. Entry already exists for this action: " << aCall->action);
        }
    } else if (auto eFun = mi->to<ExternFunction>()) {
        LOG6("  Is 'ExternFunction'. Checking params for: " << eFun->method);
        checkParametersForMap(eFun->method->getParameters(), &vars);
    } else if (auto fCall = mi->to<FunctionCall>()) {
        LOG6("  Is 'FunctionCall'. Checking params for: " << fCall->function);
        checkParametersForMap(fCall->function->getParameters(), &vars);
    }
    LOG5("Finished 'MethodCallExpression': " << mc);
}

// Returns value stored for variable denoted with 'name'.
// Returns nullptr if nothing is stored.
const IR::Expression *DoGlobalCopyPropagation::copyprop_name(cstring name) {
    if (name.isNullOrEmpty() || !performRewrite) return nullptr;
    LOG6("Propagating value: " << (*vars)[name] << " for variable: " << name);
    return (*vars)[name];
}

const IR::Expression *DoGlobalCopyPropagation::postorder(IR::PathExpression *path) {
    if (!performRewrite)
        return path;

    auto ret = copyprop_name(path->path->name);
    return ret ? ret : path;
}

const IR::Expression *DoGlobalCopyPropagation::preorder(IR::Member *member) {
    if (!performRewrite)
        return member;

    if (auto name = lvalue_name(member)) {
        prune();
        if (auto rv = copyprop_name(name))
            return rv;
    }
    return member;
}

const IR::Expression *DoGlobalCopyPropagation::preorder(IR::ArrayIndex *arr) {
    if (!performRewrite)
        return arr;

    if (auto name = lvalue_name(arr)) {
        prune();
        if (auto rv = copyprop_name(name)) {
            return rv;
        }
    }
    return arr;
}

// If information for this action is available work on action body and propagate values.
const IR::P4Action *DoGlobalCopyPropagation::preorder(IR::P4Action *act) {
    if (actions->find(getOriginal()) != actions->end()) {
        performRewrite = true;
        vars = actions->find(getOriginal())->second;
        LOG2("DoGlobalCopyPropagation working on action: " << act->name);
    } else {
        performRewrite = false;
    }

    return act;
}

const IR::P4Action *DoGlobalCopyPropagation::postorder(IR::P4Action *act) {
    performRewrite = false;
    LOG2("DoGlobalCopyPropagation finished action: " << act->name);
    return act;
}

IR::MethodCallExpression *DoGlobalCopyPropagation::postorder(IR::MethodCallExpression *mc) {
    if (!performRewrite || mc->method->is<IR::Member>())
        return mc;

    auto *mi = MethodInstance::resolve(mc, refMap, typeMap, true);
    LOG5("Working on 'MethodCallExpression' : " << mc);
    // Remove entries in the 'vars' map for variables that are used as 'Out' or 'InOut' parameters.
    if (auto eFun = mi->to<ExternFunction>()) {
        LOG6("  Is 'ExternFunction'. Checking params for: " << eFun->method);
        checkParametersForMap(eFun->method->getParameters(), vars);
    } else if (auto fCall = mi->to<P4::FunctionCall>()) {
        LOG6("  Is 'FunctionCall'. Checking params for: " << fCall->function);
        checkParametersForMap(fCall->function->getParameters(), vars);
    }
    LOG5("Finished 'MethodCallExpression' : " << mc);

    return mc;
}


// When visiting IfStatement nodes the literal values assigned to variables in those
// 'ifTrue' and 'ifFalse' blocks can't be retained in the 'vars' container since the
// path taken can't be determined in compile time. Therefore a copy is made to preserve
// the state of the map before those blocks, and all variables that are possibly changed
// in these blocks are removed from the original map.
IR::IfStatement *DoGlobalCopyPropagation::preorder(IR::IfStatement *stat) {
    if (!performRewrite)
        return stat;

    std::map<cstring, const IR::Expression*> copyOfVars(*vars);
    LOG3("Working on 'IfStatement->ifTrue' block: " << stat->ifTrue);
    visit(stat->ifTrue);
    // Check if some variables had their values changed when visiting 'ifTrue' block.
    compareValuesInMaps(&copyOfVars, vars);
    // Restore old state of map
    *vars = copyOfVars;
    LOG3("Finished 'IfStatement->ifTrue' block: " << stat->ifTrue);
    LOG3("Working on 'IfStatement->ifFalse' block: " << stat->ifFalse);
    visit(stat->ifFalse);
    // Check if some variables had their values changed when visiting 'ifFalse' block.
    compareValuesInMaps(&copyOfVars, vars);
    // Restore old state of map
    *vars = copyOfVars;
    LOG3("Finished 'IfStatement->ifFalse' block: " << stat->ifFalse);
    prune();

    return stat;
}

// Propagate values for variables on the right side of the statement
// and update value for 'stat->left' variable if needed.
const IR::Node *DoGlobalCopyPropagation::preorder(IR::AssignmentStatement *stat) {
    if (!performRewrite)
        return stat;

    LOG5("Working on statement: " << stat);
    LOG6("  Visiting right side of statement");
    visit(stat->right);
    // Remove old values
    if ((*vars)[lvalue_name(stat->left)] == nullptr ||
            !(stat->right->equiv(*((*vars)[lvalue_name(stat->left)]))))
        removeVarsContaining(vars, lvalue_name(stat->left));
    performRewrite = false;
    LOG6("  Visiting left side of statement");
    visit(stat->left);
    performRewrite = true;
    prune();
    // Store the value for 'stat->left' if it is now a constant. If it is an assignment to an
    // identical literal as already stored in the 'vars' container the statement is removed.
    if (auto lit = stat->right->to<IR::Literal>()) {
        if (stat->left->is<IR::Slice>())
            return stat;
        if ((*vars)[lvalue_name(stat->left)] && lit->equiv(*((*vars)[lvalue_name(stat->left)])))
            return new IR::EmptyStatement(stat->srcInfo);
        (*vars)[lvalue_name(stat->left)] = lit;
        LOG5("  Setting value: " << lit << ", for: " << stat->left);
    }

    return stat;
}

}  // namespace P4
