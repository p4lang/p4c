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

#include "register_read_write.h"

#include "bf-p4c/arch/helpers.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/mau/stateful_alu.h"
#include "frontends/p4/methodInstance.h"

namespace BFN {

bool RegisterReadWrite::CheckRegisterActions::preorder(
    const IR::Declaration_Instance *reg_act_decl_inst) {
    auto *reg_act_ext_inst = dynamic_cast<P4::ExternInstantiation *>(
        P4::ExternInstantiation::resolve(reg_act_decl_inst, self.refMap, self.typeMap));
    if (!reg_act_ext_inst || reg_act_ext_inst->type->name != "RegisterAction") return true;
    auto *reg_path_expr =
        reg_act_ext_inst->constructorArguments->at(0)->expression->to<IR::PathExpression>();
    BUG_CHECK(reg_path_expr != nullptr, "Expected PathExpression");
    auto *reg_decl_inst = getDeclInst(self.refMap, reg_path_expr);
    BUG_CHECK(reg_decl_inst != nullptr, "Expected Declaration_Instance");
    all_register_actions[reg_decl_inst].push_back(reg_act_decl_inst);
    return false;
}

/*
 * Check the number of register actions attached to registers. It cannot exceed 4.
 * This is a Tofino 1/2 HW restriction.
 */
void RegisterReadWrite::CheckRegisterActions::end_apply() {
    for (auto &item : all_register_actions) {
        std::string actions_str;
        if (static_cast<int>(item.second.size()) > Device::statefulAluSpec().MaxInstructions) {
            if (self.generated_register_actions.count(item.first) > 0) {
                // RegisterAction has been created in RegisterReadWrite
                std::string sep;
                for (auto &action : self.generated_register_actions.at(item.first)) {
                    actions_str += sep + action->externalName();
                    sep = ", ";
                }
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                      "%1%: too many actions access the Register\n"
                      "The target architecture limits the number of actions accessing "
                      "a single register to %2%.\n"
                      "Reorganize your code to meet this restriction.\n"
                      "The Register is accessed in the following actions: %3%",
                      item.first, Device::statefulAluSpec().MaxInstructions, actions_str);
            } else {
                // RegisterAction has been instantiated in the P4 code
                std::string sep;
                for (auto &action : item.second) {
                    actions_str += sep + action->externalName();
                    sep = ", ";
                }
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                      "%1%: too many RegisterActions attached to the Register\n"
                      "The target architecture limits the number of RegisterActions attached "
                      "to a single Register to %2%.\n"
                      "Reorganize your code to meet this restriction.\n"
                      "The following RegisterActions are attached to the Register: %3%",
                      item.first, Device::statefulAluSpec().MaxInstructions, actions_str);
            }
        }
    }
}

/**
 * The method checks whether the form of the read/write register call is correct,
 * i.e. has one of the forms described in the error message in this function.
 * @param[in] reg_stmt The read/write register statement that is being checked
 * @return A pair of 1. the read/write method call expression contained in the reg_stmt, and
 *                   2. the left-hand side of the assignment statement of the register read method.
 */
std::pair<const IR::MethodCallExpression * /*call*/, const IR::Expression * /*read_expr*/>
RegisterReadWrite::extractRegisterReadWrite(const IR::Statement *reg_stmt) {
    const IR::MethodCallExpression *call = nullptr;
    const IR::Expression *read_expr = nullptr;

    if (auto reg_mcall_stmt = reg_stmt->to<IR::MethodCallStatement>()) {
        // This is a call to .write or .read without assignment, so not assigning to read_expr
        call = reg_mcall_stmt->methodCall;
    } else if (auto reg_assign_stmt = reg_stmt->to<IR::AssignmentStatement>()) {
        // This is a call to .read or .write that stores the result somewhere, so we need
        // to store the left-hand side of the assignment
        if (auto *slice = reg_assign_stmt->right->to<IR::Slice>()) {
            call = slice->e0->to<IR::MethodCallExpression>();
        } else {
            call = reg_assign_stmt->right->to<IR::MethodCallExpression>();
        }
        read_expr = reg_assign_stmt->left;
    }

    return std::pair<const IR::MethodCallExpression * /*call*/,
                     const IR::Expression * /*read_expr*/>(call, read_expr);
}

bool RegisterReadWrite::UpdateRegisterActionsAndExecuteCalls::processDeclaration(
    const IR::Declaration *orig_act, const IR::BlockStatement *&src_body) {
    // src_body is being updated iteratively for each register used in the action
    LOG1(" updating action: " << orig_act);

    if (self.action_register_calls.count(orig_act) == 0) return false;

    // Iterate over registers used in the action
    for (auto reg : self.action_register_calls[orig_act]) {
        auto calls = reg.second;
        // Temporary body
        auto body = new IR::BlockStatement();
        // The read assignment statement to be remembered
        // The execute call is placed on its position.
        const IR::AssignmentStatement *assign_stmt = nullptr;

        auto src_body_it = src_body->components.begin();

        // Remove register read & write calls in action and remember the read assignment
        while (src_body_it != src_body->components.end() && assign_stmt == nullptr) {
            auto inst = *src_body_it;
            LOG2(" updating action statement: " << inst);
            LOG2(" it's a " << inst->node_type_name());
            if (calls.count(inst->to<IR::Statement>()) == 0) {
                // Add non-read & write statements
                LOG2("  adding a " << inst->to<IR::Statement>() << " to the body");
                body->push_back(inst);
            } else if (assign_stmt == nullptr) {
                // Skip read & write statements and remember the read assignment
                LOG2("  setting the assign_stmt to " << inst->to<IR::AssignmentStatement>());
                assign_stmt = inst->to<IR::AssignmentStatement>();
            }
            src_body_it++;
        }

        BUG_CHECK(self.action_register_exec_calls.count(orig_act) > 0 &&
                      self.action_register_exec_calls[orig_act].count(reg.first) > 0,
                  "No register execution call generated for register read/write in action %1%",
                  orig_act);

        auto reg_exec_call = self.action_register_exec_calls[orig_act][reg.first];

        // Add register execute call in appropriate form
        IR::Statement *stmt = nullptr;
        if (assign_stmt) {
            if (auto *slice = assign_stmt->right->to<IR::Slice>()) {
                stmt =
                    new IR::AssignmentStatement(assign_stmt->getSourceInfo(), assign_stmt->left,
                                                new IR::Slice(reg_exec_call, slice->e1, slice->e2));
            } else {
                stmt = new IR::AssignmentStatement(assign_stmt->getSourceInfo(), assign_stmt->left,
                                                   reg_exec_call);
            }
        } else {
            stmt = new IR::MethodCallStatement(reg_exec_call);
        }
        body->push_back(stmt);

        // Update temporary body with remaining non-register read/write instructions
        while (src_body_it != src_body->components.end()) {
            auto inst = *src_body_it;
            if (calls.count(inst->to<IR::Statement>()) == 0) body->push_back(inst);
            src_body_it++;
        }

        // Operate on updated body in the next iteration
        src_body = body;
    }

    return true;
}

IR::Node *RegisterReadWrite::UpdateRegisterActionsAndExecuteCalls::preorder(IR::P4Action *act) {
    auto p4act = getOriginal<IR::P4Action>();
    auto src_body = p4act->body;
    if (!processDeclaration(p4act, src_body)) return act;

    LOG2(" -> replacing p4 action");
    return new IR::P4Action(act->srcInfo, act->name, p4act->annotations, p4act->parameters,
                            src_body);
}

/*
 * The execute call is placed at the position of the read call with assignment or,
 * if only the write call is present, at the end of the action.
 */
IR::Node *RegisterReadWrite::UpdateRegisterActionsAndExecuteCalls::preorder(
    IR::Declaration_Instance *act) {
    // this pass expects a P4 action or a register action
    auto inst = dynamic_cast<P4::ExternInstantiation *>(
        P4::ExternInstantiation::resolve(act, self.refMap, self.typeMap));
    if (!inst || inst->type->name != "RegisterAction") return act;

    auto regAct = getOriginal<IR::Declaration_Instance>();
    Log::TempIndent indent;
    LOG2("regAct: " << regAct << indent);
    auto applyDecl = regAct->initializer->components[0]->to<IR::Declaration>();

    const IR::BlockStatement *src_body = applyDecl->to<IR::Function>()->body;
    if (!processDeclaration(regAct, src_body)) return act;

    auto *rv = new IR::Declaration_Instance(act->name, regAct->annotations, regAct->type,
                                            regAct->arguments, regAct->initializer);
    LOG2(" -> replacing decl inst with:" << rv);
    return rv;
}

IR::Node *RegisterReadWrite::UpdateRegisterActionsAndExecuteCalls::postorder(IR::P4Control *ctrl) {
    LOG1(" TNA Control: " << ctrl);
    auto orig_ctrl = getOriginal<IR::P4Control>();
    if (self.control_register_actions.count(orig_ctrl) == 0) return ctrl;

    // Add register actions to control block
    auto register_actions = self.control_register_actions[orig_ctrl];
    for (auto register_action : register_actions) {
        int count = 0;
        for (auto local : ctrl->controlLocals) {
            count++;
            auto reg_arg = register_action->arguments->at(0);
            auto reg_name = reg_arg->expression->to<IR::PathExpression>()->path->name;
            if (local->getName() == reg_name) break;
        }
        ctrl->controlLocals.insert(ctrl->controlLocals.begin() + count, register_action);
    }
    return ctrl;
}

// REGISTER READ EXAMPLE
// FROM :
//   @name(".accum") register<bit<16>, bit<10>>(32w1024) accum;
//   @name(".addb1") action addb1(bit<9> port, bit<10> idx) {
//      ig_intr_md_for_tm.ucast_egress_port = port;
//      hdr.data.h1 = accum.read(idx);
//   }
//
// TO :
//   @name(".accum") register<bit<16>, bit<10>>(32w1024) accum;
//   @name(".sful") RegisterAction<bit<16>, bit<32>, bit<16>>(accum) sful_0 = {
//       void apply(inout bit<16> value, out bit<16> rv) {
//           bit<16> in_value_0;
//           in_value_0 = value;
//           rv = in_value_0;
//       }
//   }
//   @name(".addb1") action addb1(bit<9> port, bit<10> idx) {
//      ig_intr_md_for_tm.ucast_egress_port = port;
//      hdr.data.h1 = sful.execute(idx);
//   };
//
// REGISTER WRITE EXAMPLE
// FROM :
//   @name(".accum") register<bit<16>, bit<10>>(32w1024) accum;
//   @name(".addb1") action addb1(bit<9> port, bit<10> idx) {
//      ig_intr_md_for_tm.ucast_egress_port = port;
//      accum.write(idx, hdr.data.h1 + 16w1);
//   }
//
// TO :
//   @name(".accum") register<bit<16>, bit<10>>(32w1024) accum;
//   @name(".accum_register_action")
//   RegisterAction<bit<16>, bit<10>, bit<16>>(accum) accum_register_action = {
//       void apply(inout bit<16> value) {
//           value = hdr.data.h1 + 16w1;
//       }
//   };
//   @name(".addb1") action addb1(bit<9> port, bit<10> idx) {
//      ig_intr_md_for_tm.ucast_egress_port = port;
//      accum_register_action.execute(idx);
//   }
// FIXME -- really should factor out common code between createRegisterAction and
//          createRegisterExecute
void RegisterReadWrite::AnalyzeActionWithRegisterCalls::createRegisterExecute(
    RegActionInfo &info, const IR::Statement *reg_stmt, cstring action_name) {
    BUG_CHECK(reg_stmt, "No register call statment present to analyze in action - %1%",
              action_name);

    auto rv = extractRegisterReadWrite(reg_stmt);
    auto *call = rv.first;
    if (!call) return;

    LOG2("creating register execute call for MethodCallExpression: " << call);

    // Create Register Action - Add to declaration
    auto member = call->method->to<IR::Member>();
    auto reg_path = member->expr->to<IR::PathExpression>()->path;

    // Register read/write calls don not have an index argument for direct
    // registers. This is currently unsupported.
    if (call->arguments->size() == 0) {
        P4C_UNIMPLEMENTED(
            "%s: The method call of read and write on a Register "
            "does not currently support for direct registers.  Please use "
            "DirectRegisterAction to describe direct register programming.",
            call);
    }

    bool is_read = (member->member.name == "read");
    bool is_write = (member->member.name == "write");
    int num_call_args = call->arguments->size();
    BUG_CHECK(is_read || is_write,
              " Invalid indirect register call. Only 'read' or 'write' calls are supported, "
              " but %1% specified",
              member->member.name);
    if (is_read) {
        BUG_CHECK(num_call_args == 1,
                  " Invalid indirect register read call. Needs 1 argument for register index "
                  " but %1% specified",
                  num_call_args);
    }
    if (is_write) {
        BUG_CHECK(num_call_args == 2,
                  " Invalid indirect register write call. Needs 2 arguments for register index "
                  " and write expression only %1% specified",
                  num_call_args);
    }

    if (!info.execute_call) {
        // Create Execute Method Call Expression
        auto regaction = new IR::PathExpression(IR::ID(reg_path->name + "_" + action_name));
        auto method = new IR::Member(regaction, "execute");

        // Use first argument which is the indirect index
        auto args = new IR::Vector<IR::Argument>({call->arguments->at(0)});
        info.execute_call = new IR::MethodCallExpression(method, args);
    }
    LOG5("  " << info.execute_call);
}

void RegisterReadWrite::AnalyzeActionWithRegisterCalls::createRegisterAction(
    RegActionInfo &info, const IR::Statement *reg_stmt, const IR::Declaration *act) {
    BUG_CHECK(reg_stmt, "No register call statment present to analyze in action - ", act);

    auto [call, assign_lhs] = extractRegisterReadWrite(reg_stmt);
    if (!call) return;

    LOG1("creating register action for MethodCallExpression: " << call);

    // Register read/write calls don not have an index argument for direct
    // registers. This is currently unsupported.
    if (call->arguments->size() == 0) {
        P4C_UNIMPLEMENTED(
            "%s: The method call of read and write on a Register "
            "does not currently support for direct registers.  Please use "
            "DirectRegisterAction to describe direct register programming.",
            call);
    }

    // Create Register Action - Add to declaration
    auto member = call->method->to<IR::Member>();
    auto reg_path = member->expr->to<IR::PathExpression>()->path;
    auto reg_decl = self.refMap->getDeclaration(reg_path)->to<IR::Declaration_Instance>();

    bool is_read = (member->member.name == "read");
    bool is_write = (member->member.name == "write");
    int num_call_args = call->arguments->size();
    BUG_CHECK(is_read || is_write,
              " Invalid indirect register call. Only 'read' or 'write' calls are supported, "
              " but %1% specified",
              member->member.name);
    if (is_read) {
        BUG_CHECK(num_call_args == 1,
                  " Invalid indirect register read call. Needs 1 argument for register index "
                  " but %1% specified",
                  num_call_args);
        if (!info.read_expr) {
            // Do not overwrite with returned nullptr if the read expression is already stored
            info.read_expr = assign_lhs;
        }
    }
    if (is_write) {
        BUG_CHECK(num_call_args == 2,
                  " Invalid indirect register write call. Needs 2 arguments for register index "
                  " and write expression only %1% specified",
                  num_call_args);
    }

    auto reg_args = reg_decl->type->to<IR::Type_Specialized>()->arguments;
    auto rtype = reg_args->at(0);
    auto itype = reg_args->at(1);
    auto utype = rtype;
    auto tmp = self.typeMap->getType(rtype);
    if (auto tt = tmp->to<IR::Type_Type>()) tmp = tt->type;
    if (auto stype = tmp->to<IR::Type_StructLike>()) {
        // currently only support structs with 1 or 2 identical fields in registers;
        // backend will flag an error if it is not.
        utype = stype->fields.front()->type;
    }

    auto ratype = new IR::Type_Specialized(new IR::Type_Name("RegisterAction"),
                                           new IR::Vector<IR::Type>({rtype, itype, utype}));

    // Create Register Apply block
    auto *ctor_args = new IR::Vector<IR::Argument>(
        {new IR::Argument(new IR::PathExpression(new IR::Path(reg_path->name)))});

    if (!info.apply_params)
        info.apply_params =
            new IR::ParameterList({new IR::Parameter("value", IR::Direction::InOut, rtype)});

    if (!info.apply_body) info.apply_body = new IR::BlockStatement();
    // For register reads, add a return value
    auto in_value = new IR::PathExpression("in_value");
    auto value = new IR::PathExpression("value");
    if (is_read) {
        info.apply_body->push_back(new IR::Declaration_Variable("in_value", rtype));
        info.apply_body->push_back(new IR::AssignmentStatement(in_value, value));
    } else {
        // For register writes, use second method call argument to update the
        // register value
        auto write_expr = call->arguments->at(1)->expression;
        // If write_expr contains a previous register read expression, replace with
        // in_value which is where the read value is stored inside the register
        // action
        if (info.read_expr) {
            auto *sar = new SearchAndReplaceExpr(in_value, info.read_expr);
            write_expr = write_expr->apply(*sar);
        }
        info.apply_body->push_back(new IR::AssignmentStatement(value, write_expr));
    }
    if (assign_lhs) {
        auto rv = new IR::PathExpression("rv");
        if (!info.apply_params->getParameter("rv"_cs))
            info.apply_params->push_back(new IR::Parameter("rv", IR::Direction::Out, utype));
        info.apply_body->push_back(new IR::AssignmentStatement(rv, value));
    }

    auto apply_name = reg_path->name + "_" + act->name;
    auto *externalName = new IR::StringLiteral(IR::ID("." + apply_name));
    auto *annots = new IR::Annotations();
    annots->addAnnotation(IR::ID("name"), externalName);

    if (!info.reg_action) {
        auto apply_type = new IR::Type_Method(IR::Type_Void::get(), info.apply_params, "apply"_cs);
        auto apply = new IR::Function("apply", apply_type, info.apply_body);
        auto *apply_block = new IR::BlockStatement({apply});
        info.reg_action = new IR::Declaration_Instance(IR::ID(apply_name), annots, ratype,
                                                       ctor_args, apply_block);
    }
    LOG5("  " << info.reg_action);
}

bool RegisterReadWrite::AnalyzeActionWithRegisterCalls::preorder(const IR::Declaration *act) {
    if (!act->is<IR::P4Action>() && !act->is<IR::Declaration_Instance>()) {
        return true;
    }

    if (self.action_register_calls.count(act) == 0) return false;
    Log::TempIndent indent;
    LOG1("RegisterReadWrite: analysing action: " << act << indent);

    auto control = findContext<IR::P4Control>();
    BUG_CHECK(control, "No control found for P4 Action ", act);

    for (auto reg : self.action_register_calls[act]) {
        RegActionInfo info;
        for (auto call : reg.second) {
            createRegisterAction(info, call, act);
            createRegisterExecute(info, call, act->name);
        }

        auto reg_action = info.reg_action;
        auto execute_call = info.execute_call;
        BUG_CHECK(reg_action,
                  "Cannot create register action for register reads or "
                  "writes within P4 Action %1%",
                  act);
        BUG_CHECK(execute_call,
                  "Cannot create register execute call for register reads or "
                  "writes within P4 Action %1%",
                  act);

        self.control_register_actions[control].push_back(reg_action);

        LOG3("Adding execute in " << act->name << " for " << reg.first->toString() << ": "
                                  << execute_call);

        self.action_register_exec_calls[act][reg.first] = execute_call;
        self.generated_register_actions[reg.first].insert(act);
    }

    return false;
}

void RegisterReadWrite::CollectRegisterReadsWrites::collectRegReadWrite(
    const IR::MethodCallExpression *call, const IR::Declaration *act) {
    auto mi = P4::MethodInstance::resolve(call, self.refMap, self.typeMap);
    if (!mi->is<P4::ExternMethod>()) return;

    auto em = mi->to<P4::ExternMethod>();
    cstring externName = em->actualExternType->name;
    if (externName != "Register") return;

    auto stmt = findContext<IR::Statement>();
    if (!stmt) return;

    auto reg = em->object->to<IR::Declaration_Instance>();
    if (!reg) return;

    LOG1("Register extern found: " << em->method->name << " in action: " << act->name);
    if (em->method->name == "read" || em->method->name == "write") {
        self.action_register_calls[act][reg].insert(stmt);
    }
    if (act->is<IR::P4Action>() && !self.actions_using_register[reg].count(act)) {
        for (auto *other : self.actions_using_register[reg]) {
            if (!self.table_mutex(act, other))
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                      "Non-mutualy exclusive actions %1% and %2% both trying to use %3%", act,
                      other, reg);
        }
        self.actions_using_register[reg].insert(act);
    }
}

bool RegisterReadWrite::CollectRegisterReadsWrites::preorder(const IR::MethodCallExpression *call) {
    // Check for register read/write extern calls
    auto act = findContext<IR::P4Action>();
    if (act) {
        collectRegReadWrite(call, act);
    } else {
        auto regAct = findContext<IR::Declaration_Instance>();
        if (regAct) {
            // FIXME -- this code triggers on non-actions that contain calls.  Why do
            // we care about any of these?  Calls to Register methods not in an action
            // can't be supported on tofino, so should trigger an a error later.
            // Turns out that calling a Register directly in a RegisterAction (not implementable)
            // will crash in extract_maupipe if we don't catch it earlier, so we tag such
            // things here as errors.  We should fix extract_maupipe to not craah (and flag
            // the error), or have an earlier pass check and give the error.  Maybe
            // CheckRegisterActions in this file (which is what ends up flagging an error
            // in the test we have but might not cover all cases)
            LOG1("no P4Action context, but there is a Declaration_Instance: " << regAct);
            collectRegReadWrite(call, regAct);
        }
    }

    return false;
}

/*
 * Check that all uses of a register within a single action use the same addressing.
 * This is a Tofino 1/2 HW restriction.
 */
void RegisterReadWrite::CollectRegisterReadsWrites::end_apply() {
    for (auto &[action, regs_in_action] : self.action_register_calls) {
        const IR::Expression *first_addr = nullptr;
        auto first_reg = regs_in_action.begin()->first;
        auto first_reg_type_spec = first_reg->type->to<IR::Type_Specialized>();
        // When compiling for the v1model, type information seems to be a bit different than
        // for PSA and T*NA architectures. With v1model, the template parameters are of the
        // type IR::Type_Name. Type_Name::width_bits() suggests to use getTypeType. It gives
        // correct result for all archs.
        auto first_reg_type_type =
            self.typeMap->getTypeType(first_reg_type_spec->arguments->at(0)->getNode(), true);
        auto first_width = first_reg_type_type->width_bits();
        for (auto &[reg, calls] : regs_in_action) {
            auto reg_type_spec = first_reg->type->to<IR::Type_Specialized>();
            // See above.
            auto reg_type_type =
                self.typeMap->getTypeType(reg_type_spec->arguments->at(0)->getNode(), true);
            auto width = reg_type_type->width_bits();
            if (first_width != width)
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                      "%1%: widths of all registers used within a single action have to "
                      "be the same. Widths of the following registers differ:\n%2%%3%",
                      action, first_reg, reg);
            for (auto *reg_set : calls) {
                auto mce = RegisterReadWrite::extractRegisterReadWrite(reg_set).first;
                if (!mce) {
                    ::fatal_error(
                        ErrorType::ERR_UNSUPPORTED,
                        "%1%: Registers support only calls or assignments of the following forms:\n"
                        "  register.write(index, source);\n"
                        "  destination = register.read(index);\n"
                        "  destination = register.read(index)[M:N];\n"
                        "  destination = (cast)register.read(index);\n"
                        "If more complex calls or assignments are required, try to use "
                        "the RegisterAction extern.",
                        reg_set);
                }
                if (first_addr == nullptr) {
                    first_addr = mce->arguments->at(0)->expression;
                } else {
                    auto *addr = mce->arguments->at(0)->expression;
                    if (!first_addr->equiv(*addr))
                        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                              "%1%: uses of all registers within a single action have to "
                              "use the same addressing. The following uses differ:\n%2%%3%",
                              action, first_addr, addr);
                }
            }
        }
    }
}

bool RegisterReadWrite::MoveRegisterParameters::preorder(IR::P4Control *c) {
    IR::IndexedVector<IR::Declaration> reg_params;
    for (auto *decl : c->controlLocals) {
        auto *type = self.typeMap->getType(decl);
        if (auto *canonical = type->to<IR::Type_SpecializedCanonical>()) type = canonical->baseType;
        if (auto *ext = type->to<IR::Type_Extern>())
            if (ext->name == "RegisterParam") reg_params.push_back(decl);
    }
    for (auto *decl : reg_params) c->controlLocals.removeByName(decl->getName());
    c->controlLocals.prepend(reg_params);
    return true;
}

}  // namespace BFN
