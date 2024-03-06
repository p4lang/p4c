#include "backends/p4tools/modules/smith/common/statements.h"

#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statementOrDeclaration.h"
#include "backends/p4tools/modules/smith/util/util.h"

namespace P4Tools::P4Smith {

IR::Statement *Statements::genStatement(bool is_in_func) {
    // functions can!have exit statements so set their probability to zero
    int64_t pct_exit = PCT.STATEMENT_EXIT;
    if (is_in_func) {
        pct_exit = 0;
    }
    std::vector<int64_t> percent = {PCT.STATEMENT_SWITCH,
                                    PCT.STATEMENT_ASSIGNMENTORMETHODCALL,
                                    PCT.STATEMENT_IF,
                                    PCT.STATEMENT_RETURN,
                                    pct_exit,
                                    PCT.STATEMENT_BLOCK};
    IR::Statement *stmt = nullptr;
    bool use_default_stmt = false;

    switch (randInt(percent)) {
        case 0: {
            stmt = genSwitchStatement();
            if (!stmt) {
                use_default_stmt = true;
            }
            break;
        }
        case 1: {
            use_default_stmt = true;
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
    if (use_default_stmt) {
        stmt = genAssignmentOrMethodCallStatement(is_in_func);
    }
    return stmt;
}

IR::IndexedVector<IR::StatOrDecl> Statements::genBlockStatementHelper(bool is_in_func) {
    // Randomize the total number of statements.
    int max_statements = getRndInt(DECL.BLOCKSTATEMENT_MIN_STAT, DECL.BLOCKSTATEMENT_MAX_STAT);
    IR::IndexedVector<IR::StatOrDecl> stat_or_decls;

    // Put tab_name .apply() after some initializations.
    for (int num_stat = 0; num_stat <= max_statements; num_stat++) {
        IR::StatOrDecl *stmt = statementOrDeclaration::gen_rnd(is_in_func);
        if (stmt == nullptr) {
            BUG("Statement in BlockStatement should not be nullptr!");
        }
        stat_or_decls.push_back(stmt);
    }
    return stat_or_decls;
}

IR::BlockStatement *Statements::genBlockStatement(bool is_in_func) {
    P4Scope::start_local_scope();

    auto stat_or_decls = genBlockStatementHelper(is_in_func);

    if (is_in_func && !P4Scope::prop.ret_type->to<IR::Type_Void>()) {
        auto ret_stat = genReturnStatement(P4Scope::prop.ret_type);
        stat_or_decls.push_back(ret_stat);
    }
    P4Scope::end_local_scope();

    return new IR::BlockStatement(stat_or_decls);
}

IR::IfStatement *Statements::genConditionalStatement(bool is_in_func) {
    IR::Expression *cond = nullptr;
    IR::Statement *if_true = nullptr, *if_false = nullptr;

    cond = Expressions().genExpression(new IR::Type_Boolean());
    if (!cond) {
        BUG("cond in IfStatement should !be nullptr!");
    }
    if_true = genStatement(is_in_func);
    if (!if_true) {
        // could !generate a statement
        // this happens when there is now way to generate an assignment
        if_true = new IR::EmptyStatement();
    }
    if_false = genStatement(is_in_func);
    if (!if_false) {
        // could !generate a statement
        // this happens when there is now way to generate an assignment
        if_false = new IR::EmptyStatement();
    }
    return new IR::IfStatement(cond, if_true, if_false);
}

void Statements::removeLval(IR::Expression *left, IR::Type *type) {
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

    P4Scope::delete_lval(type, lvalStr);
}

IR::Statement *Statements::genAssignmentStatement() {
    IR::AssignmentStatement *assignstat = nullptr;
    IR::Expression *left = nullptr;
    IR::Expression *right = nullptr;

    std::vector<int64_t> percent = {PCT.ASSIGNMENTORMETHODCALLSTATEMENT_ASSIGN_BIT,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_ASSIGN_STRUCTLIKE};

    switch (randInt(percent)) {
        case 0: {
            IR::Type_Bits *bitType = P4Scope::pick_declared_bit_type(true);
            // Ideally this should have a fallback option
            if (bitType == nullptr) {
                LOG3("Could not find writable bit lval for assignment!\n");
                // TODO: Find a more meaningful assignment statement
                return nullptr;
            }
            left = P4Scope::pick_lval_or_slice(bitType);
            if (P4Scope::constraints.single_stage_actions) {
                removeLval(left, bitType);
            }
            right = Expressions().genExpression(bitType);
            return new IR::AssignmentStatement(left, right);
        }
        case 1:
            // TODO: Compound types
            break;
    }

    return assignstat;
}

IR::Statement *Statements::genMethodCallExpression(IR::PathExpression *methodName,
                                                   IR::ParameterList params) {
    auto *args = new IR::Vector<IR::Argument>();
    IR::IndexedVector<IR::StatOrDecl> decls;

    // all this boilerplate should be somewhere else...
    P4Scope::start_local_scope();

    for (const auto *par : params) {
        IR::Argument *arg;
        // TODO: Fix the direction none issue here.
        if (!Expressions().checkInputArg(par) && par->direction != IR::Direction::None) {
            auto name = cstring(randStr(6));
            auto expr = Expressions().genExpression(par->type);
            // all this boilerplate should be somewhere else...
            auto decl = new IR::Declaration_Variable(name, par->type, expr);
            P4Scope::add_to_scope(decl);
            decls.push_back(decl);
        }
        arg = new IR::Argument(Expressions().genInputArg(par));
        args->push_back(arg);
    }
    auto *mce = new IR::MethodCallExpression(methodName, args);
    auto *mcs = new IR::MethodCallStatement(mce);
    P4Scope::end_local_scope();
    if (decls.empty()) {
        return mcs;
    } else {
        auto blkStmt = new IR::BlockStatement(decls);
        blkStmt->push_back(mcs);
        return blkStmt;
    }
}

IR::Statement *Statements::genMethodCallStatement(bool is_in_func) {
    IR::MethodCallExpression *mce = nullptr;

    // functions cannot call actions or tables so set their chance to zero
    int16_t tmp_action_pct = PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_ACTION;
    int16_t tmp_tbl_pct = PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_TABLE;
    int16_t tmp_ctrl_pct = PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CTRL;
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

    switch (randInt(percent)) {
        case 0: {
            auto actions = P4Scope::get_decls<IR::P4Action>();
            if (actions.empty()) {
                break;
            }
            size_t idx = getRndInt(0, actions.size() - 1);
            auto p4Fun = actions.at(idx);
            auto params = p4Fun->getParameters()->parameters;
            auto methodName = new IR::PathExpression(p4Fun->name);
            return genMethodCallExpression(methodName, params);
        }
        case 1: {
            auto funcs = P4Scope::get_decls<IR::Function>();
            if (funcs.size() == 0) {
                break;
            }
            size_t idx = getRndInt(0, funcs.size() - 1);
            auto p4Fun = funcs.at(idx);
            auto params = p4Fun->getParameters()->parameters;
            auto *methodName = new IR::PathExpression(p4Fun->name);
            return genMethodCallExpression(methodName, params);
        }
        case 2: {
            auto *tbl_set = P4Scope::get_callable_tables();
            if (tbl_set->empty()) {
                break;
            }
            auto idx = getRndInt(0, tbl_set->size() - 1);
            auto tblIter = std::begin(*tbl_set);
            std::advance(tblIter, idx);
            const IR::P4Table *tbl = *tblIter;
            auto *mem = new IR::Member(new IR::PathExpression(tbl->name), "apply");
            mce = new IR::MethodCallExpression(mem);
            tbl_set->erase(tblIter);
            break;
        }
        case 3: {
            auto decls = P4Scope::get_decls<IR::Declaration_Instance>();
            if (decls.empty()) {
                break;
            }
            auto idx = getRndInt(0, decls.size() - 1);
            auto decl_iter = std::begin(decls);
            std::advance(decl_iter, idx);
            const IR::Declaration_Instance *decl_instance = *decl_iter;
            // avoid member here
            std::stringstream tmp_method_str;
            tmp_method_str << decl_instance->name << ".apply";
            cstring tmpMethodCstr(tmp_method_str.str());
            auto *methodName = new IR::PathExpression(tmpMethodCstr);
            const auto *typeName = decl_instance->type->to<IR::Type_Name>();

            const auto *resolvedType = P4Scope::get_type_by_name(typeName->path->name);
            if (resolvedType == nullptr) {
                BUG("Type Name %s not found", typeName->path->name);
            }
            if (const auto *ctrl = resolvedType->to<IR::P4Control>()) {
                auto params = ctrl->getApplyParameters()->parameters;
                return genMethodCallExpression(methodName, params);
            }
            BUG("Declaration Instance type %s not yet supported",
                decl_instance->type->node_type_name());
        }
        case 4: {
            auto hdrs = P4Scope::get_decls<IR::Type_Header>();
            if (hdrs.empty()) {
                break;
            }
            std::set<cstring> hdrLvals;
            for (const auto *hdr : hdrs) {
                auto availableLvals = P4Scope::get_candidate_lvals(hdr, true);
                hdrLvals.insert(availableLvals.begin(), availableLvals.end());
            }
            if (hdrLvals.empty()) {
                break;
            }
            auto idx = getRndInt(0, hdrLvals.size() - 1);
            auto hdrLvalIter = std::begin(hdrLvals);
            std::advance(hdrLvalIter, idx);
            cstring hdrLval = *hdrLvalIter;
            cstring call;
            if (getRndInt(0, 1) != 0) {
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
    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_ACTION = tmp_action_pct;
    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_TABLE = tmp_tbl_pct;
    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CTRL = tmp_ctrl_pct;
    if (mce != nullptr) {
        return new IR::MethodCallStatement(mce);
    }
    // unable to return a methodcall, return an assignment instead
    return genAssignmentStatement();
}

IR::Statement *Statements::genAssignmentOrMethodCallStatement(bool is_in_func) {
    std::vector<int64_t> percent = {PCT.ASSIGNMENTORMETHODCALLSTATEMENT_ASSIGN,
                                    PCT.ASSIGNMENTORMETHODCALLSTATEMENT_METHOD_CALL};
    auto val = randInt(percent);
    if (val == 0) {
        return genAssignmentStatement();
    }
    return genMethodCallStatement(is_in_func);
}

IR::ExitStatement *Statements::genExitStatement() { return new IR::ExitStatement(); }

IR::SwitchStatement *Statements::genSwitchStatement() {
    // get the expression
    auto tbl_set = P4Scope::get_callable_tables();

    // return nullptr if there are no tables left
    if (tbl_set->size() == 0) {
        return nullptr;
    }
    auto idx = getRndInt(0, tbl_set->size() - 1);
    auto tbl_iter = std::begin(*tbl_set);

    std::advance(tbl_iter, idx);
    const IR::P4Table *tbl = *tbl_iter;
    auto expr = new IR::Member(
        new IR::MethodCallExpression(new IR::Member(new IR::PathExpression(tbl->name), "apply")),
        "action_run");
    tbl_set->erase(tbl_iter);

    // get the switch cases
    IR::Vector<IR::SwitchCase> switch_cases;
    for (auto tab_property : tbl->properties->properties) {
        if (tab_property->name.name == IR::TableProperties::actionsPropertyName) {
            auto property = tab_property->value->to<IR::ActionList>();
            for (auto action : property->actionList) {
                cstring act_name = action->getName();
                auto blk_stat = genBlockStatement();
                IR::SwitchCase *switch_case =
                    new IR::SwitchCase(new IR::PathExpression(act_name), blk_stat);
                switch_cases.push_back(switch_case);
            }
        }
    }

    return new IR::SwitchStatement(expr, switch_cases);
}

IR::ReturnStatement *Statements::genReturnStatement(const IR::Type *tp) {
    IR::Expression *expr = nullptr;

    // Type_Void is also empty
    if (tp && !tp->to<IR::Type_Void>()) {
        expr = Expressions().genExpression(tp);
    }
    return new IR::ReturnStatement(expr);
}

}  // namespace P4Tools::P4Smith
