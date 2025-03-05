/*
Copyright 2025-present Altera Corporation.

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

#include "splitter.h"

#include <utility>
#include <vector>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir-traversal.h"
#include "ir/visitor.h"

namespace P4 {

struct StatementSplitter : Inspector, ResolutionContext {
    StatementSplitter(
        std::function<bool(const IR::Statement *, const Visitor::Context *)> predicate,
        P4::NameGenerator &nameGen, P4::TypeMap *typeMap,
        absl::flat_hash_set<P4::cstring, Util::Hash> &neededDecls)
        : predicate(predicate), nameGen(nameGen), typeMap(typeMap), neededDecls(neededDecls) {}

    bool preorder(const IR::LoopStatement *) override {
        BUG("Loops not supported in statement splitter, must be unrolled before");
    }

    bool preorder(const IR::Statement *stmt) override {
        handleStmt(stmt);
        return false;
    }

    bool preorder(const IR::BlockStatement *bs) override {
        if (handleStmt(bs)) {
            // split on the bs itself
            return false;
        }

        for (size_t i = 0, sz = bs->components.size(); i < sz; i++) {
            visit(bs->components[i], "vector");
            if (result.after) {
                const auto [before, after, _] = result;  // copy
                auto *copy = bs->clone();
                copy->components.erase(copy->components.begin() + i, copy->components.end());
                if (before) {
                    copy->components.push_back(before);
                }
                result.before = filterDeclarations(copy);
                copy = bs->clone();
                copy->components.erase(copy->components.begin(), copy->components.begin() + i);
                collectNeededDeclarations(copy);
                copy->components.replace(copy->components.begin(), after);
                result.after = copy;
                break;
            }
        }
        return false;
    }

    bool preorder(const IR::IfStatement *ifs) override {
        if (handleStmt(ifs)) {
            return false;  // split on the if itself
        }

        auto [results, anySplit] = splitBranches({ifs->ifTrue, ifs->ifFalse});
        if (!anySplit) {
            return false;
        }

        IR::ID condName{nameGen.newName("cond"), nullptr};
        const auto &si = ifs->srcInfo;
        const auto *decl = new IR::Declaration_Variable(si, condName, IR::Type::Boolean::get());
        result.hoistedDeclarations.push_back(decl);

        const auto *condPE = new IR::PathExpression(si, new IR::Path(si, condName));
        const auto *asgn = new IR::AssignmentStatement(si, condPE, ifs->condition);

        auto *beforeIf = ifs->clone();
        beforeIf->condition = condPE->clone();
        beforeIf->ifTrue = results[0].before;
        beforeIf->ifFalse = results[1].before;
        result.before = new IR::BlockStatement(si, {asgn, beforeIf});

        auto *afterIf = beforeIf->clone();
        afterIf->ifTrue = results[0].after;
        afterIf->ifFalse = results[1].after;
        result.after = afterIf;

        for (auto **trueBranch : {&beforeIf->ifTrue, &afterIf->ifTrue}) {
            if (*trueBranch == nullptr) {
                *trueBranch = new IR::BlockStatement(ifs->ifTrue->srcInfo);
            }
        }
        return false;
    }

    bool preorder(const IR::SwitchStatement *sw) override {
        if (handleStmt(sw)) {
            return false;  // split on the switch itself
        }

        std::vector<const IR::Statement *> branches;
        for (const auto *case_ : sw->cases) {
            branches.push_back(case_->statement);
        }
        auto [results, anySplit] = splitBranches(branches);

        if (!anySplit) {
            return false;
        }

        IR::ID selName{nameGen.newName("selector"), nullptr};
        const auto &si = sw->srcInfo;
        const auto *selType = typeMap ? typeMap->getType(sw->expression) : nullptr;
        selType = selType ? selType : sw->expression->type;
        BUG_CHECK(selType && !selType->is<IR::Type::Unknown>(),
                  "Cannot split switch statement with unknown selector type %1%", sw->expression);
        const auto *decl = new IR::Declaration_Variable(si, selName, selType);
        result.hoistedDeclarations.push_back(decl);

        const auto *selPE = new IR::PathExpression(si, new IR::Path(si, selName));
        const auto *asgn = new IR::AssignmentStatement(si, selPE, sw->expression);

        // ensure we don't accidentally create fallthrough
        for (size_t i = 0; i < branches.size(); ++i) {
            for (const auto **val : {&results[i].before, &results[i].after}) {
                if (!*val && branches[i]) {
                    *val = new IR::BlockStatement(branches[i]->srcInfo);
                }
            }
        }

        auto *beforeSw = sw->clone();
        beforeSw->expression = selPE;
        for (size_t i = 0; i < branches.size(); ++i) {
            setCase(beforeSw, i, results[i].before);
        }
        result.before = new IR::BlockStatement(si, {asgn, beforeSw});

        auto *afterSw = beforeSw->clone();
        for (size_t i = 0; i < branches.size(); ++i) {
            setCase(afterSw, i, results[i].after);
        }
        result.after = afterSw;
        return false;
    }

    void end_apply(const IR::Node *root) override {
        if (!result.before) {
            result.before = root->checkedTo<IR::Statement>();
        }
    }

    SplitResult<IR::Statement> result;

 private:
    bool handleStmt(const IR::Statement *stmt) {
        BUG_CHECK(result.before == nullptr && result.after == nullptr,
                  "More than one leaf statement found: %1% and %2%",
                  result.before ? result.before : result.after, stmt);
        if (predicate(stmt, getChildContext())) {
            result.after = stmt;
            collectNeededDeclarations(stmt);
            return true;
        }
        return false;
    }

    void setCase(IR::SwitchStatement *sw, size_t i, const IR::Statement *value) {
        // note that we can't go all the way to statement as it can be nullptr
        modify(sw, &IR::SwitchStatement::cases, IR::Traversal::Index(i),
               [value](IR::SwitchCase *case_) {
                   case_->statement = value;
                   return case_;
               });
    }

    void takeHoisted(std::vector<const IR::Declaration *> &decls) {
        result.hoistedDeclarations.insert(result.hoistedDeclarations.end(), decls.begin(),
                                          decls.end());
        decls.clear();
    }

    std::pair<std::vector<SplitResult<IR::Statement>>, bool> splitBranches(
        std::vector<const IR::Statement *> branches) {
        std::vector<SplitResult<IR::Statement>> res;
        bool anySplit = false;
        res.reserve(branches.size());

        for (const auto *branch : branches) {
            if (!branch) {
                res.emplace_back();
                continue;
            }
            visit(branch, "branch");
            anySplit = anySplit || result.after;
            if (!result) {
                result.before = branch;
            }
            res.emplace_back(std::move(result));
            result.clear();
        }
        for (auto &[_, __, hoisted] : res) {
            takeHoisted(hoisted);
        }
        return {res, anySplit};
    }

    void collectNeededDeclarations(const IR::Node *after) {
        struct CollectNeededDecls : Inspector, ResolutionContext {
            void postorder(const IR::PathExpression *pe) override {
                // using lower-level resolution to avoid emitting errors for things not found
                if (!resolve(pe->path->name, ResolutionType::Any).empty()) {
                    needed.insert(pe->path->name);
                }
            }

            absl::flat_hash_set<P4::cstring, Util::Hash> needed;
        };

        CollectNeededDecls collect;
        after->apply(collect, getChildContext());
        neededDecls.insert(collect.needed.begin(), collect.needed.end());
    }

    template <typename T>
    const T *filterDeclarations(const T *node) {
        struct FilterDecls : Transform {
            FilterDecls(absl::flat_hash_set<P4::cstring, Util::Hash> &needed,
                        std::vector<const IR::Declaration *> &hoisted)
                : needed(needed), hoisted(hoisted) {}

            const IR::Node *preorder(IR::Declaration_Variable *decl) override {
                if (needed.contains(decl->name)) {
                    hoisted.push_back(decl);
                    return nullptr;
                }
                return decl;
            }

            absl::flat_hash_set<P4::cstring, Util::Hash> &needed;
            std::vector<const IR::Declaration *> &hoisted;
        };

        FilterDecls filter(neededDecls, result.hoistedDeclarations);
        return node->apply(filter)->template checkedTo<T>();
    }

    std::function<bool(const IR::Statement *, const Visitor::Context *)> predicate;
    P4::NameGenerator &nameGen;
    P4::TypeMap *typeMap;
    absl::flat_hash_set<P4::cstring, Util::Hash> &neededDecls;
};

SplitResult<IR::Statement> splitStatementBefore(
    const IR::Statement *stat,
    std::function<bool(const IR::Statement *, const P4::Visitor_Context *)> predicate,
    P4::NameGenerator &nameGen, P4::TypeMap *typeMap) {
    absl::flat_hash_set<P4::cstring, Util::Hash> neededDecls;
    StatementSplitter split(predicate, nameGen, typeMap, neededDecls);
    // no incoming context, declaration resolution will work only within the splitter
    stat->apply(split);
    return split.result;
}

}  // namespace P4
