/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extract_maupipe.h"

#include <assert.h>

#include <optional>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/helpers.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/lib/safe_width.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/stateful_alu.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/midend/copy_header.h"  // ENABLE_P4C3251
#include "bf-p4c/midend/simplify_references.h"
#include "bf-p4c/parde/deparser_checksum_update.h"
#include "bf-p4c/parde/extract_deparser.h"
#include "bf-p4c/parde/extract_parser.h"
#include "frontends/common/name_gateways.h"
#include "frontends/p4-14/inline_control_flow.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "ir/dbprint.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "lib/algorithm.h"
#include "lib/error.h"
#include "lib/safe_vector.h"
#include "slice.h"

namespace BFN {

static bool getBool(const IR::Argument *arg) {
    return arg->expression->to<IR::BoolLiteral>()->value;
}

static int getConstant(const IR::Argument *arg) {
    return arg->expression->to<IR::Constant>()->asInt();
}

static big_int getBigConstant(const IR::Annotation *annotation) {
    if (annotation->expr.size() != 1) {
        error("%1% should contain a constant", annotation);
        return 0;
    }
    auto constant = annotation->expr[0]->to<IR::Constant>();
    if (constant == nullptr) {
        error("%1% should contain a constant", annotation);
        return 0;
    }
    return constant->value;
}

static int64_t getConstant(const IR::Annotation *annotation, int64_t min = INT_MIN,
                           uint64_t max = INT_MAX) {
    if (annotation->expr.size() != 1) {
        error("%1% should contain a constant", annotation);
        return 0;
    }
    auto constant = annotation->expr[0]->to<IR::Constant>();
    if (constant == nullptr) {
        error("%1% should contain a constant", annotation);
        return 0;
    }
    int64_t rv = constant->asUint64();
    if (max <= INT64_MAX || min < 0) {
        if (rv < min || (rv >= 0 && uint64_t(rv) > max)) error("%1% out of range", annotation);
    } else {
        if (uint64_t(rv) < uint64_t(min) || uint64_t(rv) > max)
            error("%1% out of range", annotation);
    }
    return rv;
}

static cstring getString(const IR::Annotation *annotation) {
    if (annotation->expr.size() != 1) {
        error("%1% should contain a string", annotation);
        return ""_cs;
    }
    auto str = annotation->expr[0]->to<IR::StringLiteral>();
    if (str == nullptr) {
        error("%1% should contain a string", annotation);
        return ""_cs;
    }
    return str->value;
}

class ActionArgSetup : public MauTransform {
    /* FIXME -- use ParameterSubstitution for this somehow? */
    std::map<cstring, const IR::Expression *> args;
    const IR::Node *preorder(IR::PathExpression *pe) override {
        if (args.count(pe->path->name)) return args.at(pe->path->name);
        return pe;
    }

 public:
    void add_arg(const IR::MAU::ActionArg *a) { args[a->name] = a; }
    void add_arg(cstring name, const IR::Expression *e) { args[name] = e; }
};

/// This pass assumes that if a method call expression is used in assignment
/// statement, the assignement statement must be simple, i.e., it must be
/// in the form of:
///    var = method.execute();
/// if the rhs of the assignment statement is complex, such as
///    var = method.execute() + 1;
/// the assignment must be transformed to two simpler assignment statements
/// by an earlier pass.
///    tmp = method.execute();
///    var = tmp + 1;
/// CTD -- the above is not true -- this code would work fine for more complex expressions,
/// however, earlier passes will transform the complex expressions as noted above, so we could
/// assume that if we wanted to.
class ConvertMethodCalls : public MauTransform {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    void append_extern_constructor_param(const IR::Declaration_Instance *decl, cstring externType,
                                         IR::Vector<IR::Expression> &constructor_params) {
        // only for Hash extern for now.
        if (externType != "Hash") return;
        if (decl->arguments == nullptr) return;
        for (auto arg : *decl->arguments) {
            if (!arg->expression->is<IR::PathExpression>()) continue;
            auto path = arg->expression->to<IR::PathExpression>();
            auto param = refMap->getDeclaration(path->path, false);
            if (auto d = param->to<IR::Declaration_Instance>())
                constructor_params.push_back(new IR::GlobalRef(typeMap->getType(d), d));
        }
    }

    const IR::Expression *preorder(IR::MethodCallExpression *mc) {
        auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap, true);
        cstring name;
        const IR::Expression *recv = nullptr, *extra_arg = nullptr;
        IR::Vector<IR::Expression> constructor_params;
        if (auto bi = mi->to<P4::BuiltInMethod>()) {
            name = bi->name;
            recv = bi->appliedTo;
#if ENABLE_P4C3251
            // Implemention consolidate in bf-p4c/midend/copy_header.cpp
            BUG_CHECK(name != IR::Type_Header::setValid && name != IR::Type_Header::setInvalid &&
                          name != IR::Type_Header::isValid,
                      "%s not removed by DoCopyHeaders", name);
#else
            /* FIXME(CTD) -- duplicates SimplifyHeaderValidMethods a bit, as this may (will?)
             * run before that, and it can't deal with MAU::TypedPrimitives */
            if (name == "isValid") {
                return new IR::Member(mc->srcInfo, mc->type, recv, "$valid");
            } else if (name == "setValid" || name == "setInvalid") {
                recv = new IR::Member(mc->type, recv, "$valid");
                extra_arg = new IR::Constant(mc->type, name == "setValid");
                name = "modify_field"_cs;
            }
#endif
        } else if (auto em = mi->to<P4::ExternMethod>()) {
            name = em->actualExternType->name + "." + em->method->name;
            auto mem = mc->method->to<IR::Member>();
            if (mem && mem->expr->is<IR::This>()) {
                recv = mem->expr;
            } else {
                auto n = em->object->getNode();
                recv = new IR::GlobalRef(mem ? mem->expr->srcInfo : mc->method->srcInfo,
                                         typeMap->getType(n), n);
                // if current extern takes another extern instance as constructor parameter,
                // e.g., Hash takes CRC_Polynomial as an argument.
                if (n->is<IR::Declaration_Instance>())
                    append_extern_constructor_param(n->to<IR::Declaration_Instance>(),
                                                    em->actualExternType->name, constructor_params);
            }
        } else if (auto ef = mi->to<P4::ExternFunction>()) {
            name = ef->method->name;
        } else if (mi->is<P4::ApplyMethod>()) {
            error(ErrorType::ERR_UNSUPPORTED,
                  "%s in complex context not yet supported -- must "
                  "be in an 'if' by itself, not combined with other operations with &&/||",
                  mc);
        } else {
            BUG("method call %s not yet implemented", mc);
        }
        IR::MAU::TypedPrimitive *prim;
        if (mc->method && mc->method->type)
            prim = new IR::MAU::TypedPrimitive(mc->srcInfo, mc->type, mc->method->type, name);
        else
            prim = new IR::MAU::TypedPrimitive(mc->srcInfo, mc->type, nullptr, name);
        int op_index = 0;
        if (recv) {
            prim->operands.push_back(recv);
            op_index++;
        }
        if (constructor_params.size() != 0) {
            prim->operands.append(constructor_params);
            op_index += constructor_params.size();
        }
        int op_index_offset = op_index;
        for (auto arg : *mc->arguments) {
            prim->operands.push_back(arg->expression);
            if (arg->name.name.isNullOrEmpty()) {
                if (auto method = mc->method) {
                    if (auto method_type = method->type) {
                        if (auto method_type_method = method_type->to<IR::Type_Method>()) {
                            auto param_idx = op_index == 0 ? 0 : op_index - op_index_offset;
                            auto param = method_type_method->parameters->getParameter(param_idx);
                            if (param) {
                                prim->op_names[op_index] = param->name;
                            }
                        }
                    }
                }
            } else {
                prim->op_names[op_index] = arg->name;
            }
            op_index++;
        }
        if (extra_arg) {
            prim->operands.push_back(extra_arg);
        }
        // if method call returns a value
        return prim;
    }

    const IR::MAU::Primitive *postorder(IR::MAU::Primitive *prim) {
        if (prim->name == "method_call_init") {
            BUG_CHECK(prim->operands.size() == 1, "method call initialization failed");
            if (auto *p = prim->operands.at(0)->to<IR::MAU::Primitive>())
                return p;
            else if (prim->operands.at(0)->is<IR::Member>())
                // comes from a bare "isValid" call -- is a noop
                return nullptr;
            else
                BUG("method call initialization mismatch: %s", prim);
        } else {
            return prim;
        }
    }

 public:
    ConvertMethodCalls(P4::ReferenceMap *rm, P4::TypeMap *tm) : refMap(rm), typeMap(tm) {}
};

/** Initial conversion of P4-16 action information to backend structures.  Converts all method
 *  calls to primitive calls with operands, and also converts all parameters within an action
 *  into IR::ActionArgs
 */
class ActionFunctionSetup : public PassManager {
    ActionArgSetup *action_arg_setup;

 public:
    explicit ActionFunctionSetup(ActionArgSetup *aas) : action_arg_setup(aas) {
        addPasses({action_arg_setup});
        stop_on_error = false;
    }
};

/** The purpose of this pass is to initialize each IR::MAU::Action with information on whether
 *  or not the action can be used as a default action, or if specifically it is meant only for the
 *  default action.  This information must be passed back directly to the context JSON.
 */
class SetupActionProperties : public MauModifier {
    const IR::P4Table *table;
    const IR::ActionListElement *elem;
    P4::ReferenceMap *refMap;

    bool preorder(IR::MAU::Action *act) override {
        bool has_constant_default_action = false;
        auto prop = table->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
        if (prop && prop->isConstant) has_constant_default_action = true;

        bool is_the_default_action = false;
        bool is_const_default_action = false;
        bool can_be_hit_action = true;
        cstring hit_disallowed_reason = ""_cs;
        bool can_be_default_action = !has_constant_default_action;
        cstring default_disallowed_reason = ""_cs;

        // First, check for action annotations
        auto table_only_annot = elem->annotations->getSingle("tableonly"_cs);
        auto default_only_annot = elem->annotations->getSingle("defaultonly"_cs);
        if (has_constant_default_action) default_disallowed_reason = "has_const_default_action"_cs;
        if (table_only_annot) {
            can_be_default_action = false;
            default_disallowed_reason = "user_indicated_table_only"_cs;
        }
        if (default_only_annot) {
            can_be_hit_action = false;
            hit_disallowed_reason = "user_indicated_default_only"_cs;
        }

        // Second, see if this action is the default action and/or constant default action
        auto default_action = table->getDefaultAction();
        const IR::Vector<IR::Argument> *args = nullptr;
        if (default_action) {
            if (auto mc = default_action->to<IR::MethodCallExpression>()) {
                default_action = mc->method;
                args = mc->arguments;
            }

            auto path = default_action->to<IR::PathExpression>();
            if (!path) BUG("Default action path %s cannot be found", default_action);
            auto actName = refMap->getDeclaration(path->path, true)->externalName();

            if (actName == act->name) {
                is_the_default_action = true;
                if (has_constant_default_action) {
                    can_be_default_action = true;  // this is the constant default action
                    default_disallowed_reason = ""_cs;
                    is_const_default_action = true;
                }
            }
        }

        // Finally, set Action IR node attributes.
        act->hit_allowed = can_be_hit_action;
        act->hit_disallowed_reason = hit_disallowed_reason;
        act->default_allowed = can_be_default_action;
        act->disallowed_reason = default_disallowed_reason;

        if (is_the_default_action) {
            if (is_const_default_action) act->is_constant_action = true;
            // The program-specified default action will be initialized by the compiler.
            act->init_default = true;
            if (args) act->default_params = *args;
        }
        if (default_only_annot) {
            act->hit_allowed = false;
            if (has_constant_default_action && !is_const_default_action)
                error(
                    "%s: Action %s cannot be default only when there is another "
                    "constant default action.",
                    elem->srcInfo, elem);
        }
        return false;
    }

 public:
    SetupActionProperties(const IR::P4Table *t, const IR::ActionListElement *ale,
                          P4::ReferenceMap *rm)
        : table(t), elem(ale), refMap(rm) {}
};

class ActionBodySetup : public Inspector {
    IR::MAU::Action *af;

    bool InHashAnnot() const {
        const Visitor::Context *ctxt = nullptr;
        while (auto *blk = findContext<IR::BlockStatement>(ctxt)) {
            if (blk->getAnnotation("in_hash"_cs)) return true;
            if (blk->getAnnotation("in_vliw"_cs)) return false;
        }
        return false;
    }

    bool preorder(const IR::IndexedVector<IR::StatOrDecl> *) override { return true; }
    bool preorder(const IR::BlockStatement *) override { return true; }
    bool preorder(const IR::AssignmentStatement *assign) override {
        if (!af->exitAction) {
            cstring pname = "modify_field"_cs;
            if (assign->left->type->is<IR::Type_Header>()) pname = "copy_header"_cs;
            auto prim = new IR::MAU::Primitive(assign->srcInfo, pname, assign->left, assign->right);
            prim->in_hash = InHashAnnot();
            af->action.push_back(prim);
        }
        return false;
    }
    bool preorder(const IR::MethodCallStatement *mc) override {
        if (!af->exitAction) {
            auto mc_init =
                new IR::MAU::Primitive(mc->srcInfo, "method_call_init"_cs, mc->methodCall);
            af->action.push_back(mc_init);
        }
        return false;
    }
    bool preorder(const IR::ExitStatement *) override {
        af->exitAction = true;
        return false;
    }
    bool preorder(const IR::Declaration *) override {
        // FIXME -- for now, ignoring local variables?  Need copy prop + dead code elim
        return false;
    }
    bool preorder(const IR::Annotations *) override {
        // FIXME -- for now, ignoring annotations.
        return false;
    }
    bool preorder(const IR::Node *n) override {
        BUG("un-handled node %1% in action", n);
        return false;
    }

 public:
    explicit ActionBodySetup(IR::MAU::Action *af) : af(af) {}
};

static const IR::MAU::Action *createActionFunction(const IR::P4Action *ac,
                                                   const IR::Vector<IR::Argument> *args,
                                                   const Visitor::Context *ctxt) {
    auto rv = new IR::MAU::Action(ac->srcInfo, ac->name, ac->annotations);
    rv->name.name = ac->externalName();
    ActionArgSetup aas;
    size_t arg_idx = 0;
    for (auto param : *ac->parameters->getEnumerator()) {
        if ((param->direction == IR::Direction::None) ||
            ((!args || arg_idx >= args->size()) && param->direction == IR::Direction::In)) {
            auto arg = new IR::MAU::ActionArg(param->srcInfo, param->type, rv->name, param->name,
                                              param->getAnnotations());
            aas.add_arg(arg);
            rv->args.push_back(arg);
        } else {
            if (!args || arg_idx >= args->size())
                error("%s: Not enough args for %s", args ? args->srcInfo : ac->srcInfo, ac);
            else
                aas.add_arg(param->name, args->at(arg_idx++)->expression);
        }
    }
    if (arg_idx != (args ? args->size() : 0)) error("%s: Too many args for %s", args->srcInfo, ac);
    ac->body->apply(ActionBodySetup(rv), ctxt);
    ActionFunctionSetup afs(&aas);
    return rv->apply(afs)->to<IR::MAU::Action>();
}

static IR::MAU::AttachedMemory *createIdleTime(cstring name, const IR::Annotations *annot) {
    auto idletime = new IR::MAU::IdleTime(name);

    if (auto s = annot->getSingle("idletime_precision"_cs)) {
        idletime->precision = getConstant(s);
        /* Default is 3 */
        if (idletime->precision != 1 && idletime->precision != 2 && idletime->precision != 3 &&
            idletime->precision != 6)
            idletime->precision = 3;
    }

    if (auto s = annot->getSingle("idletime_interval"_cs)) {
        idletime->interval = getConstant(s);
        if (idletime->interval < 0 || idletime->interval > 12) idletime->interval = 7;
    }

    if (auto s = annot->getSingle("idletime_two_way_notification"_cs)) {
        int two_way_notification = getConstant(s);
        if (two_way_notification == 1)
            idletime->two_way_notification = "two_way"_cs;
        else if (two_way_notification == 0)
            idletime->two_way_notification = "disable"_cs;
    }

    if (auto s = annot->getSingle("idletime_per_flow_idletime"_cs)) {
        int per_flow_enable = getConstant(s);
        idletime->per_flow_idletime = (per_flow_enable == 1) ? true : false;
    }

    /* this is weird - precision value overrides an explicit two_way_notification
     * and per_flow_idletime pragma  */
    if (idletime->precision > 1) idletime->two_way_notification = "two_way"_cs;

    if (idletime->precision == 1 || idletime->precision == 2)
        idletime->per_flow_idletime = false;
    else
        idletime->per_flow_idletime = true;

    return idletime;
}

const IR::Type *getBaseType(const IR::Type *type) {
    const IR::Type *val = type;
    if (auto *t = type->to<IR::Type_Specialized>()) {
        val = t->baseType;
    } else if (auto *t = type->to<IR::Type_SpecializedCanonical>()) {
        val = t->baseType;
    }
    return val;
}

cstring getTypeName(const IR::Type *type) {
    cstring tname;
    if (auto t = type->to<IR::Type_Name>()) {
        tname = t->path->to<IR::Path>()->name;
    } else if (auto t = type->to<IR::Type_Extern>()) {
        tname = t->name;
    } else {
        BUG("Type %s is not supported as attached table", type->toString());
    }
    return tname;
}

/**
 * Checks if there is an annotation of the provided name attached to the match
 * table.  If so, return its integer value.  Since there is no existing
 * use case for a negative annotation value, return negative error code if it does not exist.
 * This function should only be called when there can be a single annotation
 * of the provided name.
 *
 * return codes:
 *    -1 - table doesn't have annotation
 *    -2 - the annotation isn't a constant
 *    -3 - the annotation is not positive
 */
static int getSingleAnnotationValue(const cstring name, const IR::MAU::Table *table) {
    if (table->match_table) {
        auto annot = table->match_table->getAnnotations();
        if (auto s = annot->getSingle(name)) {
            ERROR_CHECK(s->expr.size() >= 1,
                        "%s: The %s pragma on table %s "
                        "does not have a value",
                        name, table->srcInfo, table->name);
            auto pragma_val = s->expr.at(0)->to<IR::Constant>();
            if (pragma_val == nullptr) {
                error("%s: The %s pragma value on table %s is not a constant", name, table->srcInfo,
                      table->name);
                return -2;
            }
            int value = pragma_val->asInt();
            if (value < 0) {
                error("%s: The %s pragma value on table %s cannot be less than zero.", name,
                      table->srcInfo, table->name);
                return -3;
            }
            return value;
        }
    }
    return -1;
}

static void getCRCPolynomialFromExtern(const P4::ExternInstance &instance,
                                       IR::MAU::HashFunction &hashFunc) {
    if (instance.type->name != "CRCPolynomial") {
        error("Expected CRCPolynomial extern instance %1%", instance.type->name);
        return;
    }
    auto coeffValue = instance.substitution.lookupByName("coeff"_cs)->expression;
    BUG_CHECK(coeffValue->to<IR::Constant>(), "Non-constant coeff");
    auto reverseValue = instance.substitution.lookupByName("reversed"_cs)->expression;
    BUG_CHECK(reverseValue->to<IR::BoolLiteral>(), "Non-boolean reversed");
    auto msbValue = instance.substitution.lookupByName("msb"_cs)->expression;
    BUG_CHECK(msbValue->to<IR::BoolLiteral>(), "Non-boolean msb");
    auto initValue = instance.substitution.lookupByName("init"_cs)->expression;
    BUG_CHECK(initValue->to<IR::Constant>(), "Non-constant init");
    auto extendValue = instance.substitution.lookupByName("extended"_cs)->expression;
    BUG_CHECK(extendValue->to<IR::BoolLiteral>(), "Non-boolean extend");
    auto xorValue = instance.substitution.lookupByName("xor"_cs)->expression;
    BUG_CHECK(xorValue->to<IR::Constant>(), "Non-constant xor");

    hashFunc.msb = msbValue->to<IR::BoolLiteral>()->value;
    hashFunc.extend = extendValue->to<IR::BoolLiteral>()->value;
    hashFunc.reverse = reverseValue->to<IR::BoolLiteral>()->value;
    hashFunc.poly = coeffValue->to<IR::Constant>()->asUint64();
    hashFunc.size = coeffValue->to<IR::Constant>()->type->width_bits();
    hashFunc.init = initValue->to<IR::Constant>()->asUint64();
    hashFunc.final_xor = xorValue->to<IR::Constant>()->asUint64();
    hashFunc.type = IR::MAU::HashFunction::CRC;
}

template <typename T>
static void check_true_egress_accounting(const T *ctr, const IR::MAU::Table *match_table) {
    CHECK_NULL(match_table);

    if (match_table->gress != EGRESS) {
        error(ErrorType::ERR_INVALID, "%1%: True egress accounting is only available on egress.",
              ctr);
    }
    if (ctr->type != IR::MAU::DataAggregation::BOTH &&
        ctr->type != IR::MAU::DataAggregation::BYTES) {
        error(ErrorType::ERR_INVALID,
              "%1%: True egress accounting is only valid for counting bytes", ctr);
    }
}

static int get_meter_color(const IR::Argument *arg) {
    auto expr = arg->expression;

    if (!expr->is<IR::Constant>())
        error(ErrorType::ERR_INVALID,
              "Invalid meter color value. "
              "Value must be constant: %1%",
              expr);
    auto value = arg->expression->to<IR::Constant>()->asInt();
    if (value < 0 || value > 255)
        error(ErrorType::ERR_OVERLIMIT,
              "Meter color value out of range. "
              "Value must be in the range of 0-255: %1%",
              expr);
    return value;
}

/**
 * TODO: Pass ExternInstance* as a parameter, instead of srcInfo, name, type,
 * substitution, annot.
 * Gated by use in AttachTables::DefineGlobalRefs::postorder(IR::GlobalRef *gref)
 */
static IR::MAU::AttachedMemory *createAttached(
    Util::SourceInfo srcInfo, cstring name, const IR::Type *type,
    const P4::ParameterSubstitution *substitution, const IR::Vector<IR::Type> *typeArgs,
    const IR::Annotations *annot, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
    StatefulSelectors &stateful_selectors, const IR::MAU::AttachedMemory **created_ap = nullptr,
    const IR::MAU::Table *match_table = nullptr) {
    auto baseType = getBaseType(type);
    auto tname = getTypeName(baseType);

    if (tname == "ActionSelector") {
        auto sel = new IR::MAU::Selector(srcInfo, name, annot);

        // setup action selector group and group size.
        int num_groups = StageUseEstimate::COMPILER_DEFAULT_SELECTOR_POOLS;
        int max_group_size = StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE;
        bool sps_scramble = true;

        // Check match table for selector pragmas that specify selector properties.
        // P4-14 legacy reasons put these pragmas on the match table rather than
        // the action selector object.
        if (match_table) {
            // Check for max group size pragma.
            int pragma_max_group_size =
                getSingleAnnotationValue("selector_max_group_size"_cs, match_table);
            if (pragma_max_group_size != -1) {
                max_group_size = pragma_max_group_size;
                // max ram words is 992, max members per word is 120
                if (max_group_size < 1 || max_group_size > (992 * 120)) {
                    error(
                        "%s: The selector_max_group_size pragma value on table %s is "
                        "not between %d and %d.",
                        match_table->srcInfo, match_table->name, 1, 992 * 120);
                }
            }
            // Check for number of max groups pragma.
            int pragma_num_max_groups =
                getSingleAnnotationValue("selector_num_max_groups"_cs, match_table);
            if (pragma_num_max_groups != -1) {
                num_groups = pragma_num_max_groups;
                if (num_groups < 1) {
                    error(
                        "%s: The selector_num_max_groups pragma value on table %s is "
                        "not greater than or equal to 1.",
                        match_table->srcInfo, match_table->name);
                }
            }
            // Check for selector_enable_scramble pragma.
            int pragma_sps_en =
                getSingleAnnotationValue("selector_enable_scramble"_cs, match_table);
            if (pragma_sps_en != -1) {
                if (pragma_sps_en == 0) {
                    sps_scramble = false;
                } else if (pragma_sps_en == 1) {
                    sps_scramble = true;
                } else {
                    error(
                        "%s: The selector_enable_scramble pragma value on table %s is "
                        "only allowed to be 0 or 1.",
                        match_table->srcInfo, match_table->name);
                }
            }
        }

        sel->sps_scramble = sps_scramble;

        // processing ActionSelector constructor parameters.
        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            if (p->name == "mode") {
                if (arg->expression->to<IR::Member>()->member.name == "FAIR")
                    sel->mode = IR::MAU::SelectorMode::FAIR;
                else if (arg->expression->to<IR::Member>()->member.name == "RESILIENT")
                    sel->mode = IR::MAU::SelectorMode::RESILIENT;
                else
                    error("%1%: Selector mode of %2% must be either FAIR or RESILIENT",
                          sel->srcInfo, sel->name);
            } else if (p->name == "hash") {
                auto inst = P4::ExternInstance::resolve(arg->expression, refMap, typeMap);
                if (inst != std::nullopt) {
                    if (inst->arguments->size() == 1) {
                        if (!sel->algorithm.setup(inst->arguments->at(0)->expression))
                            BUG("invalid algorithm %s", inst->arguments->at(0)->expression);
                    } else if (inst->arguments->size() == 2) {
                        auto crc_poly = P4::ExternInstance::resolve(
                            inst->arguments->at(1)->expression, refMap, typeMap);
                        if (crc_poly == std::nullopt) continue;
                        getCRCPolynomialFromExtern(*crc_poly, sel->algorithm);
                    }
                }
            } else if (p->name == "reg") {
                auto regpath = arg->expression->to<IR::PathExpression>()->path;
                auto reg = refMap->getDeclaration(regpath)->to<IR::Declaration_Instance>();
                if (stateful_selectors.count(reg))
                    error("%1% bound to both %2% and %3%", reg, stateful_selectors.at(reg), sel);
                stateful_selectors.emplace(reg, sel);
            } else if (p->name == "max_group_size") {
                max_group_size = getConstant(arg);
                // max ram words is 992, max members per word is 120
                if (max_group_size < 1 || max_group_size > (992 * 120)) {
                    error(
                        "%s: The max_group_size value on ActionSelector %s is "
                        "not between %d and %d.",
                        srcInfo, name, 1, 992 * 120);
                }
            } else if (p->name == "num_groups") {
                num_groups = getConstant(arg);
                if (num_groups < 1) {
                    error(
                        "%s: The selector_num_max_groups pragma value on table %s is "
                        "not greater than or equal to 1.",
                        srcInfo, name);
                }
            } else if (p->name == "action_profile") {
                if (auto path = arg->expression->to<IR::PathExpression>()) {
                    auto af = P4::ExternInstance::resolve(path, refMap, typeMap);
                    if (af == std::nullopt) {
                        error(
                            "Expected %1% for ActionSelector %2% to resolve to an "
                            "ActionProfile extern instance",
                            arg, name);
                    }
                    auto ap = new IR::MAU::ActionData(srcInfo, IR::ID(*af->name));
                    ap->direct = false;
                    ap->size = getConstant(af->arguments->at(0));
                    // FIXME Need to reconstruct the field list from the table key?
                    *created_ap = ap;
                }
            } else if (p->name == "size") {
                // TODO: used to support the deprecated ActionSelector API.
                // ActionSelector(bit<32> size, Hash<_> hash, SelectorMode_t mode);
                // Remove once the deprecated API is removed from
                auto ap = new IR::MAU::ActionData(srcInfo, IR::ID(name));
                ap->direct = false;
                ap->size = getConstant(arg);
                // FIXME Need to reconstruct the field list from the table key?
                *created_ap = ap;
            }
        }

        sel->num_pools = num_groups;
        sel->max_pool_size = max_group_size;
        sel->size = (sel->max_pool_size + 119) / 120 * sel->num_pools;
        return sel;
    } else if (tname == "ActionProfile") {
        auto ap = new IR::MAU::ActionData(srcInfo, IR::ID(name), annot);
        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            if (p->name == "size") ap->size = getConstant(arg);
        }
        return ap;
    } else if (tname == "Counter" || tname == "DirectCounter") {
        auto ctr = new IR::MAU::Counter(srcInfo, name, annot);
        if (tname == "DirectCounter") ctr->direct = true;

        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            if (p->name == "type") {
                ctr->settype(arg->expression->as<IR::Member>().member.name);
            } else if (p->name == "size") {
                ctr->size = getConstant(arg);
            } else if (p->name == "true_egress_accounting") {
                ctr->true_egress_accounting = getBool(arg);
            }
        }

        if (typeArgs != nullptr && typeArgs->size() != 0) {
            ctr->min_width = safe_width_bits(typeArgs->at(0));
        } else {
            ctr->min_width = -1;
        }
        /* min_width comes via Type_Specialized */
        for (auto anno : annot->annotations) {
            if (anno->name == "max_width")
                ctr->max_width = getConstant(anno);
            else if (anno->name == "threshold")
                ctr->threshold = getConstant(anno);
            else if (anno->name == "interval")
                ctr->interval = getConstant(anno);
            else if (anno->name == "true_egress_accounting")
                ctr->true_egress_accounting = true;
            else if ((anno->name == "adjust_byte_count") && anno->expr.size() == 1 &&
                     anno->expr[0]->to<IR::Constant>())
                // Byte count adjustment is always subtracted
                ctr->bytecount_adjust -= getConstant(anno);
        }
        if (ctr->true_egress_accounting) check_true_egress_accounting(ctr, match_table);

        return ctr;
    } else if (tname == "Meter" || tname == "DirectMeter") {
        auto mtr = new IR::MAU::Meter(srcInfo, name, annot);
        mtr->direct = tname == "DirectMeter";
        // Ideally, we would access the arguments by name, however, the P4 frontend IR only
        // populate the 'name' field of IR::Argument when it is used as a named argument.
        // We had to access the name by 'index' and be careful about not to access indices
        // that do not exist.
        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            auto expr = arg->expression;
            if (!expr) continue;
            if (p->name == "size") {
                if (!expr->is<IR::Constant>())
                    error(ErrorType::ERR_INVALID, "Invalid Meter size %1%", expr);
                mtr->size = expr->to<IR::Constant>()->asInt();
            } else if (p->name == "type") {
                if (!expr->is<IR::Member>())
                    error(ErrorType::ERR_INVALID,
                          "Invalid Meter type %1%, must be"
                          "PACKETS or BYTES",
                          expr);
                mtr->settype(arg->expression->as<IR::Member>().member.name);
            } else if (p->name == "red") {
                mtr->red_value = get_meter_color(arg);
            } else if (p->name == "yellow") {
                mtr->yellow_value = get_meter_color(arg);
            } else if (p->name == "green") {
                mtr->green_value = get_meter_color(arg);
            } else if (p->name == "true_egress_accounting") {
                mtr->true_egress_accounting = getBool(arg);
            }
        }
        // annotations are supported for p4-14.
        for (auto anno : annot->annotations) {
            if (anno->name == "result")
                mtr->result = anno->expr.at(0);
            else if (anno->name == "implementation")
                mtr->implementation = getString(anno);
            else if (anno->name == "green")
                mtr->green_value = getConstant(anno);
            else if (anno->name == "yellow")
                mtr->yellow_value = getConstant(anno);
            else if (anno->name == "red")
                mtr->red_value = getConstant(anno);
            else if (anno->name == "meter_profile")
                mtr->profile = getConstant(anno);
            else if (anno->name == "meter_sweep_interval")
                mtr->sweep_interval = getConstant(anno);
            else if (anno->name == "true_egress_accounting")
                mtr->true_egress_accounting = true;
            else if ((anno->name == "adjust_byte_count") && anno->expr.size() == 1 &&
                     anno->expr[0]->to<IR::Constant>())
                // Byte count adjustment is always subtracted
                mtr->bytecount_adjust -= getConstant(anno);
            else
                LOG1("WARNING: Unknown annotation " << anno->name << " on " << tname);
        }
        if (mtr->true_egress_accounting) check_true_egress_accounting(mtr, match_table);

        return mtr;
    } else if (tname == "Lpf") {
        auto mtr = new IR::MAU::Meter(srcInfo, name, annot);
        mtr->implementation = IR::ID("lpf");
        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            if (p->name == "size") {
                mtr->size = getConstant(arg);
            }
        }
        return mtr;
    } else if (tname == "DirectLpf") {
        auto mtr = new IR::MAU::Meter(srcInfo, name, annot);
        mtr->implementation = IR::ID("lpf");
        mtr->direct = true;
        return mtr;
    } else if (tname == "Wred") {
        auto mtr = new IR::MAU::Meter(srcInfo, name, annot);
        mtr->implementation = IR::ID("wred");
        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            if (p->name == "size") {
                mtr->size = getConstant(arg);
            } else if (p->name == "drop_value") {
                mtr->red_drop_value = getConstant(arg);
            } else if (p->name == "no_drop_value") {
                mtr->red_nodrop_value = getConstant(arg);
            }
        }
        return mtr;
    } else if (tname == "DirectWred") {
        auto mtr = new IR::MAU::Meter(srcInfo, name, annot);
        mtr->implementation = IR::ID("wred");
        mtr->direct = true;
        for (auto p : *substitution->getParametersInOrder()) {
            auto arg = substitution->lookup(p);
            if (arg == nullptr) continue;
            if (p->name == "drop_value") {
                mtr->red_drop_value = getConstant(arg);
            } else if (p->name == "no_drop_value") {
                mtr->red_nodrop_value = getConstant(arg);
            }
        }
        return mtr;
    }

    LOG2("Failed to create attached table for " << tname);
    return nullptr;
}

class FixP4Table : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    IR::MAU::Table *tt;
    std::set<cstring> &unique_names;
    DeclarationConversions &converted;
    StatefulSelectors &stateful_selectors;
    DeclarationConversions &assoc_profiles;

    // TODO: this function only needs one parameter. RefMap and typeMap
    // are passed in as a temporary workaround to avoid larger refactor.
    void createAttachedTableFromTableProperty(P4::ExternInstance &instance,
                                              P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        const IR::MAU::AttachedMemory *obj = nullptr;
        const IR::MAU::AttachedMemory *side_obj = nullptr;
        auto tname = cstring::make_unique(unique_names, *instance.name);
        unique_names.insert(tname);
        auto sub = instance.substitution;
        obj = createAttached(instance.expression->srcInfo, tname, instance.type,
                             &instance.substitution, instance.typeArguments,
                             instance.annotations->getAnnotations(), refMap, typeMap,
                             stateful_selectors, &side_obj, tt);

        // TODO: workaround to populate DeclarationConversions and StatefulSelectors.
        // The key to these maps should be ExternInstance, instead of Declaration_Instance,
        // but that require a larger refactor, which can be done later.
        {  // begin workaround
            if (auto path = instance.expression->to<IR::PathExpression>()) {
                auto decl = refMap->getDeclaration(path->path, true);
                if (!decl->is<IR::Declaration_Instance>()) return;

                auto inst = decl->to<IR::Declaration_Instance>();
                auto type = typeMap->getType(inst);
                if (!type) {
                    BUG("Couldn't determine the type of expression: %1%", path);
                }
                if (converted.count(inst) == 0) {
                    converted[inst] = obj;
                    if (side_obj) assoc_profiles[inst] = side_obj;
                } else {
                    obj = converted.at(inst);
                    if (assoc_profiles.count(inst)) side_obj = assoc_profiles.at(inst);
                }
            }
        }  // end workaround
        LOG3("attaching " << obj->name << " to " << tt->name);
        tt->attached.push_back(new IR::MAU::BackendAttached(obj->srcInfo, obj));
        if (side_obj)
            tt->attached.push_back(new IR::MAU::BackendAttached(side_obj->srcInfo, side_obj));
    }

    bool preorder(const IR::P4Table *tc) override {
        auto impl = getExternInstanceFromProperty(tc, "implementation"_cs, refMap, typeMap);
        if (impl != std::nullopt) {
            if (impl->type->name == "ActionProfile") {
                createAttachedTableFromTableProperty(*impl, refMap, typeMap);
            } else if (impl->type->name == "ActionSelector") {
                createAttachedTableFromTableProperty(*impl, refMap, typeMap);
            }
        }

        auto counters = getExternInstanceFromProperty(tc, "counters"_cs, refMap, typeMap);
        if (counters != std::nullopt) {
            if (counters->type->name == "DirectCounter") {
                createAttachedTableFromTableProperty(*counters, refMap, typeMap);
            }
        }

        auto meters = getExternInstanceFromProperty(tc, "meters"_cs, refMap, typeMap);
        if (meters != std::nullopt) {
            if (meters->type->name == "DirectMeter") {
                createAttachedTableFromTableProperty(*meters, refMap, typeMap);
            }
        }

        auto timeout = getExpressionFromProperty(tc, "idle_timeout"_cs);
        if (timeout != std::nullopt) {
            auto bool_lit = (*timeout)->expression->to<IR::BoolLiteral>();
            if (bool_lit == nullptr || bool_lit->value == false) return false;
            auto annot = tc->getAnnotations();
            auto it = createIdleTime(tc->name, annot);
            tt->attached.push_back(new IR::MAU::BackendAttached(it->srcInfo, it));
        }

        auto atcam = getExternInstanceFromProperty(tc, "atcam"_cs, refMap, typeMap);
        if (atcam != std::nullopt) {
            if (atcam->type->name == "Atcam") {
                tt->layout.partition_count =
                    atcam->arguments->at(0)->expression->to<IR::Constant>()->asInt();
            }
        }

        // BEGIN: ALPM_OPT
        // internal table property to implement alpm optimization
        // We choose to implement alpm in the midend as a transformation to
        // atcam and preclassifier tcam based on the intuition that the ALPM
        // extern definition in TNA is a library extern, not a primitive
        // extern.
        auto as_atcam = getExpressionFromProperty(tc, "as_atcam"_cs);
        if (as_atcam != std::nullopt) {
            auto bool_lit = (*as_atcam)->expression->to<IR::BoolLiteral>();
            tt->layout.atcam = bool_lit->value;
        }

        auto as_alpm = getExpressionFromProperty(tc, "as_alpm"_cs);
        if (as_alpm != std::nullopt) {
            auto bool_lit = (*as_alpm)->expression->to<IR::BoolLiteral>();
            tt->layout.alpm = bool_lit->value;
        }

        auto alpm_preclassifier = getExpressionFromProperty(tc, "alpm_preclassifier"_cs);
        if (alpm_preclassifier != std::nullopt) {
            auto bool_lit = (*alpm_preclassifier)->expression->to<IR::BoolLiteral>();
            tt->layout.pre_classifier = bool_lit->value;
        }

        auto partition_count = getExpressionFromProperty(tc, "atcam_partition_count"_cs);
        if (partition_count != std::nullopt) {
            auto int_lit = (*partition_count)->expression->to<IR::Constant>()->asInt();
            tt->layout.partition_count = int_lit;
        }

        auto subtrees_per_partition =
            getExpressionFromProperty(tc, "atcam_subtrees_per_partition"_cs);
        if (subtrees_per_partition != std::nullopt) {
            auto int_lit = (*subtrees_per_partition)->expression->to<IR::Constant>()->asInt();
            tt->layout.subtrees_per_partition = int_lit;
        }

        auto number_entries = getExpressionFromProperty(tc, "alpm_preclassifier_number_entries"_cs);
        if (number_entries != std::nullopt) {
            auto int_lit = (*number_entries)->expression->to<IR::Constant>()->asInt();
            tt->layout.pre_classifer_number_entries = int_lit;
        }

        auto atcam_subset_width = getExpressionFromProperty(tc, "atcam_subset_width"_cs);
        if (atcam_subset_width != std::nullopt) {
            auto int_lit = (*atcam_subset_width)->expression->to<IR::Constant>()->asInt();
            tt->layout.atcam_subset_width = int_lit;
        }

        auto shift_granularity = getExpressionFromProperty(tc, "shift_granularity"_cs);
        if (shift_granularity != std::nullopt) {
            auto int_lit = (*shift_granularity)->expression->to<IR::Constant>()->asInt();
            tt->layout.shift_granularity = int_lit;
        }

        // table property to pass the excluded_field_msb_bits pragma info to backend
        // the syntax is excluded_field_msb = { {name, msb}, {name, msb} };
        auto exclude_field_msb = getExpressionFromProperty(tc, "excluded_field_msb_bits"_cs);
        if (exclude_field_msb != std::nullopt) {
            if (auto all_excluded_fields =
                    (*exclude_field_msb)->expression->to<IR::ListExpression>()) {
                for (auto excluded_field : all_excluded_fields->components) {
                    if (auto name_and_msb = excluded_field->to<IR::ListExpression>()) {
                        auto name = name_and_msb->components.at(0)->to<IR::StringLiteral>()->value;
                        auto msb = name_and_msb->components.at(1)->to<IR::Constant>()->asInt();
                        tt->layout.excluded_field_msb_bits[name] = msb;
                    }
                }
            }
        }
        // END:: ALPM_OPT

        auto hash = getExternInstanceFromProperty(tc, "proxy_hash"_cs, refMap, typeMap);
        if (hash != std::nullopt) {
            if (hash->type->name == "Hash") {
                tt->layout.proxy_hash = true;

                tt->layout.proxy_hash_width = safe_width_bits(hash->typeArguments->at(0));
                ERROR_CHECK(tt->layout.proxy_hash_width != 0, "ProxyHash width cannot be 0");

                if (hash->arguments->size() == 1) {
                    if (!tt->layout.proxy_hash_algorithm.setup(hash->arguments->at(0)->expression))
                        BUG("invalid algorithm %s", hash->arguments->at(0)->expression);
                } else if (hash->arguments->size() == 2) {
                    auto crc_poly = P4::ExternInstance::resolve(hash->arguments->at(1)->expression,
                                                                refMap, typeMap);
                    if (crc_poly == std::nullopt) return false;
                    getCRCPolynomialFromExtern(*crc_poly, tt->layout.proxy_hash_algorithm);
                }
            }
        }
        return false;
    }

 public:
    FixP4Table(P4::ReferenceMap *r, P4::TypeMap *tm, IR::MAU::Table *tt, std::set<cstring> &u,
               DeclarationConversions &con, StatefulSelectors &ss, DeclarationConversions &ap)
        : refMap(r),
          typeMap(tm),
          tt(tt),
          unique_names(u),
          converted(con),
          stateful_selectors(ss),
          assoc_profiles(ap) {}
};

Visitor::profile_t AttachTables::init_apply(const IR::Node *root) {
    LOG5("AttachTables working on:" << std::endl << root);
    return PassManager::init_apply(root);
}

/** Determine stateful ALU Declaration_Instance underneath the register action, as well
 *  as typing information
 */
bool AttachTables::findSaluDeclarations(const IR::Declaration_Instance *ext,
                                        const IR::Declaration_Instance **reg_ptr,
                                        const IR::Type_Specialized **regtype_ptr,
                                        const IR::Type_Extern **seltype_ptr) {
    auto *loc = &ext->srcInfo;
    const IR::Declaration_Instance *reg = nullptr;
    if (auto exttype = ext->type->to<IR::Type_Specialized>()) {
        auto tname = exttype->baseType->toString();
        if (tname == "Register" || tname == "DirectRegister") reg = ext;
    }
    if (!reg) {
        auto reg_arg = ext->arguments->size() > 0 ? ext->arguments->at(0)->expression : nullptr;
        if (!reg_arg) {
            // FIXME -- P4_14 compat code, no longer needed?
            if (auto regprop = ext->properties["reg"_cs]) {
                if (auto rpv = regprop->value->to<IR::ExpressionValue>())
                    reg_arg = rpv->expression;
                else
                    BUG("reg property %s is not an ExpressionValue", regprop);
            } else {
                error("%sno reg property in stateful_alu %s", ext->srcInfo, ext->name);
                return false;
            }
        }
        loc = &reg_arg->srcInfo;
        auto pe = reg_arg->to<IR::PathExpression>();
        auto d = pe ? refMap->getDeclaration(pe->path, true) : nullptr;
        reg = d ? d->to<IR::Declaration_Instance>() : nullptr;
    }
    auto regtype = reg ? reg->type->to<IR::Type_Specialized>() : nullptr;
    auto seltype = reg ? reg->type->to<IR::Type_Extern>() : nullptr;
    if ((!regtype || regtype->baseType->toString() != "DirectRegister") &&
        (!regtype || regtype->baseType->toString() != "Register") &&
        (!seltype || seltype->name != "ActionSelector")) {
        error("%snot a Register or ActionSelector", *loc);
        return false;
    }
    *reg_ptr = reg;
    if (regtype_ptr) *regtype_ptr = regtype;
    if (seltype_ptr) *seltype_ptr = seltype;
    return true;
}

/** Converts the Declaration_Instance of the register action into a StatefulAlu object.  Checks
 *  to see if a stateful ALU has already been created, and will also create the SALU Instructions
 *  if the register action is new
 */
void AttachTables::InitializeStatefulAlus ::updateAttachedSalu(const IR::Declaration_Instance *ext,
                                                               const IR::GlobalRef *gref) {
    const IR::Declaration_Instance *reg = nullptr;
    const IR::Type_Specialized *regtype = nullptr;
    const IR::Type_Extern *seltype = nullptr;
    LOG6("updateAttachedSalu(" << ext->name << "[" << ext->id << "], "
                               << gref->srcInfo.toPositionString() << "[" << gref->id << "])");
    bool found = self.findSaluDeclarations(ext, &reg, &regtype, &seltype);
    if (!found) return;

    IR::MAU::StatefulAlu *salu = nullptr;
    if (self.salu_inits.count(reg)) {
        salu = self.salu_inits.at(reg);
    } else {
        // Stateful ALU has not been seen yet
        LOG3("Creating new StatefulAlu for "
             << (regtype ? regtype->toString() : seltype->toString()) << " " << reg->name);
        auto regName = reg->externalName();
        // @reg annotation is used in p4-14 to generate PD API for action selector.
        auto anno = ext->annotations->getSingle("reg"_cs);
        if (anno) regName = getString(anno);
        salu = new IR::MAU::StatefulAlu(reg->srcInfo, regName, reg->annotations, reg);
        // Reg initialization values for lo/hi are passed as annotations. In
        // p4_14 conversion, they cannot be set directly on the register. For
        // p4_16 programs, these are passed in as optional stateful params
        for (auto annot : ext->annotations->annotations) {
            if (annot->name == "initial_register_lo_value") {
                salu->init_reg_lo = getBigConstant(annot);
                warning(ErrorType::WARN_DEPRECATED,
                        "%s is deprecated, use the initial_value "
                        "argument of the Register constructor instead",
                        annot);
                LOG4("Reg initial lo value: " << salu->init_reg_lo);
            }
            if (annot->name == "initial_register_hi_value") {
                salu->init_reg_hi = getBigConstant(annot);
                warning(ErrorType::WARN_DEPRECATED,
                        "%s is deprecated, use the initial_value "
                        "argument of the Register constructor instead",
                        annot);
                LOG4("Reg initial hi value: " << salu->init_reg_hi);
            }
        }
        if (seltype) {
            salu->direct = false;
            auto sel = self.converted.at(reg)->to<IR::MAU::Selector>();
            ERROR_CHECK(sel,
                        "%s: Could not find the associated selector within the stateful "
                        "ALU %s, even though a selector is specified",
                        gref->srcInfo, salu->name);
            if (self.stateful_selectors.count(reg))
                error("%1% bound to both %2% and %3%", reg, self.stateful_selectors.at(reg), sel);
            salu->selector = sel;
            int ram_lines = SelectorRAMLinesPerEntry(sel);
            ram_lines = ((ram_lines * sel->num_pools + 1023) / 1024) * 1024;
            salu->size = ram_lines * 128;
        } else if (regtype != nullptr && regtype->toString().startsWith("Register<")) {
            salu->direct = false;
            salu->size = getConstant(reg->arguments->at(0));
        } else {
            salu->direct = true;
        }
        if (self.stateful_selectors.count(reg)) salu->selector = self.stateful_selectors.at(reg);
        if (regtype) {
            salu->width = safe_width_bits(regtype->arguments->at(0));
            if (auto str = regtype->arguments->at(0)->to<IR::Type_Struct>())
                salu->dual = str->fields.size() > 1;
            if (reg->arguments->size() > !salu->direct) {
                auto *init = reg->arguments->at(!salu->direct)->expression;
                // any malformed initial value should have been diagnosed already, so no need
                // for error messages here
                if (auto *k = init->to<IR::Constant>()) {
                    salu->init_reg_lo = k->value;
                } else if (init->is<IR::ListExpression>() || init->is<IR::StructExpression>()) {
                    auto components = *getListExprComponents(*init);
                    if (components.size() >= 1) {
                        if (auto *k = components.at(0)->to<IR::Constant>())
                            salu->init_reg_lo = k->value;
                    }
                    if (components.size() >= 2) {
                        if (auto *k = components.at(1)->to<IR::Constant>())
                            salu->init_reg_hi = k->value;
                    }
                }
            }
        } else {
            salu->width = 1;
        }
        if (auto cts = reg->annotations->getSingle("chain_total_size"_cs))
            salu->chain_total_size = getConstant(cts);
        self.salu_inits[reg] = salu;
    }

    if (reg == ext) {
        // direct register call (to .clear())
        return;
    }

    // copy annotations from register action
    IR::Annotations *new_annot = nullptr;
    for (auto annot : ext->annotations->annotations) {
        if (annot->name == "name") continue;
        if (auto old = salu->annotations->getSingle(annot->name)) {
            if (old->expr.equiv(annot->expr)) continue;
            warning("Conflicting annotations %s and %s related to stateful alu %s", annot, old,
                    salu);
        }
        if (!new_annot) new_annot = salu->annotations->clone();
        new_annot->add(annot);
    }
    if (new_annot) salu->annotations = new_annot;

    if (auto red_or = ext->annotations->getSingle("reduction_or_group"_cs)) {
        auto pragma_val = red_or->expr.at(0)->to<IR::StringLiteral>();
        ERROR_CHECK(pragma_val,
                    "%s: Please provide a valid reduction_or_group for, which should "
                    "be a string %s",
                    salu->srcInfo, salu->name);
        if (pragma_val) {
            if (salu->reduction_or_group.isNull()) {
                salu->reduction_or_group = pragma_val->value;
            } else {
                ERROR_CHECK(salu->reduction_or_group == pragma_val->value,
                            "%s: Stateful ALU %s"
                            "cannot have multiple reduction or group",
                            salu->srcInfo, salu->name);
            }
        }
    }
    if (ext->type->toString().startsWith("LearnAction")) salu->learn_action = true;

    auto prim = findContext<IR::MAU::Primitive>();
    LOG6("  - " << (prim ? prim->name : "<no primitive>"));
    if (prim && prim->name.endsWith(".address")) {
        salu->chain_vpn = true;
        return;
    }
    auto tbl = findContext<IR::MAU::Table>();
    auto tbl_name = tbl ? tbl->name : "<no table>"_cs;
    LOG6("  - table " << tbl_name);
    auto act = findContext<IR::MAU::Action>();
    auto act_name = act ? act->name.originalName : "<no action>"_cs;
    LOG6("  - action " << act_name);
    auto ta_pair = tbl_name + "-" + act_name;
    if (!salu->action_map.emplace(ta_pair, ext->name).second)
        error("%s: multiple calls to execute in action %s", gref->srcInfo, act_name);
}

bool AttachTables::isSaluActionType(const IR::Type *type) {
    static std::set<cstring> saluActionTypes = {
        "DirectRegister"_cs,        "DirectRegisterAction"_cs,
        "DirectRegisterAction2"_cs, "DirectRegisterAction3"_cs,
        "DirectRegisterAction4"_cs, "LearnAction"_cs,
        "LearnAction2"_cs,          "LearnAction3"_cs,
        "LearnAction4"_cs,          "MinMaxAction"_cs,
        "MinMaxAction2"_cs,         "MinMaxAction3"_cs,
        "MinMaxAction4"_cs,         "Register"_cs,
        "RegisterAction"_cs,        "RegisterAction2"_cs,
        "RegisterAction3"_cs,       "RegisterAction4"_cs,
        "SelectorAction"_cs};
    cstring tname = type->toString();
    tname = tname.before(tname.find('<'));
    return saluActionTypes.count(tname) > 0;
}

void AttachTables::InitializeStatefulAlus::postorder(const IR::GlobalRef *gref) {
    visitAgain();
    if (auto di = gref->obj->to<IR::Declaration_Instance>()) {
        if (isSaluActionType(di->type)) {
            updateAttachedSalu(di, gref);
        }
    }
}

/**
 * Gathers information about register params and checks
 * that they are used in a single stateful ALU.
 */
bool AttachTables::InitializeRegisterParams::preorder(const IR::MAU::Primitive *prim) {
    if (prim->name != "RegisterParam.read") return true;
    const IR::Declaration_Instance *reg_param_decl = nullptr;
    if (auto *gr = prim->operands.at(0)->to<IR::GlobalRef>())
        reg_param_decl = gr->obj->to<IR::Declaration_Instance>();
    BUG_CHECK(reg_param_decl != nullptr, "null Declaration_Instance under GlobalRef");
    const IR::Declaration_Instance *reg_decl = nullptr;
    if (auto *reg_act_decl = findContext<IR::Declaration_Instance>()) {
        if (isSaluActionType(getBaseType(reg_act_decl->type))) {
            BUG_CHECK(!reg_act_decl->arguments->empty(), "RegisterAction misses an argument");
            auto *reg_act_decl_arg0_expr = reg_act_decl->arguments->at(0)->expression;
            if (auto *reg_decl_path = reg_act_decl_arg0_expr->to<IR::PathExpression>())
                reg_decl = getDeclInst(self.refMap, reg_decl_path);
        } else {
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%: RegisterParam cannot be used outside RegisterAction", prim);
        }
    }
    if (reg_decl == nullptr)
        error(ErrorType::ERR_UNSUPPORTED,
              "%1%: RegisterParam can be used only in RegisterAction or within Register.write",
              prim);
    if (self.salu_inits.count(reg_decl) > 0) {
        auto *salu = self.salu_inits.at(reg_decl);
        if (param_salus.count(reg_param_decl) > 0) {
            auto *previous_salu = param_salus.at(reg_param_decl);
            if (previous_salu != salu)
                error(ErrorType::ERR_UNSUPPORTED,
                      "%1%: RegisterParam cannot be used with multiple Registers", prim);
        } else {
            param_salus[reg_param_decl] = salu;
        }
    }
    return false;
}

/**
 * Allocates a register file row for each register param.
 */
void AttachTables::InitializeRegisterParams::end_apply() {
    for (auto p : param_salus) {
        auto *decl = p.first;
        auto *salu = p.second;
        int index = static_cast<int>(salu->regfile.size());
        BUG_CHECK(!decl->arguments->empty(), "RegisterParam misses an argument");
        auto *initial_expr = decl->arguments->at(0)->expression;
        auto *initial_expr_type = self.typeMap->getType(initial_expr);
        BUG_CHECK(initial_expr_type != nullptr,
                  "Missing type information about RegisterParam argument");
        auto *bits = initial_expr_type->to<IR::Type_Bits>();
        if (!bits ||
            (bits->width_bits() != 8 && bits->width_bits() != 16 && bits->width_bits() != 32)) {
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%: Unsupported RegisterParam type %2%. "
                  "Supported types are bit<8>, int<8>, bit<16>, int<16>, bit<32>, and int<32>.",
                  decl, initial_expr_type);
        }
        int64_t initial_value = 0;
        if (auto *k = initial_expr->to<IR::Constant>()) initial_value = k->asInt64();
        auto *row = new IR::MAU::SaluRegfileRow(decl->srcInfo, index, initial_value, decl->name,
                                                decl->externalName());
        salu->regfile.insert(std::make_pair(decl->name.name, row));
        LOG3("Added " << row << " to " << salu->name);
    }
}

void AttachTables::InitializeStatefulInstructions::postorder(const IR::GlobalRef *gref) {
    visitAgain();
    auto ext = gref->obj->to<IR::Declaration_Instance>();
    if (ext == nullptr) return;
    if (!isSaluActionType(ext->type)) return;

    const IR::Declaration_Instance *reg = nullptr;
    const IR::Type_Specialized *regtype = nullptr;
    const IR::Type_Extern *seltype = nullptr;
    bool found = self.findSaluDeclarations(ext, &reg, &regtype, &seltype);
    if (!found) return;

    auto *salu = self.salu_inits.at(reg);

    // If the register action hasn't been seen before, this creates an SALU Instruction
    if (register_actions.count(ext) == 0) {
        LOG3("Adding " << ext->name << " to stateful ALU " << salu->name);
        register_actions.insert(ext);
        if (!inst_ctor[salu]) {
            // Create exactly one CreateSaluInstruction per SALU and reuse it for all the
            // instructions in that SALU, so we can accumulate info in it.
            LOG3("Creating instruction at stateful ALU " << salu->name << "...");
            inst_ctor[salu] = new CreateSaluInstruction(salu);
        }
        ext->apply(*inst_ctor[salu]);
    }
}

const IR::MAU::StatefulAlu *AttachTables::DefineGlobalRefs ::findAttachedSalu(
    const IR::Declaration_Instance *ext) {
    const IR::Declaration_Instance *reg = nullptr;
    bool found = self.findSaluDeclarations(ext, &reg);
    if (!found) return nullptr;

    auto *salu = self.salu_inits.at(reg);
    BUG_CHECK(salu,
              "Stateful Alu cannot be found, and should have been initialized within "
              "the previous pass");
    return salu;
}

bool AttachTables::DefineGlobalRefs::preorder(IR::MAU::Table *tbl) {
    LOG3("AttachTables visiting table " << tbl->name);
    return true;
}

bool AttachTables::DefineGlobalRefs::preorder(IR::MAU::Action *act) {
    LOG3("AttachTables visiting action " << act->name);
    return true;
}

void AttachTables::DefineGlobalRefs::postorder(IR::MAU::Table *tbl) {
    for (auto a : attached[tbl->name]) {
        bool already_attached = false;
        for (auto al_a : tbl->attached) {
            if (a->attached == al_a->attached) {
                already_attached = true;
                break;
            }
        }
        if (already_attached) {
            LOG3(a->attached->name << " already attached to " << tbl->name);
        } else {
            LOG3("attaching " << a->attached->name << " to " << tbl->name);
            tbl->attached.push_back(a);
        }
    }
}

void AttachTables::DefineGlobalRefs::postorder(IR::GlobalRef *gref) {
    // expressions might be in two actions (due to inlining), which need to
    // be visited independently
    visitAgain();

    if (auto di = gref->obj->to<IR::Declaration_Instance>()) {
        auto tt = findContext<IR::MAU::Table>();
        BUG_CHECK(tt, "GlobalRef not in a table");
        auto inst = P4::Instantiation::resolve(di, refMap, typeMap);
        const IR::MAU::AttachedMemory *obj = nullptr;
        if (self.converted.count(di)) {
            obj = self.converted.at(di);
            gref->obj = obj;
        } else if (auto att =
                       createAttached(di->srcInfo, di->controlPlaneName(), di->type,
                                      &inst->substitution, inst->typeArguments, di->annotations,
                                      refMap, typeMap, self.stateful_selectors, nullptr, tt)) {
            LOG3("Created " << att->node_type_name() << ' ' << att->name << " (pt 3)");
            gref->obj = self.converted[di] = att;
            obj = att;
        } else if (isSaluActionType(di->type)) {
            auto salu = findAttachedSalu(di);
            // Could be because an earlier pass errored out, and we do not stop_on_error
            // In a non-error case, this should always be non-null, and is checked in a BUG
            if (salu == nullptr) return;
            gref->obj = salu;
            obj = salu;
        }

        if (obj) {
            bool already_attached = false;
            for (auto at : attached[tt->name]) {
                if (at->attached == obj) {
                    already_attached = true;
                    break;
                }
            }
            if (!already_attached) {
                attached[tt->name].push_back(new IR::MAU::BackendAttached(obj->srcInfo, obj));
            }
        }
    }
}

static const IR::MethodCallExpression *isApplyHit(const IR::Expression *e, bool *lnot = 0) {
    if (auto *n = e->to<IR::LNot>()) {
        e = n->expr;
        if (lnot) *lnot = true;
    } else if (lnot) {
        *lnot = false;
    }
    if (auto *mem = e->to<IR::Member>()) {
        if (mem->member != "hit") return nullptr;
        if (auto *mc = mem->expr->to<IR::MethodCallExpression>()) {
            mem = mc->method->to<IR::Member>();
            if (mem && mem->member == "apply") return mc;
        }
    }
    return nullptr;
}

class RewriteActionNames : public Modifier {
    const IR::P4Action *p4_action;
    IR::MAU::Table *mau_table;

 public:
    RewriteActionNames(const IR::P4Action *ac, IR::MAU::Table *tt) : p4_action(ac), mau_table(tt) {}

 private:
    bool preorder(IR::Path *path) {
        if (findOrigCtxt<IR::MethodCallExpression>()) {
            for (auto action : Values(mau_table->actions)) {
                if (action->name.name == p4_action->externalName() &&
                    p4_action->name.name == path->name.name) {
                    path->name.name = p4_action->externalName();
                    path->name.originalName = action->name.originalName;
                    break;
                }
            }
        }
        return false;
    }
};

class GetBackendTables : public MauInspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    gress_t gress;
    const IR::MAU::TableSeq *&rv;
    DeclarationConversions &converted;
    StatefulSelectors &stateful_selectors;
    DeclarationConversions assoc_profiles;
    std::set<cstring> unique_names;
    assoc::map<const IR::Node *, IR::MAU::Table *> tables;
    assoc::map<const IR::Node *, IR::MAU::TableSeq *> seqs;
    CollectSourceInfoLogging &sourceInfoLogging;
    IR::MAU::TableSeq *getseq(const IR::Node *n) {
        if (!seqs.count(n) && tables.count(n)) seqs[n] = new IR::MAU::TableSeq(tables.at(n));
        return seqs.at(n);
    }

 public:
    GetBackendTables(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, gress_t gr,
                     const IR::MAU::TableSeq *&rv, DeclarationConversions &con,
                     StatefulSelectors &ss, CollectSourceInfoLogging &sourceInfoLogging)
        : refMap(refMap),
          typeMap(typeMap),
          gress(gr),
          rv(rv),
          converted(con),
          stateful_selectors(ss),
          sourceInfoLogging(sourceInfoLogging) {}

 private:
    void handle_pragma_ixbar_group_num(const IR::Annotations *annotations, IR::MAU::TableKey *key) {
        if (auto ixbar_num = annotations->getSingle("ixbar_group_num"_cs)) {
            key->ixbar_group_num = getConstant(ixbar_num, 0, 7);
        }
    }

    void setup_match_mask(IR::MAU::Table *tt, const IR::Mask *mask, IR::ID match_id,
                          int p4_param_order, const IR::Annotations *annotations,
                          std::optional<cstring> partition_index) {
        std::optional<cstring> name_annotation = std::nullopt;
        if (auto nameAnn = annotations->getSingle(IR::Annotation::nameAnnotation))
            name_annotation = nameAnn->getName();

        auto slices = convertMaskToSlices(mask);
        for (auto slice : slices) {
            auto ixbar_read = new IR::MAU::TableKey(slice, match_id);
            ixbar_read->from_mask = true;
            if (ixbar_read->for_match()) ixbar_read->p4_param_order = p4_param_order;
            ixbar_read->annotations = annotations;
            if (partition_index && name_annotation && (*partition_index == *name_annotation))
                ixbar_read->partition_index = true;
            handle_pragma_ixbar_group_num(annotations, ixbar_read);
            tt->match_key.push_back(ixbar_read);
        }
    }

    // precondition: type of origExpr is signed or unsigned fixed-width interger
    // (i.e. bit<N> or int<N>)
    IR::Mask *make_mask(const IR::Expression *origExpr, const IR::Expression *left,
                        const IR::Constant *right) {
        auto *type = origExpr->type->to<IR::Type_Bits>();
        BUG_CHECK(type, "Could not extract type of operands of %1%", origExpr);

        if (type->isSigned) {
            // bitcast to unsigned - & has the same semantics anyway
            type = IR::Type_Bits::get(type->srcInfo, type->size, false);
            left = new IR::BFN::ReinterpretCast(left->srcInfo, type, left);
            right = new IR::Constant(right->srcInfo, type, right->value, right->base, true);
        }
        return new IR::Mask(origExpr->srcInfo, type, left, right);
    }

    void setup_tt_match(IR::MAU::Table *tt, const IR::P4Table *table) {
        auto annot = table->getAnnotations();
        // Set compiler generated flag if hidden annotation present
        // Can be on a keyless table, hence we check this at the beginning
        auto h = annot->getSingle("hidden"_cs);
        if (h) tt->is_compiler_generated = true;

        auto *key = table->getKey();
        if (key == nullptr) return;
        int p4_param_order = 0;

        std::optional<cstring> partition_index = std::nullopt;
        // Fold 'atcam_partition_index' annotation into InputXbarRead IR node.
        auto s = annot->getSingle("atcam_partition_index"_cs);
        if (s) partition_index = s->expr.at(0)->to<IR::StringLiteral>()->value;

        for (auto key_elem : key->keyElements) {
            auto key_expr = key_elem->expression;
            IR::ID match_id(key_elem->matchType->srcInfo, key_elem->matchType->path->name);
            if (auto *b_and = key_expr->to<IR::BAnd>()) {
                IR::Mask *mask = nullptr;

                if (b_and->left->is<IR::Constant>())
                    mask = make_mask(b_and, b_and->right, b_and->left->to<IR::Constant>());
                else if (b_and->right->is<IR::Constant>())
                    mask = make_mask(b_and, b_and->left, b_and->right->to<IR::Constant>());
                else
                    error("%s: mask %s must have a constant operand to be used as a table key",
                          b_and->srcInfo, b_and);
                if (mask == nullptr) continue;
                setup_match_mask(tt, mask, match_id, p4_param_order, key_elem->getAnnotations(),
                                 partition_index);
            } else if (auto *mask = key_expr->to<IR::Mask>()) {
                setup_match_mask(tt, mask, match_id, p4_param_order, key_elem->getAnnotations(),
                                 partition_index);
            } else if (match_id.name == "atcam_partition_index") {
                auto ixbar_read = new IR::MAU::TableKey(key_expr, match_id);
                ixbar_read->partition_index = true;
                ixbar_read->p4_param_order = p4_param_order;
                ixbar_read->annotations = key_elem->getAnnotations();
                // annotate if atcam is used in alpm, if yes, the
                // atcam_partition_index field is managed by driver, therefore
                // does not need to be emitted as p4 param in assembly and
                // context.json.
                auto as_alpm = getExpressionFromProperty(table, "as_alpm"_cs);
                if (as_alpm != std::nullopt) {
                    ixbar_read->used_in_alpm = true;
                }
                tt->match_key.push_back(ixbar_read);
            } else {
                auto ixbar_read = new IR::MAU::TableKey(key_expr, match_id);
                if (ixbar_read->for_match()) ixbar_read->p4_param_order = p4_param_order;
                ixbar_read->annotations = key_elem->getAnnotations();

                std::optional<cstring> name_annotation = std::nullopt;
                if (auto nameAnn = key_elem->getAnnotation(IR::Annotation::nameAnnotation))
                    name_annotation = nameAnn->getName();

                if (partition_index && name_annotation && (*partition_index == *name_annotation))
                    ixbar_read->partition_index = true;

                handle_pragma_ixbar_group_num(key_elem->annotations, ixbar_read);
                tt->match_key.push_back(ixbar_read);
            }
            if (match_id.name != "selector" && match_id.name != "dleft_hash") p4_param_order++;
        }
    }

    // This function creates a copy of the static entry list within the match
    // table into the backend MAU::Table. This is required to modify names
    // present on actions which are updated in the backend.
    void update_entries_list(IR::MAU::Table *tt, const IR::P4Action *ac) {
        if (tt->entries_list == nullptr) return;
        auto el = tt->entries_list->apply(RewriteActionNames(ac, tt));
        tt->entries_list = el->to<IR::EntriesList>();
    }

    // Convert from IR::P4Action to IR::MAU::Action
    void setup_actions(IR::MAU::Table *tt, const IR::P4Table *table) {
        auto actionList = table->getActionList();
        if (tt->match_table->getEntries() && tt->match_table->getEntries()->size())
            tt->entries_list = tt->match_table->getEntries();
        for (auto act : actionList->actionList) {
            auto decl = refMap->getDeclaration(act->getPath())->to<IR::P4Action>();
            BUG_CHECK(decl != nullptr, "Table %s actions property cannot contain non-action entry",
                      table->name);
            // act->expression can be either PathExpression or MethodCallExpression, but
            // the createBuiltin pass in frontend has already converted IR::PathExpression
            // to IR::MethodCallExpression.
            auto mce = act->expression->to<IR::MethodCallExpression>();
            auto newaction = createActionFunction(
                decl, mce->arguments,
                // if this is a @hidden table it was probably created from statements in
                // the apply, so include that context when looking for @in_hash annotations
                table->getAnnotations()->getSingle("hidden"_cs) ? getContext() : nullptr);
            SetupActionProperties sap(table, act, refMap);
            auto newaction_props = newaction->apply(sap)->to<IR::MAU::Action>();
            if (!tt->actions.count(newaction_props->name.originalName))
                tt->actions.emplace(newaction_props->name.originalName, newaction_props);
            else
                error("%s: action %s appears multiple times in table %s", decl->name.srcInfo,
                      decl->name, tt->name);
            update_entries_list(tt, decl);
        }
    }

    bool preorder(const IR::IndexedVector<IR::Declaration> *) override { return false; }
    bool preorder(const IR::P4Table *) override { return false; }

    bool preorder(const IR::BlockStatement *b) override {
        assert(!seqs.count(b));
        seqs[b] = new IR::MAU::TableSeq();
        return true;
    }
    void postorder(const IR::BlockStatement *b) override {
        for (auto el : b->components) {
            if (tables.count(el)) seqs.at(b)->tables.push_back(tables.at(el));
            if (seqs.count(el))
                for (auto tbl : seqs.at(el)->tables) seqs.at(b)->tables.push_back(tbl);
        }
    }
    bool preorder(const IR::MethodCallExpression *m) override {
        auto mi = P4::MethodInstance::resolve(m, refMap, typeMap, true);
        if (!mi || !mi->isApply()) {
            // it's possible that an extern function is invoked in the
            // apply statement, e.g.
            // if (isValidate(ig_intr_dprsr_md.mirror_type)) { ... }
            return false;
        }
        auto table = mi->object->to<IR::P4Table>();
        if (!table) BUG("%1% not apllied to table", m);
        if (!tables.count(m)) {
            auto tt = tables[m] =
                new IR::MAU::Table(cstring::make_unique(unique_names, table->name), gress, table);
            unique_names.insert(tt->name);
            tt->match_table = table =
                table
                    ->apply(FixP4Table(refMap, typeMap, tt, unique_names, converted,
                                       stateful_selectors, assoc_profiles))
                    ->to<IR::P4Table>();
            setup_tt_match(tt, table);
            setup_actions(tt, table);
            LOG3("tt " << tt);
        } else {
            error("%s: Multiple applies of table %s not supported", m->srcInfo, table->name);
        }
        return true;
    }
    void postorder(const IR::MethodCallStatement *m) override {
        if (!tables.count(m->methodCall)) BUG("MethodCall %1% is not apply", m);
        tables[m] = tables.at(m->methodCall);
    }
    void postorder(const IR::SwitchStatement *s) override {
        auto exp = s->expression->to<IR::Member>();
        if (!exp || exp->member != "action_run" || !tables.count(exp->expr)) {
            error("%s: Can only switch on table.apply().action_run", s->expression->srcInfo);
            return;
        }
        auto tt = tables[s] = tables.at(exp->expr);
        safe_vector<cstring> fallthrough;
        for (auto c : s->cases) {
            cstring label;
            if (c->label->is<IR::DefaultExpression>()) {
                label = "$default"_cs;
            } else {
                label = refMap->getDeclaration(c->label->to<IR::PathExpression>()->path)
                            ->getName()
                            .originalName;
                if (tt->actions.at(label)->exitAction) {
                    warning("Action %s in table %s exits unconditionally", c->label,
                            tt->externalName());
                    label = cstring();
                }
            }
            if (c->statement) {
                auto n = getseq(c->statement);
                if (label) tt->next.emplace(label, n);
                for (auto ft : fallthrough) tt->next.emplace(ft, n);
                fallthrough.clear();
            } else if (label) {
                fallthrough.push_back(label);
            }
        }
    }
    bool preorder(const IR::IfStatement *c) override {
        if (!isApplyHit(c->condition)) {
            static unsigned uid = 0;
            char buf[16];
            snprintf(buf, sizeof(buf), "cond-%d", ++uid);
            tables[c] = new IR::MAU::Table(cstring(buf), gress, c->condition);

            sourceInfoLogging.addSymbol(CollectSourceInfoLogging::Symbol(
                cstring(buf), c->node_type_name(), c->getSourceInfo()));
        }
        return true;
    }
    void postorder(const IR::IfStatement *c) override {
        bool lnot;
        cstring T = "$true"_cs, F = "$false"_cs;
        if (auto *mc = isApplyHit(c->condition, &lnot)) {
            tables[c] = tables.at(mc);
            T = lnot ? "$miss"_cs : "$hit"_cs;
            F = lnot ? "$hit"_cs : "$miss"_cs;
        }
        if (c->ifTrue && !c->ifTrue->is<IR::EmptyStatement>())
            tables.at(c)->next.emplace(T, getseq(c->ifTrue));
        if (c->ifFalse && !c->ifFalse->is<IR::EmptyStatement>())
            tables.at(c)->next.emplace(F, getseq(c->ifFalse));
    }

    // need to understand architecture to process the correct control block
    bool preorder(const IR::BFN::TnaControl *cf) override {
        visit(cf->body);
        rv = getseq(cf->body);
        return false;
    }

    bool preorder(const IR::EmptyStatement *) override { return false; }
    void postorder(const IR::Statement *st) override { BUG("Unhandled statement %1%", st); }

    void end_apply() override {
        // FIXME -- if both ingress and egress contain method calls to the same
        // extern object, and that object contains any method calls, this will end
        // up duplicating that object.  Currently there are no extern objects that can
        // both contain code and be shared between threads, so that can't happen
        rv = rv->apply(ConvertMethodCalls(refMap, typeMap));
    }
};

class ExtractMetadata : public Inspector {
 public:
    ExtractMetadata(IR::BFN::Pipe *rv, ParamBinding *bindings) : rv(rv), bindings(bindings) {
        setName("ExtractMetadata");
    }

    const IR::Parameter *getParameterByTypeName(const IR::ParameterList *params,
                                                cstring type_name) {
        const IR::Parameter *retval = nullptr;
        for (auto &param : *params) {
            if (auto type = param->type->to<IR::Type_StructLike>()) {
                if (type->name.name == type_name) {
                    retval = param;
                    break;
                }
            } else {
                error("Unsupported parameter type %1%", param->type);
            }
        }
        return retval;
    }

    void postorder(const IR::BFN::TnaControl *mau) override {
        gress_t gress = mau->thread;

        for (auto &t : Device::archSpec().getMAUIntrinsicTypes(gress)) {
            if (auto param = getParameterByTypeName(mau->getApplyParameters(), t.type)) {
                rv->metadata.addUnique(t.name, bindings->get(param)->obj);
            }
        }

        // only used to support v1model
        if (gress == INGRESS) {
            if (auto param = getParameterByTypeName(mau->getApplyParameters(),
                                                    "compiler_generated_metadata_t"_cs)) {
                rv->metadata.addUnique(COMPILER_META,
                                       bindings->get(param)->obj->to<IR::Metadata>());
            }
        }
    }

 private:
    IR::BFN::Pipe *rv;
    ParamBinding *bindings;
};

/**
 * \ingroup ExtractChecksum ExtractChecksum
 * \brief Pass that extracts checksums from deparser.
 */
struct ExtractChecksum : public Inspector {
    explicit ExtractChecksum(IR::BFN::Pipe *rv) : rv(rv) { setName("ExtractChecksumNative"); }

    void postorder(const IR::BFN::TnaDeparser *deparser) override {
        extractChecksumFromDeparser(deparser, rv);
    }

    IR::BFN::Pipe *rv;
};

// used by backend for tna architecture
ProcessBackendPipe::ProcessBackendPipe(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                       IR::BFN::Pipe *rv, DeclarationConversions &converted,
                                       StatefulSelectors ss, ParamBinding *bindings) {
    setName("ProcessBackendPipe");
    CHECK_NULL(bindings);
    auto simplifyReferences = new SimplifyReferences(bindings, refMap, typeMap);
    auto useV1model = (BackendOptions().arch == "v1model");
    addPasses({
        new AttachTables(refMap, typeMap, converted, ss),  // add attached tables
        new ProcessParde(rv, useV1model),                  // add parde metadata
        /// followings two passes are necessary, because ProcessBackendPipe transforms the
        /// IR::BFN::Pipe objects. If all the above passes can be moved to an earlier midend
        /// pass, then the passes below can possibily be removed.
        simplifyReferences,
    });
}

cstring BackendConverter::getPipelineName(const IR::P4Program *program, int index) {
    auto mainDecls = program->getDeclsByName("main"_cs)->toVector();
    if (mainDecls.size() == 0) {
        error("No main declaration in the program");
        return nullptr;
    } else if (mainDecls.size() > 1) {
        error("Multiple main declarations in the program");
        return nullptr;
    }
    auto decl = mainDecls.at(0);
    if (auto di = decl->to<IR::Declaration_Instance>()) {
        if (auto expr = di->arguments->at(index)->expression) {
            if (auto path = expr->to<IR::PathExpression>()) {
                auto name = path->path->name;
                return name;
            }
        }
    }

    BUG_CHECK(toplevel, "unable to get pipeline name");
    // If no declaration found (anonymous instantiation) we get the pipe name
    // from arch definition
    auto main = toplevel->getMain();
    BUG_CHECK(main, "Cannot find main block");
    auto cparams = main->getConstructorParameters();
    BUG_CHECK(cparams, "Main block does not have constructor parameter");
    auto idxParam = cparams->getParameter(index);
    BUG_CHECK(idxParam, "No constructor parameter found for index %d in main", index);
    return idxParam->name;
}

// custom visitor to the main package block to generate the
// toplevel pipeline structure.
bool BackendConverter::preorder(const IR::P4Program *program) {
    ApplyEvaluator eval(refMap, typeMap);
    auto *new_program = program->apply(eval);

    toplevel = eval.getToplevelBlock();
    BUG_CHECK(toplevel, "toplevel cannot be nullptr");

    auto main = toplevel->getMain();
    main->apply(*arch);

    /// setup the context to know which pipes are available in the program: for logging and
    /// other output declarations.
    BFNContext::get().discoverPipes(new_program, toplevel);
    BUG_CHECK(toplevel, "toplevel cannot be nullptr");

    /// SimplifyReferences passes are fixup passes that modifies the visited IR tree.
    /// Unfortunately, the modifications by simplifyReferences will transform IR tree towards
    /// the backend IR, which means we can no longer run typeCheck pass after applying
    /// simplifyReferences to the frontend IR.
    auto simplifyReferences = new SimplifyReferences(bindings, refMap, typeMap);

    // collect and set global pragmas
    CollectGlobalPragma collect_pragma;
    new_program->apply(collect_pragma);

    // Save valid pipes and valid ghost pipes on program. This info is relevant
    // mainly for multipipe programs and to give access to each pipe compilation
    // on how many ghost threads and pipes are in use
    auto npipe = 0;
    auto &options = BackendOptions();
    for (auto pkg : main->constantValue) {
        if (!pkg.second) continue;
        if (!pkg.second->is<IR::PackageBlock>()) continue;

        options.pipes |= (0x1 << npipe);

        if (arch->pipelines.getPipeline(npipe).threads.count(GHOST)) {
            options.ghost_pipes |= (0x1 << npipe);
        }

        npipe++;
    }

    npipe = 0;
    for (const auto &arch_pipe : arch->pipelines.getPipelines()) {
        DeclarationConversions converted;
        auto rv = new IR::BFN::Pipe(arch_pipe.names, arch_pipe.ids);
        auto &threads = arch_pipe.threads;
        std::list<gress_t> gresses = {INGRESS, EGRESS};

        for (auto gress : gresses) {
            if (!threads.count(gress)) {
                error("Unable to find thread %1%", arch_pipe.names.front());
                return false;
            }
            auto thread = threads.at(gress);
            thread = thread->apply(*simplifyReferences);
            if (auto mau = thread->mau->to<IR::BFN::TnaControl>()) {
                mau->apply(ExtractMetadata(rv, bindings));
                mau->apply(GetBackendTables(refMap, typeMap, gress, rv->thread[gress].mau,
                                            converted, stateful_selectors, sourceInfoLogging));
            }
            for (auto p : thread->parsers) {
                if (auto parser = p->to<IR::BFN::TnaParser>()) {
                    parser->apply(ExtractParser(refMap, typeMap, rv, arch));
                }
            }
            if (auto dprsr = dynamic_cast<const IR::BFN::TnaDeparser *>(thread->deparser)) {
                dprsr->apply(ExtractDeparser(refMap, typeMap, rv));
                dprsr->apply(ExtractChecksum(rv));
            } else {
                BUG("missing deparser");
            }
        }

        // Enable ghost parser on all pipes in program if it has at least one
        // ghost control
        if (options.ghost_pipes > 0) {
            auto gh_intr_md_fields = IR::IndexedVector<IR::StructField>(
                {new IR::StructField("ping_pong"_cs, IR::Type_Bits::get(1)),
                 new IR::StructField("qlength"_cs, IR::Type_Bits::get(18)),
                 new IR::StructField("qid"_cs, IR::Type_Bits::get(11)),
                 new IR::StructField("pipe_id"_cs, IR::Type_Bits::get(2))});
            auto ghost_type =
                new IR::Type_Header(IR::ID("ghost_intrinsic_metadata_t"), gh_intr_md_fields);
            auto ghost_hdr = new IR::Header(IR::ID("gh_intr_md"), ghost_type);
            auto ghost_conc_hdr = new IR::ConcreteHeaderRef(ghost_hdr);
            auto ghost_md = new IR::BFN::FieldLVal(ghost_conc_hdr);
            rv->ghost_thread.ghost_parser = new IR::BFN::GhostParser(
                INGRESS, ghost_md, IR::ID("ghost_parser"), rv->canon_name());
        }

        if (threads.count(GHOST)) {
            auto thread = threads.at(GHOST);
            thread = thread->apply(*simplifyReferences);
            if (auto mau = thread->mau->to<IR::BFN::TnaControl>()) {
                mau->apply(ExtractMetadata(rv, bindings));
                mau->apply(GetBackendTables(refMap, typeMap, GHOST, rv->ghost_thread.ghost_mau,
                                            converted, stateful_selectors, sourceInfoLogging));
            }
        }

        rv->global_pragmas = collect_pragma.global_pragmas();

        ProcessBackendPipe processBackendPipe(refMap, typeMap, rv, converted, stateful_selectors,
                                              bindings);

        auto p = rv->apply(processBackendPipe);

        pipe.push_back(p);
        for (size_t npipe : arch_pipe.ids) {
            pipes.emplace(npipe, p);
        }
    }

    return false;
}

}  // namespace BFN
