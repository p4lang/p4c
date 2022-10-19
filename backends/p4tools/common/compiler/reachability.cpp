#include "backends/p4tools/common/compiler/reachability.h"

#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

namespace P4Tools {

P4ProgramDCGCreator::P4ProgramDCGCreator(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                                         NodesCallGraph* dcg)
    : dcg(dcg), typeMap(typeMap), refMap(refMap), p4program(nullptr) {
    CHECK_NULL(dcg);
    setName("P4ProgramDCGCreator");
    visitDagOnce = false;
}

bool P4ProgramDCGCreator::preorder(const IR::Annotation* annotation) {
    if (annotation->name.name != IR::Annotation::nameAnnotation) {
        return true;
    }
    IR::ID name;
    if (!annotation->expr.empty()) {
        if (const auto* strLit = annotation->expr[0]->to<IR::StringLiteral>()) {
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

bool P4ProgramDCGCreator::preorder(const IR::ConstructorCallExpression* callExpr) {
    const auto* p4Type = callExpr->constructedType->getP4Type();
    if (const auto* typeName = p4Type->to<IR::Type_Name>()) {
        const auto* decl = p4program->getDeclsByName(typeName->path->name.name)->single();
        visit(decl->checkedTo<IR::Type_Declaration>());
    } else {
        visit(p4Type);
    }
    return true;
}

bool P4ProgramDCGCreator::preorder(const IR::MethodCallExpression* method) {
    // Check for application of a table.
    CHECK_NULL(method->method);
    if (const auto* path = method->method->to<IR::PathExpression>()) {
        const auto* currentControl = findOrigCtxt<IR::P4Control>();
        if (currentControl != nullptr) {
            const auto* decl = currentControl->getDeclByName(path->path->name.name);
            if (decl != nullptr) {
                if (const auto* action = decl->to<IR::P4Action>()) {
                    for (const auto* arg : *method->arguments) {
                        visit(arg);
                    }
                    addEdge(method);
                    visit(action);
                    return false;
                }
            }
        }
    }
    if (!method->method->is<IR::Member>()) {
        return true;
    }
    const auto* member = method->method->to<IR::Member>();
    for (const auto* arg : *method->arguments) {
        visit(arg);
    }
    // Do not anylyse tables apply and value set index methods.
    if (member->member != IR::IApply::applyMethodName && member->member.originalName != "index") {
        return true;
    }
    if (const auto* pathExpr = member->expr->to<IR::PathExpression>()) {
        const auto* currentControl = findOrigCtxt<IR::P4Control>();
        if (currentControl != nullptr) {
            const auto* decl = currentControl->getDeclByName(pathExpr->path->name.name);
            visit(decl->checkedTo<IR::P4Table>());
            return false;
        }
        const auto* parser = findOrigCtxt<IR::P4Parser>();
        if (parser != nullptr) {
            const auto* decl = parser->getDeclByName(pathExpr->path->name.name);
            visit(decl->checkedTo<IR::Declaration_Instance>());
            return true;
        }
        return true;
    }
    const auto* type = member->expr->type;
    if (const auto* tableType = type->to<IR::Type_Table>()) {
        visit(tableType->table);
        return false;
    }
    return true;
}

bool P4ProgramDCGCreator::preorder(const IR::MethodCallStatement* method) {
    addEdge(method);
    for (const auto* arg : *method->methodCall->arguments) {
        visit(arg);
    }
    visit(method->methodCall);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Action* action) {
    addEdge(action, action->name);
    for (const auto* annotation : action->annotations->annotations) {
        visit(annotation);
    }
    visit(action->body);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Parser* parser) {
    addEdge(parser, parser->name);
    visit(parser->states.getDeclaration<IR::ParserState>("start"));
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Table* table) {
    addEdge(table, table->name);
    DCGVertexTypeSet prevSet;
    if (table->annotations != nullptr) {
        for (const auto* annotation : table->annotations->annotations) {
            visit(annotation);
        }
    }
    bool wasImplementations = false;
    if (table->properties != nullptr) {
        for (const auto* property : table->properties->properties) {
            if (property->name.name == "implementation") {
                visit(property->annotations->annotations);
                wasImplementations = true;
            }
        }
    }
    auto storedSet = prev;
    const auto* entryList = table->getEntries();
    if (entryList != nullptr) {
        for (const auto* entry : entryList->entries) {
            prev = storedSet;
            visit(entry);
            prevSet.insert(prev.begin(), prev.end());
        }
    } else if (wasImplementations) {
        prevSet.insert(prev.begin(), prev.end());
    }
    prev = storedSet;
    visit(table->getDefaultAction());
    prevSet.insert(prev.begin(), prev.end());
    prev = prevSet;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::ParserState* parserState) {
    addEdge(parserState, parserState->name);
    visited.insert(parserState);
    if (parserState->annotations != nullptr) {
        for (const auto* annotation : parserState->annotations->annotations) {
            visit(annotation);
        }
    }
    for (const auto* component : parserState->components) {
        visit(component);
    }
    if (parserState->selectExpression != nullptr) {
        if (const auto* pathExpr = parserState->selectExpression->to<IR::PathExpression>()) {
            if (pathExpr->path->name.name == IR::ParserState::accept ||
                pathExpr->path->name.name == IR::ParserState::reject) {
                addEdge(parserState->selectExpression);
                return true;
            }
        }
        if (visited.count(parserState->selectExpression) != 0U) {
            return false;
        }
        visit(parserState->selectExpression);
    }
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Control* control) {
    addEdge(control, control->name);
    visit(control->body);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4Program* program) {
    LOG1("Analyzing " << program);
    p4program = program;
    const auto mainCount = program->getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");
    const auto* mainIDecl = program->getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program->main);
    const auto* declInstance = mainIDecl->to<IR::Declaration_Instance>();
    std::vector<const IR::ConstructorCallExpression*> v;
    for (const auto* arg : *declInstance->arguments) {
        const auto* expr = arg->expression;
        if (const auto* ctorCall = expr->template to<IR::ConstructorCallExpression>()) {
            v.push_back(ctorCall);
            continue;
        }

        if (const auto* pathExpr = expr->template to<IR::PathExpression>()) {
            // Look up the path expression in the declaration map, and expect to find a
            // declaration instance.
            std::function<bool(const IR::IDeclaration*)> filter =
                [pathExpr](const IR::IDeclaration* d) {
                    CHECK_NULL(d);
                    const auto* decl = d->to<IR::Declaration_Instance>();
                    if (decl == nullptr) {
                        return false;
                    }
                    return pathExpr->path->name == decl->name;
                };
            const auto* declVector = program->getDeclarations()->where(filter)->toVector();
            BUG_CHECK(!declVector->empty(), "Not a declaration instance: %1%", pathExpr);

            // Convert the declaration instance into a constructor-call expression.
            const auto* decl = declVector->at(0)->to<IR::Declaration_Instance>();
            auto* ctorCall =
                new IR::ConstructorCallExpression(decl->srcInfo, decl->type, decl->arguments);
            v.push_back(ctorCall);
            continue;
        }
    }
    this->prev = {program};
    for (const auto* arg : v) {
        // Apply to the arguments.,
        visit(arg);
    }
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::P4ValueSet* valueSet) {
    addEdge(valueSet, valueSet->name);
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::SelectExpression* selectExpression) {
    visit(selectExpression->select);
    DCGVertexTypeSet prevSet;
    const auto* currentParser = findOrigCtxt<IR::P4Parser>();
    BUG_CHECK(currentParser != nullptr, "Null parser pointer");
    auto storedSet = prev;
    for (const auto* selectCase : selectExpression->selectCases) {
        prev = storedSet;
        visit(selectCase->keyset);
        const auto* parserState = currentParser->states.getDeclaration<IR::ParserState>(
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

bool P4ProgramDCGCreator::preorder(const IR::IfStatement* ifStatement) {
    addEdge(ifStatement);
    auto storedSet = prev;
    visit(ifStatement->condition);
    visit(ifStatement->ifTrue);
    auto next = prev;
    if (ifStatement->ifFalse != nullptr) {
        prev = storedSet;
        visit(ifStatement->ifFalse);
        next.insert(prev.begin(), prev.end());
    }
    prev = next;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::SwitchStatement* switchStatement) {
    visit(switchStatement->expression);
    DCGVertexTypeSet prevSet;
    auto storedSet = prev;
    for (const auto* switchCase : switchStatement->cases) {
        prev = storedSet;
        visit(switchCase->label);
        visit(switchCase->statement);
        prevSet.insert(prev.begin(), prev.end());
    }
    prev = prevSet;
    return false;
}

bool P4ProgramDCGCreator::preorder(const IR::StatOrDecl* statOrDecl) {
    if (const auto* block = statOrDecl->to<IR::BlockStatement>()) {
        if (block->annotations != nullptr) {
            for (const auto* a : block->annotations->annotations) {
                visit(a);
            }
        }
        for (const auto* c : block->components) {
            visit(c);
        }
        return false;
    }
    IR::ID name;
    if (const auto* decl = statOrDecl->to<IR::Declaration>()) {
        name = decl->name;
    }
    addEdge(statOrDecl, name);
    return true;
}

void P4ProgramDCGCreator::addEdge(const DCGVertexType* vertex, IR::ID vertexName) {
    LOG1("Add edge : " << prev.size() << "(" << *prev.begin() << ") : " << vertex);
    for (const auto* p : prev) {
        dcg->calls(p, vertex);
    }
    prev.clear();
    prev.insert(vertex);
    dcg->addToHash(vertex, vertexName);
}

}  // namespace P4Tools
