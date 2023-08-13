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

#include "frontends/p4/sideEffects.h"

#include <utility>

#include "frontends/p4/alias.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/tableApply.h"
#include "ir/vector.h"
#include "lib/enumerator.h"
#include "lib/map.h"

namespace P4 {

cstring DoSimplifyExpressions::createTemporary(const IR::Type *type) {
    type = type->getP4Type();
    BUG_CHECK(type && !type->is<IR::Type_Dontcare>(), "Can't create don't-care temps");
    auto tmp = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(tmp, nullptr), type);
    toInsert.push_back(decl);
    return tmp;
}

/** Add ```@varName = @expression``` to the vector of statements.
 *
 * @return A copy of the l-value expression created for varName.
 */
const IR::Expression *DoSimplifyExpressions::addAssignment(Util::SourceInfo srcInfo,
                                                           cstring varName,
                                                           const IR::Expression *expression) {
    const IR::PathExpression *left;
    if (auto pe = expression->to<IR::PathExpression>())
        left = new IR::PathExpression(IR::ID(varName, pe->path->name.originalName));
    else
        left = new IR::PathExpression(IR::ID(varName, nullptr));
    auto stat = new IR::AssignmentStatement(srcInfo, left, expression);
    statements.push_back(stat);
    auto result = left->clone();
    added->emplace(result);
    return result;
}

// catch-all case
const IR::Node *DoSimplifyExpressions::postorder(IR::Expression *expression) {
    LOG3("Visiting " << dbp(expression));
    auto orig = getOriginal<IR::Expression>();
    typeMap->cloneExpressionProperties(expression, orig);
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::Literal *expression) {
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::ArrayIndex *expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    if (SideEffects::check(getOriginal<IR::Expression>(), this, refMap, typeMap) ||
        // if the expression appears as part of an argument also use a temporary for the index
        findContext<IR::Argument>() != nullptr) {
        visit(expression->left);
        CHECK_NULL(expression->left);
        visit(expression->right);
        CHECK_NULL(expression->right);
        if (added->find(expression->right) == added->end()) {
            // If the index is a fresh temporary we don't need to replace it again;
            // we are sure it will not alias anything else.
            // Otherwise this can lead to an infinite loop.
            if (!expression->right->is<IR::Constant>()) {
                auto indexType = typeMap->getType(expression->right, true);
                auto tmp = createTemporary(indexType);
                expression->right = addAssignment(expression->srcInfo, tmp, expression->right);
                typeMap->setType(expression->right, indexType);
            }
        }
    }
    typeMap->setType(expression, type);
    if (isWrite()) typeMap->setLeftValue(expression);
    prune();
    return expression;
}

static bool isIfContext(const Visitor::Context *ctxt) {
    if (ctxt && ctxt->node->is<IR::LNot>()) ctxt = ctxt->parent;
    return ctxt && ctxt->node->is<IR::IfStatement>();
}

const IR::Node *DoSimplifyExpressions::preorder(IR::Member *expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    const IR::Expression *rv = expression;
    if (SideEffects::check(getOriginal<IR::Expression>(), this, refMap, typeMap) ||
        // This may be part of a left-value that is passed as an out argument
        findContext<IR::Argument>() != nullptr) {
        visit(expression->expr);
        CHECK_NULL(expression->expr);

        // Special case for table.apply().hit/miss, which is not dismantled by
        // the MethodCallExpression.
        if (TableApplySolver::isHit(expression, refMap, typeMap) ||
            TableApplySolver::isMiss(expression, refMap, typeMap)) {
            if (isIfContext(getContext())) {
                /* if the hit/miss test is directly in an "if", don't bother cloning it
                 * as that will just create a redundant table later */
            } else if (getParent<IR::AssignmentStatement>()) {
                /* already assigning it somewhere -- no need to add another copy */
            } else {
                BUG_CHECK(type->is<IR::Type_Boolean>(), "%1%: not boolean", type);
                auto tmp = createTemporary(type);
                auto path = new IR::PathExpression(IR::ID(tmp, nullptr));
                auto stat = new IR::AssignmentStatement(path, expression);
                statements.push_back(stat);
                typeMap->setType(expression, type);
                rv = path->clone();
                ;
            }
        }
    }
    typeMap->setType(rv, type);
    if (isWrite()) typeMap->setLeftValue(rv);
    prune();
    return rv;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::SelectExpression *expression) {
    LOG3("Visiting " << dbp(expression));
    visit(expression->select);
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::Operation_Unary *expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    visit(expression->expr);
    CHECK_NULL(expression->expr);
    typeMap->setType(expression, type);
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::StructExpression *expression) {
    LOG3("Visiting " << dbp(expression));
    bool foundEffect = false;
    for (auto v : expression->components) {
        if (SideEffects::check(v->expression, this, refMap, typeMap)) {
            foundEffect = true;
            break;
        }
    }
    if (!foundEffect) return expression;
    // allocate temporaries for all members in order.
    // this will handle cases like a = (S) { b, f(b) }, where f can mutate b.
    IR::IndexedVector<IR::NamedExpression> vec;
    LOG3("Dismantling " << dbp(expression));
    for (auto &v : expression->components) {
        auto t = typeMap->getType(v->expression, true);
        auto tmp = createTemporary(t);
        visit(v);
        auto path = addAssignment(expression->srcInfo, tmp, v->expression);
        typeMap->setType(path, t);
        // We cannot directly mutate v, because of https://github.com/p4lang/p4c/issues/43
        vec.push_back(new IR::NamedExpression(v->name, path));
    }
    expression->components = vec;
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::ListExpression *expression) {
    LOG3("Visiting " << dbp(expression));
    bool foundEffect = false;
    for (auto v : expression->components) {
        if (SideEffects::check(v, this, refMap, typeMap)) {
            foundEffect = true;
            break;
        }
    }
    if (!foundEffect) return expression;
    // allocate temporaries for all members in order.
    // this will handle cases like a = { b, f(b) }, where f can mutate b.
    LOG3("Dismantling " << dbp(expression));
    for (auto &v : expression->components) {
        auto t = typeMap->getType(v, true);
        auto tmp = createTemporary(t);
        visit(v);
        auto path = addAssignment(expression->srcInfo, tmp, v);
        v = path;
        typeMap->setType(path, t);
    }
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::Operation_Binary *expression) {
    LOG3("Visiting " << dbp(expression));
    auto original = getOriginal<IR::Operation_Binary>();
    auto type = typeMap->getType(original, true);
    if (SideEffects::check(original, this, refMap, typeMap)) {
        if (SideEffects::check(original->right, this, refMap, typeMap)) {
            // We are a bit conservative here. We handle this case:
            // T f(inout T val) { ... }
            // val + f(val);
            // We must save val before the evaluation of f
            auto ltype = typeMap->getType(original->left, true);
            auto leftTmp = createTemporary(ltype);
            visit(expression->left);
            auto leftPath = addAssignment(expression->srcInfo, leftTmp, expression->left);
            expression->left = leftPath;
            typeMap->setType(leftPath, ltype);
        } else {
            visit(expression->left);
        }
        CHECK_NULL(expression->left);
        visit(expression->right);
        typeMap->setType(expression, type);
        auto tmp = createTemporary(type);
        auto path = addAssignment(expression->srcInfo, tmp, expression);
        typeMap->setType(path, type);
        prune();
        return path;
    }
    typeMap->setType(expression, type);
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::shortCircuit(IR::Operation_Binary *expression) {
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    if (SideEffects::check(getOriginal<IR::Expression>(), this, refMap, typeMap)) {
        visit(expression->left);
        CHECK_NULL(expression->left);

        // e1 && e2
        // becomes roughly:
        // if (!simplify(e1))
        //    tmp = false;
        // else
        //    tmp = simplify(e2);

        bool land = expression->is<IR::LAnd>();
        auto constant = new IR::BoolLiteral(!land);
        auto tmp = createTemporary(type);
        auto ifTrue = new IR::AssignmentStatement(
            expression->srcInfo, new IR::PathExpression(IR::ID(tmp, nullptr)), constant);

        auto save = statements;
        statements.clear();
        visit(expression->right);
        auto path = addAssignment(expression->srcInfo, tmp, expression->right);
        auto ifFalse = statements;
        statements = save;
        if (land) {
            expression->left = new IR::LNot(expression->left);
            typeMap->setType(expression->left, type);
        }
        auto block = new IR::BlockStatement(ifFalse);
        auto ifStatement =
            new IR::IfStatement(expression->srcInfo, expression->left, ifTrue, block);
        statements.push_back(ifStatement);
        typeMap->setType(path, type);
        prune();
        return path;
    }
    typeMap->setType(expression, type);
    prune();
    return expression;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::Mux *expression) {
    // We always dismantle muxes - some architectures may not support them
    LOG3("Visiting " << dbp(expression));
    auto type = typeMap->getType(getOriginal(), true);
    visit(expression->e0);
    CHECK_NULL(expression->e0);
    auto tmp = createTemporary(type);

    auto save = statements;
    statements.clear();
    visit(expression->e1);
    (void)addAssignment(expression->srcInfo, tmp, expression->e1);
    auto ifTrue = statements;

    statements.clear();
    visit(expression->e2);
    auto path = addAssignment(expression->srcInfo, tmp, expression->e2);
    auto ifFalse = statements;
    statements = save;

    auto ifStatement = new IR::IfStatement(expression->e0, new IR::BlockStatement(ifTrue),
                                           new IR::BlockStatement(ifFalse));
    statements.push_back(ifStatement);
    typeMap->setType(path, type);
    prune();
    return path;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::LAnd *expression) {
    return shortCircuit(expression);
}
const IR::Node *DoSimplifyExpressions::preorder(IR::LOr *expression) {
    return shortCircuit(expression);
}

bool DoSimplifyExpressions::mayAlias(const IR::Expression *left,
                                     const IR::Expression *right) const {
    ReadsWrites rw(refMap);
    return rw.mayAlias(left, right);
}

/// Returns true if type is a header or a struct containing a header.
/// (We don't care about stacks or unions.)
bool DoSimplifyExpressions::containsHeaderType(const IR::Type *type) {
    if (type->is<IR::Type_Header>()) return true;
    auto st = type->to<IR::Type_Struct>();
    if (st == nullptr) return false;
    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (containsHeaderType(ftype)) return true;
    }
    return false;
}

namespace {

/// When invoked on an expression computes all expressions that are
/// modified while evaluating the expression.  This is a list of
/// left-values.  Also, a table application expression is
/// assumed to modify everything.
class GetWrittenExpressions : public Inspector {
    ReferenceMap *refMap;
    TypeMap *typeMap;

 public:
    // If this expression is in the set, it means that the expression
    // may modify everything in the program.  This is an
    // over-approximation used when the expression is a table
    // invocation --- for this case it's too complicated to compute
    // precisely the side effects without an inter-procedural
    // analysis.
    static const IR::Expression *everything;
    std::set<const IR::Expression *> written;

    GetWrittenExpressions(ReferenceMap *refMap, TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("GetWrittenExpressions");
    }
    void postorder(const IR::MethodCallExpression *expression) override {
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (auto a = mi->to<ApplyMethod>()) {
            if (a->isTableApply()) {
                written.emplace(everything);
                return;
            }
        }
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            if (!p->hasOut()) continue;
            // The only modified expressions much be left-values
            // that are substituted to out or inout parameters.
            auto arg = mi->substitution.lookup(p);
            LOG3("Expression is modified " << arg->expression);
            written.emplace(arg->expression);
        }
    }
};

// Some expression that cannot occur in the program.
const IR::Expression *GetWrittenExpressions::everything = new IR::Constant(0);

}  // namespace

const IR::Node *DoSimplifyExpressions::preorder(IR::MethodCallExpression *mce) {
    // BUG_CHECK(!isWrite(), "%1%: method on left hand side?", mce);
    // isWrite is too conservative, so this check may fail for something like f().isValid()
    LOG3("Visiting " << dbp(mce));
    auto orig = getOriginal<IR::MethodCallExpression>();
    auto type = typeMap->getType(orig, true);
    if (!SideEffects::check(orig, this, refMap, typeMap)) {
        return mce;
    }

    auto copyBack = new IR::IndexedVector<IR::StatOrDecl>();
    auto args = new IR::Vector<IR::Argument>();
    auto mi = MethodInstance::resolve(orig, refMap, typeMap);

    // If a parameter is in this set then we use a temporary to
    // copy the corresponding argument.  We could always use
    // temporaries for arguments - it is always correct - but this
    // could entail the creation of "fat" temporaries that contain
    // large structs.  We want to avoid copying these large
    // structs if possible.
    std::set<const IR::Parameter *> useTemporary;
    // Set of expressions modified while evaluating this method call.
    std::set<const IR::Expression *> modifies;
    GetWrittenExpressions gwe(refMap, typeMap);
    gwe.setCalledBy(this);
    mce->apply(gwe);

    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (p->direction == IR::Direction::None) continue;
        auto arg = mi->substitution.lookup(p);
        if (gwe.written.find(GetWrittenExpressions::everything) != gwe.written.end()) {
            // just copy everything.
            LOG3("Detected table application, using temporaries for all parameters " << arg);
            for (auto p : *mi->substitution.getParametersInArgumentOrder()) useTemporary.emplace(p);
            break;
        }
        modifies.insert(gwe.written.begin(), gwe.written.end());

        // If an argument evaluation has side-effects then
        // always use a temporary to hold the argument value.
        if (SideEffects::check(arg->expression, this, refMap, typeMap)) {
            LOG3("Using temporary for " << dbp(mce) << " param " << dbp(p) << " arg side effect");
            useTemporary.emplace(p);
            continue;
        }

        // If the parameter is out and the argument is a slice then
        // also use a temporary; makes the job of def-use analysis easier
        if (arg->expression->is<IR::Slice>() && p->hasOut()) {
            LOG3("Using temporary for " << dbp(mce) << " param " << dbp(p)
                                        << " since it is an out slice");
            useTemporary.emplace(p);
            continue;
        }

        // If the parameter contains header values and the
        // argument is a list expression or a struct initializer
        // then we also use a temporary.  This makes the job of
        // the SetHeaders pass later simpler (otherwise we have to
        // handle this case there).
        auto ptype = typeMap->getType(p, true);
        if (!containsHeaderType(ptype)) continue;

        if (arg->expression->is<IR::ListExpression>() ||
            arg->expression->is<IR::StructExpression>()) {
            LOG3("Using temporary for " << dbp(mce) << " param " << dbp(p)
                                        << " assigning tuple to header");
            useTemporary.emplace(p);
            continue;
        }
    }

    // For each argument check to see if it aliases any expression in the written set.
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (useTemporary.find(p) != useTemporary.end()) continue;
        auto arg = mi->substitution.lookup(p);
        if (typeMap->isCompileTimeConstant(arg->expression)) continue;
        for (auto e : modifies) {
            // Here we use just raw equality: equivalent but not equal expressions
            // should be compared.
            if (e != arg->expression && mayAlias(arg->expression, e)) {
                LOG3("Using temporary for " << dbp(mce) << " param " << dbp(p) << " aliasing"
                                            << dbp(e));
                if (p->hasOut())
                    warn(ErrorType::WARN_ORDERING,
                         "%1%: 'out' argument has fields in common with %2%", arg, e);
                useTemporary.emplace(p);
                break;
            }
        }
    }

    visit(mce->method);

    ClonePathExpressions cloner;  // a cheap version of deep copy
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        if (p->direction == IR::Direction::None) {
            args->push_back(arg);
            continue;
        }

        bool useTemp = useTemporary.count(p) != 0;
        LOG3("Transforming " << dbp(arg) << " for " << dbp(p) << (useTemp ? " with " : " without ")
                             << "temporary");

        const IR::Expression *argValue = nullptr;
        visit(arg);  // May mutate arg!  Recursively simplifies arg.
        auto argex = arg->expression;
        CHECK_NULL(argex);

        if (useTemp) {
            // declare temporary variable if this is not already
            // a temporary
            if (temporaries.find(argex) == temporaries.end()) {
                LOG3("Not a temporary " << argex);
                auto paramtype = typeMap->getType(p, true);
                if (paramtype->is<IR::Type_Dontcare>()) paramtype = typeMap->getType(arg, true);
                auto tmp = createTemporary(paramtype);
                argValue = new IR::PathExpression(IR::ID(tmp, nullptr));
                typeMap->setType(argValue, paramtype);
                typeMap->setLeftValue(argValue);
                if (p->direction != IR::Direction::Out) {
                    auto clone = argValue->clone();
                    auto stat = new IR::AssignmentStatement(clone, argex);
                    LOG3(clone << " = " << argex);
                    statements.push_back(stat);
                    typeMap->setType(clone, paramtype);
                    typeMap->setLeftValue(clone);
                }
            } else {
                LOG3("Already a temporary " << argex);
                argValue = argex;
            }
        } else {
            argValue = argex;
        }
        if (p->direction != IR::Direction::In && useTemp) {
            auto assign = new IR::AssignmentStatement(cloner.clone<IR::Expression>(argex),
                                                      cloner.clone<IR::Expression>(argValue));
            copyBack->push_back(assign);
            LOG3("Will copy out value " << dbp(assign));
        }
        args->push_back(new IR::Argument(arg->name, argValue));
    }

    // Special handling for table.apply(...).X; we cannot generate
    // a temporary for the method call tmp = table.apply(), since
    // we cannot write down the type of tmp.  So we don't
    // dismantle these expressions.
    bool tbl_apply = false;
    if (auto mmbr = getParent<IR::Member>()) {
        auto tbl = TableApplySolver::isActionRun(mmbr, refMap, typeMap);
        auto tbl1 = TableApplySolver::isHit(mmbr, refMap, typeMap);
        auto tbl2 = TableApplySolver::isMiss(mmbr, refMap, typeMap);
        tbl_apply = tbl != nullptr || tbl1 != nullptr || tbl2 != nullptr;
    }
    // Simplified method call, with arguments substituted
    if (!IR::equiv(mce->arguments, args)) mce->arguments = args;
    typeMap->setType(mce, type);
    const IR::Expression *rv = mce;
    // See whether we assign the result of the call to a temporary
    if (type->is<IR::Type_Void>() ||             // no return type
        getParent<IR::MethodCallStatement>()) {  // result of call is not used
        if (!copyBack->empty()) {
            statements.push_back(new IR::MethodCallStatement(mce->srcInfo, mce));
            rv = nullptr;
        }  // else just return mce
    } else if (tbl_apply) {
        typeMap->setType(mce, type);
        rv = mce;
    } else if (getParent<IR::AssignmentStatement>() && copyBack->empty()) {
        /* no need for an extra copy as there's no out args to copy back afterwards */
        typeMap->setType(mce, type);
        rv = mce;
    } else {
        auto tmp = createTemporary(type);
        auto left = new IR::PathExpression(IR::ID(tmp, nullptr));
        auto stat = new IR::AssignmentStatement(left, mce);
        statements.push_back(stat);
        rv = left->clone();
        LOG3("Is temporary " << rv);
        temporaries.emplace(rv);
        typeMap->setType(rv, type);
        LOG3(orig << " replaced with " << left << " = " << mce);
    }
    statements.append(*copyBack);
    prune();
    return rv;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::Function *function) {
    if (toInsert.empty()) return function;
    auto body = new IR::BlockStatement(function->body->srcInfo);
    for (auto a : toInsert) body->push_back(a);
    for (auto s : function->body->components) body->push_back(s);
    function->body = body;
    toInsert.clear();
    return function;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::P4Parser *parser) {
    if (toInsert.empty()) return parser;
    parser->parserLocals.append(toInsert);
    toInsert.clear();
    return parser;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::P4Control *control) {
    if (toInsert.empty()) return control;
    control->controlLocals.append(toInsert);
    toInsert.clear();
    return control;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::P4Action *action) {
    if (toInsert.empty()) return action;
    auto body = new IR::BlockStatement(action->body->srcInfo);
    for (auto a : toInsert) body->push_back(a);
    for (auto s : action->body->components) body->push_back(s);
    action->body = body;
    toInsert.clear();
    return action;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::ParserState *state) {
    if (state->selectExpression == nullptr) return state;
    state->components.append(statements);
    statements.clear();
    return state;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::AssignmentStatement *statement) {
    if (statements.empty()) return statement;
    statements.push_back(statement);
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::MethodCallStatement *statement) {
    if (statements.empty()) {
        BUG_CHECK(statement->methodCall, "NULL methodCall?");
        return statement;
    }
    if (statement->methodCall) statements.push_back(statement);
    if (statements.size() == 1) {
        auto rv = statements.front();
        statements.clear();
        return rv;
    }
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

const IR::Node *DoSimplifyExpressions::postorder(IR::ReturnStatement *statement) {
    if (statements.empty()) return statement;
    statements.push_back(statement);
    auto block = new IR::BlockStatement(statements);
    statements.clear();
    return block;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::IfStatement *statement) {
    IR::Statement *rv = statement;
    visit(statement->condition, "condition");
    if (!statements.empty()) {
        statements.push_back(statement);
        rv = new IR::BlockStatement(statements);
        statements.clear();
    }
    visit(statement->ifTrue, "ifTrue");
    visit(statement->ifFalse, "ifFalse");
    prune();
    return rv;
}

const IR::Node *DoSimplifyExpressions::preorder(IR::SwitchStatement *statement) {
    IR::Statement *rv = statement;
    visit(statement->expression, "expression");
    if (!statements.empty()) {
        statements.push_back(statement);
        rv = new IR::BlockStatement(statements);
        statements.clear();
    }
    visit(statement->cases, "cases");
    prune();
    return rv;
}

void DoSimplifyExpressions::end_apply(const IR::Node *) {
    BUG_CHECK(toInsert.empty(), "DoSimplifyExpressions::end_apply orphaned declarations");
    BUG_CHECK(statements.empty(), "DoSimplifyExpressions::end_apply orphaned statements");
}

///////////////////////////////////////////////

const IR::Node *KeySideEffect::preorder(IR::Key *key) {
    // If any key field has side effects then pull out all
    // the key field values.
    LOG3("Visiting " << key);
    bool complex = false;
    for (auto k : key->keyElements)
        complex = complex || P4::SideEffects::check(k->expression, this, refMap, typeMap);
    if (!complex)
        // This prune will prevent the postoder(IR::KeyElement*) below from executing
        prune();
    else
        LOG3("Will pull out " << key);
    return key;
}

const IR::Node *KeySideEffect::postorder(IR::KeyElement *element) {
    // If we got here we need to pull the key element out.
    LOG3("Extracting key element " << element);
    auto table = findOrigCtxt<IR::P4Table>();
    CHECK_NULL(table);
    TableInsertions *insertions;
    auto it = toInsert.find(table);
    if (it == toInsert.end()) {
        insertions = new TableInsertions();
        toInsert.emplace(table, insertions);
    } else {
        insertions = it->second;
    }

    auto tmp = refMap->newName("key");
    auto type = typeMap->getType(element->expression, true);
    auto decl = new IR::Declaration_Variable(tmp, type, nullptr);
    insertions->declarations.push_back(decl);
    auto left = new IR::PathExpression(tmp);
    auto right = element->expression;
    auto assign = new IR::AssignmentStatement(element->expression->srcInfo, left, right);
    insertions->statements.push_back(assign);

    auto path = new IR::PathExpression(tmp);
    // This preserves annotations on the key
    element->expression = path;
    LOG2("Created new key expression " << element);
    return element;
}

const IR::Node *KeySideEffect::preorder(IR::P4Table *table) {
    auto orig = getOriginal<IR::P4Table>();
    if (invokedInKey->find(orig) != invokedInKey->end()) {
        // if this table is invoked in some key computation do not
        // analyze its key yet; we will do this in a future iteration.
        LOG2("Will not analyze key of " << table);
        prune();
    }
    return table;
}

const IR::Node *KeySideEffect::postorder(IR::P4Table *table) {
    auto insertions = ::get(toInsert, getOriginal<IR::P4Table>());
    if (insertions == nullptr) return table;

    auto result = new IR::IndexedVector<IR::Declaration>();
    for (auto d : insertions->declarations) result->push_back(d);
    result->push_back(table);
    return result;
}

const IR::Node *KeySideEffect::doStatement(const IR::Statement *statement,
                                           const IR::Expression *expression) {
    LOG3("Visiting " << getOriginal());
    HasTableApply hta(refMap, typeMap);
    hta.setCalledBy(this);
    (void)expression->apply(hta);
    if (hta.table == nullptr) return statement;
    auto insertions = get(toInsert, hta.table);
    if (insertions == nullptr) return statement;

    auto result = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto assign : insertions->statements) {
        auto cloneAssign = assign->clone();
        result->push_back(cloneAssign);
    }
    result->push_back(statement);
    auto block = new IR::BlockStatement(*result);
    return block;
}

}  // namespace P4
