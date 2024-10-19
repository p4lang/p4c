/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_REGISTER_READ_WRITE_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_REGISTER_READ_WRITE_H_

#include "bf-p4c/midend/copy_header.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "table_mutex.h"

namespace BFN {

class SearchAndReplaceExpr : public Transform {
    IR::Expression *replace;
    const IR::Expression *search;

    IR::Node *preorder(IR::Expression *expr) {
        if (expr->toString() == search->toString()) return replace;
        return expr;
    }

 public:
    SearchAndReplaceExpr(IR::Expression *replace, const IR::Expression *search)
        : replace(replace), search(search) {}
};

// Action -> register -> read/write statements
// the action is either a P4Action or a Declaration_Instance representing a RegisterAction extern
typedef std::unordered_map<const IR::Declaration *,
                           std::unordered_map<const IR::Declaration_Instance *,  // Register
                                                                                 // declaration
                                              ordered_set<const IR::Statement *>>>
    RegisterCallsByAction;
typedef std::unordered_map<const IR::Declaration *,
                           std::unordered_map<const IR::Declaration_Instance *,  // Register
                                                                                 // declaration
                                              IR::MethodCallExpression *>>
    RegisterExecuteCallByAction;
typedef std::unordered_map<const IR::P4Control *, ordered_set<IR::Declaration_Instance *>>
    RegisterActionsByControl;
typedef std::unordered_map<const IR::Declaration *,  // Register declaration
                           std::unordered_set<const IR::Declaration *>>
    ActionsByRegister;

/**
 * \ingroup stateful_alu
 * \brief The pass replaces the Register.read/write() calls with register actions.
 *
 * The pass must be invoked prefereably towards the end of mid-end to account for
 * any mid-end optimizations like constant folding, propogation, etc.
 *
 * The subpass CheckRegisterActions checks the limit of number of %RegisterActions attached
 * to a single %Register; <b>not only of the %RegisterActions introduced
 * by the RegisterReadWrite pass</b>.
 */
class RegisterReadWrite : public PassManager {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    RegisterCallsByAction action_register_calls;
    RegisterExecuteCallByAction action_register_exec_calls;
    RegisterActionsByControl control_register_actions;
    ActionsByRegister generated_register_actions;
    ActionsByRegister actions_using_register;
    TableMutex table_mutex;

    /**
     * \ingroup stateful_alu
     * \brief The pass checks the limit of number of %RegisterActions attached
     * to a single %Register.
     *
     * The check is performed for all %RegisterActions, not only for those introduced
     * by the RegisterReadWrite pass.
     */
    class CheckRegisterActions : public Inspector {
        RegisterReadWrite &self;
        std::unordered_map<const IR::Declaration *,  // Register -> RegisterAction
                           std::vector<const IR::Declaration *>>
            all_register_actions;

     public:
        explicit CheckRegisterActions(RegisterReadWrite &self) : self(self) {}

        bool preorder(const IR::Declaration_Instance *) override;
        void end_apply() override;
    };

    /**
     * \ingroup stateful_alu
     * \brief Using maps built in BFN::RegisterReadWrite::CollectRegisterReadsWrites
     * and BFN::RegisterReadWrite::AnalyzeActionWithRegisterCalls passes, it adds newly
     * created register actions to the IR and replaces Register.read/write() calls with
     * RegisterAction.execute() calls.
     */
    class UpdateRegisterActionsAndExecuteCalls : public Transform {
        RegisterReadWrite &self;
        std::map<const IR::P4Control *, IR::Declaration_Instance *> register_actions;
        IR::Node *preorder(IR::P4Action *) override;
        IR::Node *preorder(IR::Declaration_Instance *) override;
        IR::Node *postorder(IR::P4Control *ctrl) override;

        bool processDeclaration(const IR::Declaration *action, const IR::BlockStatement *&body)
            __attribute__((__warn_unused_result__));

     public:
        explicit UpdateRegisterActionsAndExecuteCalls(RegisterReadWrite &self) : self(self) {}
    };

    /**
     * \ingroup stateful_alu
     * \brief Using the map built in the BFN::RegisterReadWrite::CollectRegisterReadsWrites
     * pass, this pass prepares corresponding register actions and Register.execute() calls.
     *
     * The pass builds two maps:
     * 1. BFN::RegisterReadWrite::control_register_actions, which contains info about
     *    which newly created register actions will be placed in which control block.
     * 2. BFN::RegisterReadWrite::action_register_exec_calls, which contains info about
     *    which newly created RegisterAction.execute() calls will be placed in which
     *    actions.
     */
    class AnalyzeActionWithRegisterCalls : public Inspector {
        RegisterReadWrite &self;
        struct RegActionInfo {
            // infomation and IR about RegisterAction being created to implement a
            // Register read+write
            IR::Declaration_Instance *reg_action = nullptr;
            IR::ParameterList *apply_params = nullptr;
            IR::BlockStatement *apply_body = nullptr;
            IR::MethodCallExpression *execute_call = nullptr;
            const IR::Expression *read_expr = nullptr;
        };
        bool preorder(const IR::Declaration *) override;

        void createRegisterExecute(RegActionInfo &reg_info, const IR::Statement *reg_stmt,
                                   cstring action_name);
        void createRegisterAction(RegActionInfo &reg_info, const IR::Statement *reg_stmt,
                                  const IR::Declaration *act);

     public:
        explicit AnalyzeActionWithRegisterCalls(RegisterReadWrite &self) : self(self) {}
    };

    /**
     * \ingroup stateful_alu
     * \brief The pass builds the BFN::RegisterReadWrite::action_register_calls map that
     * contains info about which action contains which statements with Register.read/write()
     * calls.
     */
    class CollectRegisterReadsWrites : public Inspector {
        RegisterReadWrite &self;
        bool preorder(const IR::MethodCallExpression *) override;
        void end_apply() override;
        void collectRegReadWrite(const IR::MethodCallExpression *, const IR::Declaration *);

     public:
        explicit CollectRegisterReadsWrites(RegisterReadWrite &self) : self(self) {}
    };

    /**
     * \ingroup stateful_alu
     * \brief An auxiliary pass that moves declarations of register parameters
     * to the very beginning of corresponding control block.
     *
     * If not done, the following passes would place register actions
     * (which might use the register parameters) in front of the declarations
     * of the register parameters, which would cause missing Declaration_Instance error.
     *
     * @pre This sub-pass needs to be run before all other sub-passes
     * of the BFN::RegisterReadWrite pass.
     */
    class MoveRegisterParameters : public Modifier {
        RegisterReadWrite &self;
        bool preorder(IR::P4Control *c) override;

     public:
        explicit MoveRegisterParameters(RegisterReadWrite &self) : self(self) {}
    };

 public:
    RegisterReadWrite(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                      BFN::TypeChecking *typeChecking = nullptr)
        : refMap(refMap), typeMap(typeMap) {
        if (!typeChecking) typeChecking = new BFN::TypeChecking(refMap, typeMap);
        addPasses({typeChecking, &table_mutex, new MoveRegisterParameters(*this),
                   new CollectRegisterReadsWrites(*this), new AnalyzeActionWithRegisterCalls(*this),
                   new UpdateRegisterActionsAndExecuteCalls(*this), new P4::ClearTypeMap(typeMap),
                   new CopyHeaders(refMap, typeMap, typeChecking), new P4::ClearTypeMap(typeMap),
                   typeChecking, new CheckRegisterActions(*this)});
    }

    static std::pair<const IR::MethodCallExpression * /*call*/,
                     const IR::Expression * /*read_expr*/>
    extractRegisterReadWrite(const IR::Statement *reg_stmt);
};

}  // namespace BFN

#endif  // BACKENDS_TOFINO_BF_P4C_MIDEND_REGISTER_READ_WRITE_H_
