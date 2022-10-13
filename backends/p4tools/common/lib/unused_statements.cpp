#include "backends/p4tools/common/lib/unused_statements.h"

#include <ostream>
#include <string>

#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/exceptions.h"
#include "lib/exename.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "lib/path.h"

namespace P4Tools {

namespace UnusedStatements {

CollectStatements::CollectStatements(IR::StatementList& output) : statements(output) {}
RemoveStatements::RemoveStatements(IR::Vector<IR::Node> branch) : branch(branch) {}

GenerateBranches::GenerateBranches(
    std::vector<std::tuple<const IR::Node*, size_t, bool>>& output,
    std::vector<std::pair<const IR::Node*, IR::Vector<IR::Node>>>& branches, int testCount)
    : unusedStmt(output), branches(branches), testCount(testCount) {}

std::vector<IR::Vector<IR::Node>> GenerateBranches::genPathChains() {
    std::vector<IR::Vector<IR::Node>> pathChains;
    IR::Vector<IR::Node> pathVec;
    for (auto pathes : branches) {
        for (auto path : pathes.second) {
            pathVec.push_back(path);
        }
    }
    IR::Vector<IR::Node> tmp;
    int i = 0;
    auto s = pathVec.size();
    while (i < s) {
        int k = i;
        while (k < s) {
            int j = k;
            while (j <= s) {
                if ((j - k) > 1) {
                    tmp.push_back(pathVec[i]);
                    int u = 1;
                    int r = j - k;
                    while (u < r) {
                        if (tmp.size() > 0) {
                            if (!(tmp[tmp.size() - 1]->srcInfo == pathVec[k + u]->srcInfo))
                                tmp.push_back(pathVec[k + u]);
                        }
                        u++;
                    }

                    pathChains.push_back(tmp);
                    tmp.clear();
                }
                j++;
            }
            k++;
        }
        i++;
    }

    std::vector<IR::Vector<IR::Node>> result;
    for (int i = 0; i < pathChains.size(); i++) {
        if ((pathChains[i].size() == 1) && (branches.size() > 1)) {
            continue;
        }
        if (i == 0) {
            result.push_back(pathChains[i]);
        } else if (pathChains[i] != pathChains[i - 1]) {
            result.push_back(pathChains[i]);
        }
    }

    return result;
}
std::vector<IR::Vector<IR::Node>> GenerateBranches::genAllPathes(const IR::Node* node,
                                                                 IR::Vector<IR::Node> initialPath) {
    int i = 0;
    auto s = initialPath.size();
    std::vector<IR::Vector<IR::Node>> sortedVec;
    IR::Vector<IR::Node> tmp;
    while (i < s) {
        tmp.push_back(initialPath[i]);
        sortedVec.push_back(tmp);
        tmp.clear();
        int k = i;
        while (k < s) {
            int j = k;
            while (j <= s) {
                if ((j - k) > 1) {
                    tmp.push_back(initialPath[i]);
                    int u = 1;
                    int r = j - k;
                    while (u < r) {
                        tmp.push_back(initialPath[k + u]);
                        u++;
                    }
                    sortedVec.push_back(tmp);
                    tmp.clear();
                }
                j++;
            }
            k++;
        }
        i++;
    }

    std::vector<IR::Vector<IR::Node>> result;
    int size = sortedVec.size();
    if (sortedVec.size() > testCount) {
        size = testCount;
    }
    for (int i = 0; i < size; i++) {
        auto randInt = static_cast<int>(TestgenUtils::getRandBigInt(sortedVec.size()));
        int idx = 0;
        if (randInt != 0) idx = randInt - 1;

        if (idx == 0) {
            result.push_back(sortedVec[idx]);
        } else if (sortedVec[idx] != sortedVec[idx - 1]) {
            result.push_back(sortedVec[idx]);
        }
        sortedVec.erase(sortedVec.begin() + idx);
    }
    return result;
}

bool GenerateBranches::preorder(const IR::Statement* stmt) {
    for (auto tuple : unusedStmt) {
        if (!std::get<0>(tuple)) {
            continue;
        }
        if (!std::get<0>(tuple)->srcInfo) continue;
        if (std::get<0>(tuple)->srcInfo == stmt->srcInfo) {
            if (!std::get<2>(tuple)) {
                continue;
            } else if (auto switchStmt = stmt->to<IR::SwitchStatement>()) {
                if (std::get<0>(tuple)->is<IR::SwitchStatement>()) continue;
                IR::Vector<IR::Node> cases;
                int caseIndex = 0;
                const IR::SwitchCase* selectedCase = nullptr;
                for (auto switchCase : switchStmt->cases) {
                    if ((std::get<1>(tuple) - 1) != caseIndex) {
                        cases.push_back(switchCase);
                    } else {
                        selectedCase = switchCase;
                    }

                    caseIndex += 1;
                }
                auto pathes = genAllPathes(switchStmt, cases);
                IR::Vector<IR::Node> stmts;
                for (auto element : pathes) {
                    auto clone = switchStmt->clone();
                    IR::Vector<IR::SwitchCase> caseVec;
                    if (selectedCase) caseVec.push_back(selectedCase);
                    for (auto switchCase : element) {
                        caseVec.push_back(switchCase->checkedTo<IR::SwitchCase>());
                    }
                    clone->cases = caseVec;
                    stmts.push_back(clone);
                }
                branches.push_back(std::make_pair(switchStmt, stmts));
            } else if (auto ifStmt = stmt->to<IR::IfStatement>()) {
                bool condition = std::get<1>(tuple);
                if (std::get<0>(tuple)->is<IR::IfStatement>()) {
                    if (ifStmt->condition->is<IR::LNot>()) {
                        if (condition) {
                            condition = false;
                        } else {
                            condition = true;
                        }

                    } else if (auto member = ifStmt->condition->to<IR::Member>()) {
                        if (member->member.name == IR::Type_Table::miss) {
                            if (condition) {
                                condition = false;
                            } else {
                                condition = true;
                            }
                        }
                    }
                    if (ifStmt->ifTrue) {
                        if (auto block = ifStmt->ifTrue->to<IR::BlockStatement>()) {
                            for (auto comp : block->components) {
                                if (comp->is<IR::ReturnStatement>()) {
                                    return true;
                                }
                            }
                        }
                    }
                    if (ifStmt->ifFalse) {
                        if (auto block = ifStmt->ifFalse->to<IR::BlockStatement>()) {
                            for (auto comp : block->components) {
                                if (comp->is<IR::ReturnStatement>()) {
                                    return true;
                                }
                            }
                        }
                    }
                }
                IR::Vector<IR::Node> stmts;
                if (condition == 1) {
                    if (ifStmt->ifTrue) {
                        auto clone = ifStmt->clone();
                        stmts.push_back(ifStmt);
                        clone->ifTrue = new IR::BlockStatement();
                        stmts.push_back(clone);
                    }
                } else {
                    if (ifStmt->ifFalse) {
                        auto clone = ifStmt->clone();
                        stmts.push_back(clone);
                        clone->ifFalse = new IR::EmptyStatement();
                        stmts.push_back(clone);
                    }
                }

                branches.push_back(std::make_pair(ifStmt, stmts));
            }
        }
    }
    return true;
}

bool GenerateBranches::preorder(const IR::SelectExpression* expr) {
    for (auto tuple : unusedStmt) {
        if (!std::get<0>(tuple)) continue;
        if (auto selectExpr = std::get<0>(tuple)->to<IR::SelectExpression>()) {
            if (!selectExpr->select) {
                continue;
            }
            if (!selectExpr->select->srcInfo) continue;
            if (selectExpr->select->srcInfo == expr->select->srcInfo) {
                auto caseIndex = 0;
                const IR::SelectCase* selectedCase = nullptr;
                IR::Vector<IR::Node> cases;
                for (auto selectCase : expr->selectCases) {
                    if (caseIndex == (std::get<1>(tuple) - 1)) {
                        selectedCase = selectCase;

                    } else {
                        cases.push_back(selectCase);
                    }
                    caseIndex += 1;
                }
                auto pathes = genAllPathes(expr, cases);
                IR::Vector<IR::Node> stmts;
                for (auto element : pathes) {
                    auto clone = expr->clone();
                    IR::Vector<IR::SelectCase> caseVec;
                    caseVec.push_back(selectedCase);
                    for (auto selectCase : element) {
                        caseVec.push_back(selectCase->checkedTo<IR::SelectCase>());
                    }
                    clone->selectCases = caseVec;
                    stmts.push_back(clone);
                }
                branches.push_back(std::make_pair(expr, stmts));
                return true;
            }
        }
    }
    return true;
}

bool GenerateBranches::preorder(const IR::P4Table* table) {
    for (auto tuple : unusedStmt) {
        if (!std::get<0>(tuple)) {
            continue;
        }
        if (!std::get<0>(tuple)->srcInfo) continue;
        if (std::get<0>(tuple)->srcInfo == table->srcInfo) {
            auto entries = table->getEntries();

            IR::Vector<IR::Node> newEntries;
            const IR::Entry* selectedEntry = nullptr;
            if (std::get<1>(tuple) == 0) {
                if (!entries) {
                    continue;
                }
                for (auto entry : entries->entries) {
                    newEntries.push_back(entry);
                }
            } else {
                int entryIndex = 0;
                for (auto entry : entries->entries) {
                    if ((std::get<1>(tuple) - 1) != entryIndex) {
                        newEntries.push_back(entry);
                    } else {
                        selectedEntry = entry;
                    }
                    entryIndex += 1;
                }
            }
            auto pathes = genAllPathes(table, newEntries);
            IR::Vector<IR::Node> tables;
            for (auto entry : pathes) {
                auto clone = table->clone();
                IR::Vector<IR::Entry> entriesVec;
                if (selectedEntry) entriesVec.push_back(selectedEntry);
                for (auto element : entry) {
                    entriesVec.push_back(element->checkedTo<IR::Entry>());
                }
                auto entryList = new IR::EntriesList(entries->srcInfo, entriesVec);
                IR::IndexedVector<IR::Property> properties;
                for (auto property : clone->properties->properties) {
                    if (property->name.name == "entries") {
                        auto propertyClone = property->clone();
                        propertyClone->value = entryList;
                        properties.push_back(propertyClone);
                    } else {
                        properties.push_back(property);
                    }
                }
                clone->properties = new IR::TableProperties(properties);
                tables.push_back(clone);
            }

            branches.push_back(std::make_pair(table, tables));
        }
    }

    return true;
}

bool CollectStatements::preorder(const IR::AssignmentStatement* stmt) {
    statements.components.push_back(stmt);
    return true;
}

const IR::Node* RemoveStatements::preorder(IR::Node* node) {
    if (!node->srcInfo) return node;
    auto sourceFile = node->srcInfo.getSourceFile();
    if (sourceFile.find("p4include")) {
        if (includes.find(sourceFile) == includes.end()) {
            includes.emplace(sourceFile);
            cstring value = "#include <";
            if (sourceFile.find("core.p4")) {
                value += "core.p4";
            }
            if (sourceFile.find("v1model.p4")) {
                value += "v1model.p4";
            }
            value += ">";
            return new IR::PathExpression(new IR::Path(IR::ID(value)));

        } else {
            return nullptr;
        }
    }

    return node;
}

const IR::Node* RemoveStatements::preorder(IR::SelectExpression* expr) {
    for (auto element : branch) {
        if (!element) {
            continue;
        }
        if (!element->srcInfo) {
            continue;
        }
        if (auto selectExpr = element->to<IR::SelectExpression>()) {
            if (selectExpr->select->srcInfo == expr->select->srcInfo) {
                auto clone = expr->clone();
                clone->selectCases = selectExpr->selectCases;
                return clone;
            }
        }
    }
    return expr;
}

const IR::Node* RemoveStatements::preorder(IR::P4Table* table) {
    for (auto element : branch) {
        if (!element) {
            continue;
        }
        if (!element->srcInfo) {
            continue;
        }
        if (element->srcInfo == table->srcInfo) {
            if (auto tableStmt = element->to<IR::P4Table>()) {
                auto clone = table->clone();
                clone->properties = tableStmt->properties;
                return clone;
            }
        }
    }

    return table;
}

const IR::Node* RemoveStatements::preorder(IR::Statement* stmt) {
    for (auto element : branch) {
        if (!element) {
            continue;
        }
        if (!element->srcInfo) {
            continue;
        }
        if (element->srcInfo == stmt->srcInfo) {
            if (auto ifStmt = element->to<IR::IfStatement>()) {
                auto clone = stmt->checkedTo<IR::IfStatement>()->clone();
                clone->ifTrue = ifStmt->ifTrue;
                clone->ifFalse = ifStmt->ifFalse;
                return clone;
            }
            if (auto switchStmt = element->to<IR::SwitchStatement>()) {
                auto clone = stmt->checkedTo<IR::SwitchStatement>()->clone();
                clone->cases = switchStmt->cases;
                return clone;
            }
        }
    }
    return stmt;
}

bool CollectStatements::preorder(const IR::MethodCallStatement* stmt) {
    statements.components.push_back(stmt);
    return true;
}

bool CollectStatements::preorder(const IR::SwitchStatement* stmt) {
    statements.components.push_back(stmt);
    return true;
}

bool CollectStatements::preorder(const IR::IfStatement* stmt) {
    statements.components.push_back(stmt);
    return true;
}

bool CollectStatements::preorder(const IR::SelectExpression* stmt) {
    statements.components.push_back(stmt);
    return true;
}

bool CollectStatements::preorder(const IR::P4Table* stmt) {
    statements.components.push_back(stmt);
    return true;
}

bool CollectStatements::preorder(const IR::ExitStatement* stmt) {
    statements.components.push_back(stmt);
    return true;
}

std::vector<std::tuple<const IR::Node*, size_t, bool>> getUnusedStatements(
    const std::vector<gsl::not_null<const TraceEvent*>>& traces,
    const IR::StatementList& allstatements, const Coverage::CoverageSet visited) {
    std::vector<std::pair<const IR::Node*, size_t>> sourceInfoNodeList;

    for (const auto trace : traces) {
        if (auto node = trace->getNode()) {
            sourceInfoNodeList.push_back(std::make_pair(node, trace->getIdx()));
        }
    }

    IR::IndexedVector<IR::Node> dirtyComponents;
    for (auto node : allstatements.components) {
        if (auto stmt = node->to<IR::Statement>()) {
            if (visited.count(stmt) == 0) {
                dirtyComponents.push_back(node);
            } else if (node->is<IR::SwitchStatement>()) {
                dirtyComponents.push_back(node);
            }
        } else {
            dirtyComponents.push_back(node);
        }
    }
    std::vector<std::tuple<const IR::Node*, size_t, bool>> clearComponents;
    for (auto node : dirtyComponents) {
        if (auto switchStmt = node->to<IR::SwitchStatement>()) {
            std::tuple<const IR::Node*, size_t, bool> switchStmsResult;
            switchStmsResult = std::make_tuple(node, 0, false);
            int caseIndex = 0;
            for (auto switchCase : switchStmt->cases) {
                for (auto srcInfo : sourceInfoNodeList) {
                    if (!srcInfo.first) {
                        continue;
                    }
                    if (!srcInfo.first->srcInfo) {
                        continue;
                    }
                    if (!switchCase->statement) {
                        continue;
                    }
                    if (switchCase->statement->srcInfo == srcInfo.first->srcInfo) {
                        switchStmsResult = std::make_tuple(node, caseIndex, true);
                    }
                }
                caseIndex += 1;
            }
            clearComponents.push_back(switchStmsResult);
            continue;
        } else if (auto ifstmt = node->to<IR::IfStatement>()) {
            std::tuple<const IR::Node*, size_t, bool> ifStmsResult;
            ifStmsResult = std::make_tuple(node, 0, false);
            for (auto srcInfo : sourceInfoNodeList) {
                if (srcInfo.first->is<IR::IfStatement>()) {
                    if (ifstmt->srcInfo == srcInfo.first->srcInfo) {
                        ifStmsResult = std::make_tuple(node, srcInfo.second, true);
                    }
                }
                if (ifstmt->ifTrue == srcInfo.first) {
                    ifStmsResult = std::make_tuple(node, 0, true);
                    break;
                }
                if (ifstmt->ifFalse == srcInfo.first) {
                    ifStmsResult = std::make_tuple(node, 1, true);
                    break;
                }
            }
            clearComponents.push_back(ifStmsResult);
            continue;
        } else if (auto select = node->to<IR::SelectExpression>()) {
            int caseIndex = 0;
            std::tuple<const IR::Node*, size_t, bool> selectResult;
            for (auto selectCase : select->selectCases) {
                for (auto srcInfo : sourceInfoNodeList) {
                    if (srcInfo.first == selectCase) {
                        selectResult = std::make_tuple(node, caseIndex, true);
                    }
                }
                caseIndex += 1;
            }
            clearComponents.push_back(selectResult);
            continue;
        } else if (auto table = node->to<IR::P4Table>()) {
            auto defaultAction = table->getDefaultAction();
            auto entries = table->getEntries();
            std::tuple<const IR::Node*, size_t, bool> tableResult;
            if (entries) {
                int entryIndex = 1;
                for (auto entry : entries->entries) {
                    for (auto srcInfo : sourceInfoNodeList) {
                        if (srcInfo.first->srcInfo == entry->action->srcInfo) {
                            tableResult = std::make_tuple(node, entryIndex, true);
                            break;
                        }
                        if (srcInfo.first->srcInfo == defaultAction->srcInfo) {
                            tableResult = std::make_tuple(node, 0, true);
                            break;
                        }
                    }
                    entryIndex += 1;
                }
            } else {
                tableResult = std::make_tuple(node, 0, true);
            }
            clearComponents.push_back(tableResult);
            continue;
        } else {
            auto flag = false;
            for (auto srcInfo : sourceInfoNodeList) {
                if (srcInfo.first == node) {
                    clearComponents.push_back(std::make_tuple(node, srcInfo.second, true));
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                clearComponents.push_back(std::make_tuple(node, 0, false));
            }
        }
    }
    return clearComponents;
}

void generateTests(std::vector<std::tuple<const IR::Node*, size_t, bool>> unusedStatements,
                   int testCount, size_t testIdx) {
    auto& options = P4CContext::get().options();
    const auto* program = P4::parseP4File(options);
    std::vector<std::pair<const IR::Node*, IR::Vector<IR::Node>>> branches;
    auto genBranch = UnusedStatements::GenerateBranches(unusedStatements, branches, testCount);

    program->apply(genBranch);
    auto chains = genBranch.genPathChains();

    for (int i = 0; i < testCount; i++) {
        if (chains.size() == 0) {
            break;
        }
        auto randInt = static_cast<int>(TestgenUtils::getRandBigInt(chains.size()));
        int idx = 0;
        if (randInt != 0) idx = randInt - 1;
        IR::Vector<IR::Node> branch = chains[idx];
        chains.erase(chains.begin() + randInt);
        auto newProgram = program->apply(UnusedStatements::RemoveStatements(branch));
        Util::PathName filename(options.file);
        Util::PathName newName(std::to_string(i) + "_SourceTest_" + filename.getBasename() + "_" +
                               std::to_string(testIdx) + "_stf" + "." + filename.getExtension());
        auto result = filename.getFolder().join(newName.toString());
        auto stream = openFile(result.toString(), true);
        if (stream != nullptr) {
            if (Log::verbose())
                std::cerr << "Writing program to " << result.toString() << std::endl;
            P4::ToP4 toP4(stream, Log::verbose(), options.file);
            newProgram->apply(toP4);
            delete stream;  // close the file
        }
    }
}

}  // namespace UnusedStatements

}  // namespace P4Tools
