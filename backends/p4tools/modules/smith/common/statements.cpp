#include "backends/p4tools/modules/smith/common/statements.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <sstream>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools::P4Smith {

IR::Statement *StatementGenerator::genStatement(bool is_in_func) {
    // functions can!have exit statements so set their probability to zero
    int64_t pctExit = PCT.STATEMENT_EXIT;
    if (is_in_func) {
        pctExit = 0;
    }
    std::vector<int64_t> percent = {PCT.STATEMENT_SWITCH,
                                    PCT.STATEMENT_ASSIGNMENTORMETHODCALL,
                                    PCT.STATEMENT_IF,
                                    PCT.STATEMENT_RETURN,
                                    pctExit,
                                    PCT.STATEMENT_BLOCK};
    IR::Statement *stmt = nullptr;
    bool useDefaultStmt = false;

    switch (Utils::getRandInt(percent)) {
        case 0: {
            stmt = genSwitchStatement();
            if (stmt == nullptr) {
                useDefaultStmt = true;
            }
            break;
        }
        case 1: {
            useDefaultStmt = true;
            break;
        }
        case 2: {
            stmt = genConditionalStatement(is_in_func);
            break;
        }
        case 3: {
            stmt = genReturnStatement(P4Scope::prop.ret_type);
            break;
        }
        case 4: {
            stmt = genExitStatement();
            break;
        }
        case 5: {
            stmt = genBlockStatement(is_in_func);
            break;
        }
    }
    if (useDefaultStmt) {
        stmt = genAssignmentOrMethodCallStatement(is_in_func);
    }
    return stmt;
}

IR::IndexedVector<IR::StatOrDecl> StatementGenerator::genBlockStatementHelper(bool is_in_func) {
    // Randomize the total number of statements.
    int maxStatements =
        Utils::getRandInt(DECL.BLOCKSTATEMENT_MIN_STAT, DECL.BLOCKSTATEMENT_MAX_STAT);
    IR::IndexedVector<IR::StatOrDecl> statOrDecls;

    // Put tab_name .apply() after some initializations.
    for (int numStat = 0; numStat <= maxStatements; numStat++) {
        IR::StatOrDecl *stmt =
            target().declarationGenerator().generateRandomStatementOrDeclaration(is_in_func);
        if (stmt == nullptr) {
            BUG("Statement in BlockStatement should not be nullptr!");
        }
        statOrDecls.push_back(stmt);
    }
    return statOrDecls;
}

IR::BlockStatement *StatementGenerator::genBlockStatement(bool is_in_func) {
    P4Scope::startLocalScope();

    auto statOrDecls = genBlockStatementHelper(is_in_func);

    if (is_in_func && (P4Scope::prop.ret_type->to<IR::Type_Void>() == nullptr)) {
        auto *retStat = genReturnStatement(P4Scope::prop.ret_type);
        statOrDecls.push_back(retStat);
    }
    P4Scope::endLocalScope();

    return new IR::BlockStatement(statOrDecls);
}

IR::IfStatement *StatementGenerator::genConditionalStatement(bool is_in_func) {
    IR::Expression *cond = nullptr;
    IR::Statement *ifTrue = nullptr;
    IR::Statement *ifFalse = nullptr;

    cond = target().expressionGenerator().genExpression(IR::Type_Boolean::get());
    if (cond == nullptr) {
        BUG("cond in IfStatement should !be nullptr!");
    }
    ifTrue = genStatement(is_in_func);
    if (ifTrue == nullptr) {
        // could !generate a statement
        // this happens when there is now way to generate an assignment
        ifTrue = new IR::EmptyStatement();
    }
    ifFalse = genStatement(is_in_func);
    if (ifFalse == nullptr) {
        // could !generate a statement
        // this happens when there is now way to generate an assignment
        ifFalse = new IR::EmptyStatement();
    }
    return new IR::IfStatement(cond, ifTrue, ifFalse);
}

void StatementGenerator::removeLval(const IR::Expression *left, const IR::Type *type) {
    cstring lvalStr = nullptr;
    if (const auto *path = left->to<IR::PathExpression>()) {
        lvalStr = path->path->name.name;
    } else if (const auto *mem = left->to<IR::Member>()) {
        lvalStr = mem->member.name;
    } else if (const auto *slice = left->to<IR::Slice>()) {
        lvalStr = slice->e0->to<IR::PathExpression>()->path->name.name;
    } else if (const auto *arrIdx = left->to<IR::ArrayIndex>()) {
        lvalStr = arrIdx->left->to<IR::PathExpression>()->path->name.name;
    }

    P4Scope::deleteLval(type, lvalStr);
}

IR::Statement *StatementGenerator::genAssignmentStatement() {
    IR::AssignmentStatement *assignstat = nullptr;
    IR::Expression *left = nullptr;
    IR::Expression *right = nullptr;

    std::vector<int64_t> percent = {PCT.ASSIGNMENTORMETHODCALLSTATEMENT_ASSIGN_BIT,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_ASSIGN_STRUCTLIKE};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            const auto *bitType = P4Scope::pickDeclaredBitType(true);
            // Ideally this should have a fallback option
            if (bitType == nullptr) {
                LOG3("Could not find writable bit lval for assignment!\n");
                // TODO(fruffy): Find a more meaningful assignment statement
                return nullptr;
            }
            left = target().expressionGenerator().pickLvalOrSlice(bitType);
            if (P4Scope::constraints.single_stage_actions) {
                removeLval(left, bitType);
            }
            right = target().expressionGenerator().genExpression(bitType);
            return new IR::AssignmentStatement(left, right);
        }
        case 1:
            // TODO(fruffy): Compound types
            break;
    }

    return assignstat;
}

IR::Statement *StatementGenerator::genMethodCallExpression(const IR::PathExpression *methodName,
                                                           const IR::ParameterList &params) {
    auto *args = new IR::Vector<IR::Argument>();
    IR::IndexedVector<IR::StatOrDecl> decls;

    // all this boilerplate should be somewhere else...
    P4Scope::startLocalScope();

    for (const auto *par : params) {
        IR::Argument *arg = nullptr;
        // TODO(fruffy): Fix the direction none issue here.
        if (!target().expressionGenerator().checkInputArg(par) &&
            par->direction != IR::Direction::None) {
            auto name = cstring(getRandomString(6));
            auto *expr = target().expressionGenerator().genExpression(par->type);
            // all this boilerplate should be somewhere else...
            auto *decl = new IR::Declaration_Variable(name, par->type, expr);
            P4Scope::addToScope(decl);
            decls.push_back(decl);
        }
        arg = new IR::Argument(target().expressionGenerator().genInputArg(par));
        args->push_back(arg);
    }
    auto *mce = new IR::MethodCallExpression(methodName, args);
    auto *mcs = new IR::MethodCallStatement(mce);
    P4Scope::endLocalScope();
    if (decls.empty()) {
        return mcs;
    }
    auto *blkStmt = new IR::BlockStatement(decls);
    blkStmt->push_back(mcs);
    return blkStmt;
}

IR::Statement *StatementGenerator::genMethodCallStatement(bool is_in_func) {
    IR::MethodCallExpression *mce = nullptr;

    // functions cannot call actions or tables so set their chance to zero
    int16_t tmpActionPct = PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_ACTION;
    int16_t tmpTblPct = PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_TABLE;
    int16_t tmpCtrlPct = PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CTRL;
    if (is_in_func) {
        PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_ACTION = 0;
        PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_TABLE = 0;
    }
    if (P4Scope::prop.in_action) {
        PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CTRL = 0;
    }
    std::vector<int64_t> percent = {PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_ACTION,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_FUNCTION,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_TABLE,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CTRL,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_BUILT_IN};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            auto actions = P4Scope::getDecls<IR::P4Action>();
            if (actions.empty()) {
                break;
            }
            size_t idx = Utils::getRandInt(0, actions.size() - 1);
            const auto *p4Fun = actions.at(idx);
            auto params = p4Fun->getParameters()->parameters;
            auto *methodName = new IR::PathExpression(p4Fun->name);
            return genMethodCallExpression(methodName, params);
        }
        case 1: {
            auto funcs = P4Scope::getDecls<IR::Function>();
            if (funcs.empty()) {
                break;
            }
            size_t idx = Utils::getRandInt(0, funcs.size() - 1);
            const auto *p4Fun = funcs.at(idx);
            auto params = p4Fun->getParameters()->parameters;
            auto *methodName = new IR::PathExpression(p4Fun->name);
            return genMethodCallExpression(methodName, params);
        }
        case 2: {
            auto *tblSet = P4Scope::getCallableTables();
            if (tblSet->empty()) {
                break;
            }
            auto idx = Utils::getRandInt(0, tblSet->size() - 1);
            auto tblIter = std::begin(*tblSet);
            std::advance(tblIter, idx);
            const IR::P4Table *tbl = *tblIter;
            auto *mem = new IR::Member(new IR::PathExpression(tbl->name), "apply");
            mce = new IR::MethodCallExpression(mem);
            tblSet->erase(tblIter);
            break;
        }
        case 3: {
            auto decls = P4Scope::getDecls<IR::Declaration_Instance>();
            if (decls.empty()) {
                break;
            }
            auto idx = Utils::getRandInt(0, decls.size() - 1);
            auto declIter = std::begin(decls);
            std::advance(declIter, idx);
            const IR::Declaration_Instance *declInstance = *declIter;
            // avoid member here
            std::stringstream tmpMethodStr;
            tmpMethodStr << declInstance->name << ".apply";
            cstring tmpMethodCstr(tmpMethodStr.str());
            auto *methodName = new IR::PathExpression(tmpMethodCstr);
            const auto *typeName = declInstance->type->to<IR::Type_Name>();

            const auto *resolvedType = P4Scope::getTypeByName(typeName->path->name);
            if (resolvedType == nullptr) {
                BUG("Type Name %s not found", typeName->path->name);
            }
            if (const auto *ctrl = resolvedType->to<IR::P4Control>()) {
                auto params = ctrl->getApplyParameters()->parameters;
                return genMethodCallExpression(methodName, params);
            }
            BUG("Declaration Instance type %s not yet supported",
                declInstance->type->node_type_name());
        }
        case 4: {
            auto hdrs = P4Scope::getDecls<IR::Type_Header>();
            if (hdrs.empty()) {
                break;
            }
            std::set<cstring> hdrLvals;
            for (const auto *hdr : hdrs) {
                auto availableLvals = P4Scope::getCandidateLvals(hdr, true);
                hdrLvals.insert(availableLvals.begin(), availableLvals.end());
            }
            if (hdrLvals.empty()) {
                break;
            }
            auto idx = Utils::getRandInt(0, hdrLvals.size() - 1);
            auto hdrLvalIter = std::begin(hdrLvals);
            std::advance(hdrLvalIter, idx);
            cstring hdrLval = *hdrLvalIter;
            cstring call;
            if (Utils::getRandInt(0, 1) != 0) {
                call = "setValid";
            } else {
                call = "setInvalid";
            }
            auto *mem = new IR::Member(new IR::PathExpression(hdrLval), call);
            mce = new IR::MethodCallExpression(mem);
            break;
        }
    }
    // restore previous probabilities
    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_ACTION = tmpActionPct;
    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_TABLE = tmpTblPct;
    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CTRL = tmpCtrlPct;
    if (mce != nullptr) {
        return new IR::MethodCallStatement(mce);
    }
    // unable to return a methodcall, return an assignment instead
    return genAssignmentStatement();
}

IR::Statement *StatementGenerator::genAssignmentOrMethodCallStatement(bool is_in_func) {
    std::vector<int64_t> percent = {PCT.ASSIGNMENTORMETHODCALLSTATEMENT_ASSIGN,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CALL};
    auto val = Utils::getRandInt(percent);
    if (val == 0) {
        return genAssignmentStatement();
    }
    return genMethodCallStatement(is_in_func);
}

IR::ExitStatement *StatementGenerator::genExitStatement() { return new IR::ExitStatement(); }

IR::SwitchStatement *StatementGenerator::genSwitchStatement() {
    // get the expression
    auto *tblSet = P4Scope::getCallableTables();

    // return nullptr if there are no tables left
    if (tblSet->empty()) {
        return nullptr;
    }
    auto idx = Utils::getRandInt(0, tblSet->size() - 1);
    auto tblIter = std::begin(*tblSet);

    std::advance(tblIter, idx);
    const IR::P4Table *tbl = *tblIter;
    auto *expr = new IR::Member(
        new IR::MethodCallExpression(new IR::Member(new IR::PathExpression(tbl->name), "apply")),
        "action_run");
    tblSet->erase(tblIter);

    // get the switch cases
    IR::Vector<IR::SwitchCase> switchCases;
    for (const auto *tabProperty : tbl->properties->properties) {
        if (tabProperty->name.name == IR::TableProperties::actionsPropertyName) {
            const auto *property = tabProperty->value->to<IR::ActionList>();
            for (const auto *action : property->actionList) {
                cstring actName = action->getName();
                auto *blkStat = genBlockStatement(false);
                auto *switchCase = new IR::SwitchCase(new IR::PathExpression(actName), blkStat);
                switchCases.push_back(switchCase);
            }
        }
    }

    return new IR::SwitchStatement(expr, switchCases);
}

IR::ReturnStatement *StatementGenerator::genReturnStatement(const IR::Type *tp) {
    IR::Expression *expr = nullptr;

    // Type_Void is also empty
    if ((tp != nullptr) && (tp->to<IR::Type_Void>() == nullptr)) {
        expr = target().expressionGenerator().genExpression(tp);
    }
    return new IR::ReturnStatement(expr);
}

}  // namespace P4Tools::P4Smith
