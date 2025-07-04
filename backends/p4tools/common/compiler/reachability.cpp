#include "backends/p4tools/common/compiler/reachability.h"

#include <cstddef>
#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/table_utils.h"
#include "ir/declaration.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4::P4Tools {

using namespace P4::literals;

P4ProgramDCGCreator::P4ProgramDCGCreator(NodesCallGraph *dcg) : dcg(dcg), p4program(nullptr) {
    CHECK_NULL(dcg);
    setName("P4ProgramDCGCreator");
    visitDagOnce = false;
}

bool P4ProgramDCGCreator::preorder(const IR::Annotation *annotation) {
    if (annotation->name != IR::Annotation::nameAnnotation) return true;

    IR::ID name;
    if (!annotation->getExpr().empty()) {
        if (const auto *strLit = annotation->getExpr(0)->to<IR::StringLiteral>()) {
            if (strLit->value == ".NoAction") {
                // Ignore NoAction annotations, because they aren't unique.
                return true;
            }
            name = IR::ID(strLit->value);
        }
    }
    addEdge(annotation, name);
    return annotation != nullptr;
}

bool P4ProgramDCGCreator::preorder(const IR::ConstructorCallExpression *callExpr) {
    const auto *p4Type = callExpr->constructedType->getP4Type();
    if (const auto *typeName = p4Type->to<IR::Type_Name>()) {
        const auto *decl = p4program->getDeclsByName(typeName->path->name.name)->single();
        visit(decl->checkedTo<IR::Type_Declaration>());
    } else {
        visit(p4Type);
    }
    return true;
}

bool P4ProgramDCGCreator::preorder(const IR::MethodCallExpression *call) {
    CHECK_NULL(call->method);
    for (const auto *arg : *call->arguments) {
        visit(arg);
    }
    if (call->method->type->is<IR::Type_Action>()) {
        const auto *path = call->method->checkedTo<IR::PathExpression>();
        const auto *action = getDeclaration(path->path, true)->checkedTo<IR::P4Action>();
        addEdge(call);
        visit(action);
        return false;
    }
    if (call->method->type->is<IR::Type_Method>()) {
        if (const auto *path = call->method->to<IR::PathExpression>()) {
            const auto *method = getDeclaration(path->path, true)->checkedTo<IR::Method>();
            visit(method);
            return false;
        }
        if (const auto *method = call->method->to<IR::Member>()) {
            // Case where call->method is a Member expression. For table invocations, the
            // qualifier of the member determines the table being invoked. For extern calls,
            // the qualifier determines the extern object containing the method being invoked.
            BUG_CHECK(method->expr, "Method call has unexpected format: %1%", call);

            // Handle table calls.
            if (method->expr->type->is<IR::Type_Table>()) {
                const auto *tableDecl =
                    getDeclaration(method->expr->checkedTo<IR::PathExpression>()->path, true)
                        ->checkedTo<IR::P4Table>();
                visit(tableDecl);
                return false;
            }

            // Handle extern calls. They may also be of Type_SpecializedCanonical.
            if (method->expr->type->is<IR::Type_Extern>() ||
                method->expr->type->is<IR::Type_SpecializedCanonical>()) {
                // TODO: This is the wrong place to analyze parser value sets.
                // They should be handled in the select expression.
                if (method->member.originalName == "index") {
                    const auto *decl =
                        getDeclaration(method->expr->checkedTo<IR::PathExpression>()->path, true)
                            ->checkedTo<IR::Declaration_Instance>();
                    visit(decl);
                }
                return false;
            }

            // Handle calls to header methods.
            if (method->expr->type->is<IR::Type_Header>() ||
                method->expr->type->is<IR::Type_HeaderUnion>()) {
                if (method->member == IR::Type_Header::isValid || IR::Type_Header::setInvalid ||
                    method->member == IR::Type_Header::setValid) {
                    return false;
                }
                BUG("Unknown method call on header instance: %1%", call);
            }

            if (method->expr->type->is<IR::Type_Array>()) {
                if (method->member == IR::Type_Array::push_front ||
                    method->member == IR::Type_Array::pop_front) {
                    return false;
                }
                BUG("Unknown method call on stack instance: %1%", call);
            }

            BUG("Unknown method member expression: %1% of type %2%", method->expr,
                method->expr->type);
        }

        BUG("Unknown method call: %1% of type %2%", call->method, call->method->node_type_name());
    }

    BUG("Unsupported method call type for %1%: %2%", call, call->method->type);
}

bool P4ProgramDCGCreator::preorder(const IR::MethodCallStatement *method) {
    addEdge(method);
    for (const auto *arg : *method->methodCall->arguments) {
        visit(arg);
    }
    visit(method->methodCall);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Action *action) {
    addEdge(action, action->name);
    for (const auto *annotation : action->getAnnotations()) {
        visit(annotation);
    }
    visit(action->body);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Parser *parser) {
    addEdge(parser, parser->name);
    visit(parser->states.getDeclaration<IR::ParserState>("start"_cs));
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Table *table) {
    addEdge(table, table->name);
    for (const auto *annotation : table->getAnnotations()) visit(annotation);
    bool wasImplementations = false;
    if (table->properties != nullptr) {
        for (const auto *property : table->properties->properties) {
            if (property->name.name == "implementation") {
                visit(property->annotations);
                wasImplementations = true;
            }
        }
    }
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(*table, properties);

    auto storedSet = prev;
    DCGVertexTypeSet prevSet;
    if (properties.tableIsImmutable) {
        // We can only match on entries when there are keys present.
        if (table->getKey() != nullptr) {
            const auto *entryList = table->getEntries();
            if (entryList != nullptr) {
                for (const auto *entry : entryList->entries) {
                    prev = storedSet;
                    visit(entry);
                    prevSet.insert(prev.begin(), prev.end());
                }
            } else if (wasImplementations) {
                prevSet.insert(prev.begin(), prev.end());
            }
        }
        // If the default action is immutable, we can only match on the default action.
        if (properties.defaultIsImmutable) {
            prev = storedSet;
            visit(table->getDefaultAction());
            prevSet.insert(prev.begin(), prev.end());
            prev = prevSet;
            return false;
        }
    }
    for (const auto *action : table->getActionList()->actionList) {
        prev = storedSet;
        visit(action);
        prevSet.insert(prev.begin(), prev.end());
    }

    prev = prevSet;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::ParserState *parserState) {
    addEdge(parserState, parserState->name);
    visited.insert(parserState);
    for (const auto *annotation : parserState->annotations) {
        visit(annotation);
    }
    for (const auto *component : parserState->components) {
        visit(component);
    }
    if (parserState->selectExpression != nullptr) {
        if (const auto *pathExpr = parserState->selectExpression->to<IR::PathExpression>()) {
            const auto *declaration = getDeclaration(pathExpr->path)->checkedTo<IR::ParserState>();

            BUG_CHECK(declaration != nullptr, "Parser state not found: %1%",
                      pathExpr->path->name.name);
            if (visited.count(declaration) != 0U) {
                addEdge(declaration, declaration->name);
                return false;
            }
            visit(declaration);
        } else {
            visit(parserState->selectExpression);
        }
    }
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Control *control) {
    addEdge(control, control->name);
    visit(control->body);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Program *program) {
    LOG1("Analyzing " << program);
    p4program = program;
    const auto mainCount = program->getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");
    const auto *mainIDecl = program->getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program->main);
    const auto *declInstance = mainIDecl->to<IR::Declaration_Instance>();
    std::vector<const IR::ConstructorCallExpression *> v;
    for (const auto *arg : *declInstance->arguments) {
        const auto *expr = arg->expression;
        if (const auto *ctorCall = expr->template to<IR::ConstructorCallExpression>()) {
            v.push_back(ctorCall);
            continue;
        }

        if (const auto *pathExpr = expr->template to<IR::PathExpression>()) {
            // Look up the path expression in the declaration map, and expect to find a
            // declaration instance.
            auto filter = [pathExpr](const IR::IDeclaration *d) {
                CHECK_NULL(d);
                if (const auto *decl = d->to<IR::Declaration_Instance>()) {
                    return pathExpr->path->name == decl->name;
                }
                return false;
            };
            // Convert the declaration instance into a constructor-call expression.
            const auto *decl =
                program->getDeclarations()->where(filter)->single()->to<IR::Declaration_Instance>();
            v.push_back(
                new IR::ConstructorCallExpression(decl->srcInfo, decl->type, decl->arguments));
            continue;
        }
    }
    this->prev = {program};
    for (const auto *arg : v) {
        // Visit the blocks in order of the constructor arguments.
        visit(arg);
    }
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4ValueSet *valueSet) {
    addEdge(valueSet, valueSet->name);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::SelectExpression *selectExpression) {
    visit(selectExpression->select);
    DCGVertexTypeSet prevSet;
    const auto *currentParser = findContext<IR::P4Parser>();
    BUG_CHECK(currentParser != nullptr, "Null parser pointer");
    auto storedSet = prev;
    for (const auto *selectCase : selectExpression->selectCases) {
        prev = storedSet;
        visit(selectCase->keyset);
        const auto *parserState = currentParser->states.getDeclaration<IR::ParserState>(
            selectCase->state->path->name.name);

        if (visited.count(parserState) != 0U) {
            addEdge(parserState, parserState->name);
            continue;
        }
        visit(parserState);
        prevSet.insert(prev.begin(), prev.end());
    }
    prev = prevSet;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::IfStatement *ifStatement) {
    addEdge(ifStatement);
    auto storedSet = prev;
    visit(ifStatement->condition);
    visit(ifStatement->ifTrue);
    auto next = prev;
    if (ifStatement->ifFalse != nullptr) {
        prev = storedSet;
        visit(ifStatement->ifFalse);
        next.insert(prev.begin(), prev.end());
    } else {
        next.insert(storedSet.begin(), storedSet.end());
    }
    prev = next;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::SwitchStatement *switchStatement) {
    visit(switchStatement->expression);
    DCGVertexTypeSet prevSet;
    auto storedSet = prev;
    for (const auto *switchCase : switchStatement->cases) {
        prev = storedSet;
        visit(switchCase->label);
        visit(switchCase->statement);
        prevSet.insert(prev.begin(), prev.end());
    }
    prev = prevSet;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::StatOrDecl *statOrDecl) {
    if (const auto *block = statOrDecl->to<IR::BlockStatement>()) {
        for (const auto *a : block->annotations) visit(a);
        for (const auto *c : block->components) visit(c);
        return false;
    }
    IR::ID name;
    if (const auto *decl = statOrDecl->to<IR::Declaration>()) {
        name = decl->name;
    }
    addEdge(statOrDecl, name);
    return true;
}

void P4ProgramDCGCreator::addEdge(DCGVertexType vertex, const IR::ID &vertexName) {
    LOG1("Add edge : " << prev.size() << "(" << *prev.begin() << ") : " << vertex);
    for (const auto *p : prev) {
        dcg->calls(p, vertex);
    }
    prev.clear();
    prev.insert(vertex);
    dcg->addToHash(vertex, vertexName);
}

ReachabilityEngineState *ReachabilityEngineState::getInitial() {
    auto *newState = new ReachabilityEngineState();
    newState->prevNode = nullptr;
    newState->state.push_back(nullptr);
    return newState;
}

ReachabilityEngineState *ReachabilityEngineState::copy() {
    auto *newState = new ReachabilityEngineState();
    newState->prevNode = prevNode;
    newState->state = state;
    return newState;
}

std::list<DCGVertexType> ReachabilityEngineState::getState() { return state; }

void ReachabilityEngineState::setState(const std::list<DCGVertexType> &ls) { state = ls; }

DCGVertexType ReachabilityEngineState::getPrevNode() { return prevNode; }

void ReachabilityEngineState::setPrevNode(DCGVertexType n) { prevNode = n; }

bool ReachabilityEngineState::isEmpty() { return state.empty(); }

void ReachabilityEngineState::clear() { state.clear(); }

ReachabilityEngine::ReachabilityEngine(const NodesCallGraph &dcg,
                                       std::string reachabilityExpression,
                                       bool eliminateAnnotations)
    : dcg(dcg), hash(dcg.getHash()) {
    std::list<DCGVertexType> start;
    start.push_back(nullptr);
    size_t i = 0;
    size_t j = 0;
    reachabilityExpression += ";";
    while ((i = reachabilityExpression.find(';')) != std::string::npos) {
        auto addSubExpr = reachabilityExpression.substr(0, i);
        addSubExpr += "+";
        std::list<DCGVertexType> newStart;
        while ((j = addSubExpr.find('+')) != std::string::npos) {
            auto dotSubExpr = addSubExpr.substr(0, j);
            while (dotSubExpr[0] == ' ') {
                dotSubExpr.erase(0, 1);
            }
            bool isForbidden = dotSubExpr[0] == '!';
            if (isForbidden) {
                dotSubExpr.erase(0, 1);
            }
            auto currentNames = getName(dotSubExpr);
            if (eliminateAnnotations) {
                std::unordered_set<DCGVertexType> result;
                for (auto i : currentNames) {
                    if (!i->is<IR::Annotation>()) {
                        result.insert(i);
                        continue;
                    }
                    annotationToStatements(i, result);
                }
                currentNames = result;
            }
            if (isForbidden) {
                forbiddenVertexes.insert(currentNames.begin(), currentNames.end());
                newStart.insert(newStart.end(), start.begin(), start.end());
            } else {
                for (const auto *l : start) {
                    addTransition(l, currentNames);
                }
                newStart.insert(newStart.end(), currentNames.begin(), currentNames.end());
            }
            addSubExpr = addSubExpr.substr(j + 1);
        }
        start = newStart;
        reachabilityExpression = reachabilityExpression.substr(i + 1);
    }
}

void ReachabilityEngine::annotationToStatements(DCGVertexType node,
                                                std::unordered_set<DCGVertexType> &s) {
    std::list<DCGVertexType> l = {node};
    while (!l.empty()) {
        const auto *nd = l.front();
        l.pop_front();
        if (nd->is<IR::Statement>() || nd->is<IR::P4Program>() || nd->is<IR::P4Control>() ||
            nd->is<IR::ParserState>()) {
            s.insert(nd);
            continue;
        }
        // TODO: This is nasty. I am not sure why a const cast should be allowed here.
        const auto *v = const_cast<NodesCallGraph *>(&dcg)->getCallers(nd);
        if (v != nullptr) {
            if (!(*v->begin())->is<IR::MethodCallStatement>() &&
                nd->is<IR::MethodCallExpression>()) {
                s.insert(nd);
                continue;
            }
            l.insert(l.begin(), v->begin(), v->end());
        }
    }
}

void ReachabilityEngine::addTransition(DCGVertexType left,
                                       const std::unordered_set<DCGVertexType> &rightSet) {
    for (const auto *right : rightSet) {
        auto i = userTransitions.find(left);
        if (i == userTransitions.end()) {
            std::list<DCGVertexType> l;
            l.push_back(right);
            userTransitions.emplace(left, l);
        } else {
            i->second.push_back(right);
        }
    }
}

const IR::Expression *ReachabilityEngine::addCondition(const IR::Expression *prev,
                                                       DCGVertexType currentState) {
    const auto *newCond = getCondition(currentState);
    if (newCond == nullptr) {
        return prev;
    }
    return new IR::BOr(IR::Type_Boolean::get(), newCond, prev);
}

std::unordered_set<DCGVertexType> ReachabilityEngine::getName(std::string name) {
    std::string params;
    if ((name.length() != 0U) && name[name.length() - 1] == ')') {
        size_t n = name.find('(');
        BUG_CHECK(n != std::string::npos, "Invalid format : %1%", name);
        params = name.substr(n + 1, name.length() - n - 2);
        name = name.substr(0, n);
    }
    size_t n;
    while ((n = name.find(" ")) != std::string::npos) {
        name.erase(n, 1);
    }
    auto i = hash.find(name);
    if (i == hash.end()) {
        name += "_table";
        i = hash.find(name);
    }
    BUG_CHECK(i != hash.end(), "Can't find name %1% in hash of the program", name);
    if (params.length() != 0U) {
        // Add condition to a point.
        const auto *expr = stringToNode(params);
        for (const auto *j : i->second) {
            auto k = conditions.find(j);
            if (k == conditions.end()) {
                conditions.emplace(j, expr);
            } else {
                k->second = new IR::LAnd(IR::Type_Boolean::get(), expr, k->second);
            }
        }
    }
    return i->second;
}

ReachabilityResult ReachabilityEngine::next(ReachabilityEngineState *state, DCGVertexType next) {
    CHECK_NULL(state);
    CHECK_NULL(next);
    if (forbiddenVertexes.count(next)) {
        return std::make_pair(false, nullptr);
    }
    if (state->isEmpty()) {
        return std::make_pair(true, nullptr);
    }
    if (state->getPrevNode() == nullptr) {
        state->setPrevNode(next);
    } else if (dcg.isReachable(state->getPrevNode(), next)) {
        // Check to move in the same direction.
        state->setPrevNode(next);
    } else {
        return std::make_pair(false, nullptr);
    }
    const IR::Expression *expr = nullptr;
    std::list<DCGVertexType> newState;
    std::list<DCGVertexType> currentState = state->getState();
    for (const auto *i : currentState) {
        if (i == nullptr) {
            // Start from intial.
            auto j = userTransitions.find(i);
            if (j == userTransitions.end()) {
                // No user's transitions were found.
                state->clear();
                return std::make_pair(true, nullptr);
            }
            for (const auto *k : j->second) {
                if (next == k) {
                    // Checking next states.
                    auto m = userTransitions.find(k);
                    if (m == userTransitions.end()) {
                        // No next state found.
                        state->clear();
                        return std::make_pair(true, getCondition(k));
                    }
                    for (const auto *n : m->second) {
                        expr = addCondition(expr, n);
                        newState.push_back(n);
                    }
                } else if (dcg.isReachable(next, k)) {
                    expr = addCondition(expr, k);
                    newState.push_back(k);
                }
            }
        } else if (i == next) {
            auto m = userTransitions.find(i);
            if (m == userTransitions.end()) {
                // No next state found.
                state->clear();
                return std::make_pair(true, getCondition(i));
            }
            for (const auto *n : m->second) {
                expr = addCondition(expr, n);
                newState.push_back(n);
            }
        } else if (dcg.isReachable(next, i)) {
            expr = addCondition(expr, i);
            newState.push_back(i);
        }
    }
    state->setState(newState);
    return std::make_pair(!state->isEmpty(), expr);
}

const NodesCallGraph &ReachabilityEngine::getDCG() { return dcg; }

const IR::Expression *ReachabilityEngine::getCondition(DCGVertexType n) {
    auto i = conditions.find(n);
    if (i != conditions.end()) {
        return i->second;
    }
    return nullptr;
}

const IR::Expression *ReachabilityEngine::stringToNode(std::string /*name*/) {
    P4C_UNIMPLEMENTED("Converting a string into an IR::Expression");
}

}  // namespace P4::P4Tools
