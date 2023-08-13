/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "action.h"

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include "backends/bmv2/common/JsonObjects.h"
#include "backends/bmv2/common/expression.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/programStructure.h"
#include "extern.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "lib/enumerator.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/ordered_map.h"

namespace BMV2 {

cstring ActionConverter::jsonAssignment(const IR::Type *type, bool inParser) {
    if (!inParser && type->is<IR::Type_Varbits>()) return "assign_VL";
    if (type->is<IR::Type_HeaderUnion>()) return "assign_union";
    if (type->is<IR::Type_Header>() || type->is<IR::Type_Struct>()) return "assign_header";
    if (auto ts = type->to<IR::Type_Stack>()) {
        auto et = ts->elementType;
        if (et->is<IR::Type_HeaderUnion>())
            return "assign_union_stack";
        else
            return "assign_header_stack";
    }
    if (inParser)
        // Unfortunately set can do some things that assign cannot,
        // e.g., handle lookahead on the RHS.
        return "set";
    else
        return "assign";
}

void ActionConverter::convertActionBody(const IR::Vector<IR::StatOrDecl> *body,
                                        Util::JsonArray *result) {
    for (auto s : *body) {
        // TODO(jafingerhut) - add line/col at all individual cases below,
        // or perhaps it can be done as a common case above or below
        // for all of them?

        IR::MethodCallExpression *mce2 = nullptr;
        auto isR = false;
        if (s->is<IR::AssignmentStatement>()) {
            auto assign = s->to<IR::AssignmentStatement>();
            const IR::Expression *l, *r;
            l = assign->left;
            r = assign->right;
            auto mce = r->to<IR::MethodCallExpression>();
            if (mce != nullptr) {
                auto mi = P4::MethodInstance::resolve(mce, ctxt->refMap, ctxt->typeMap);
                auto em = mi->to<P4::ExternMethod>();
                if (em != nullptr) {
                    if ((em->originalExternType->name.name == "Register" ||
                         em->method->name.name == "read") ||
                        (em->originalExternType->name.name == "Meter" &&
                         em->method->name.name == "execute")) {
                        isR = true;
                        // l = l->to<IR::PathExpression>();
                        // BUG_CHECK(l != nullptr, "register_read dest cast failed");
                        auto dest = new IR::Argument(l);
                        auto args = new IR::Vector<IR::Argument>();
                        args->push_back(dest);                   // dest
                        args->push_back(mce->arguments->at(0));  // index
                        mce2 = new IR::MethodCallExpression(mce->method, mce->typeArguments);
                        mce2->arguments = args;
                        s = new IR::MethodCallStatement(mce);
                    } else if (em->originalExternType->name.name == "Hash" ||
                               em->method->name.name == "get_hash") {
                        isR = true;
                        auto dest = new IR::Argument(l);
                        auto args = new IR::Vector<IR::Argument>();
                        args->push_back(dest);
                        BUG_CHECK(mce->arguments->size() == 1 || mce->arguments->size() == 3,
                                  "Expected 1 or 3 argument for %1%", mce);
                        if (mce->arguments->size() == 3) {
                            args->push_back(mce->arguments->at(0));  // base
                            args->push_back(mce->arguments->at(1));  // data
                            args->push_back(mce->arguments->at(2));  // max
                        } else if (mce->arguments->size() == 1) {
                            args->push_back(mce->arguments->at(0));  // data
                        }
                        mce2 = new IR::MethodCallExpression(mce->method, mce->typeArguments);
                        mce2->arguments = args;
                        s = new IR::MethodCallStatement(mce);
                    }
                }
            }
        }

        if (!s->is<IR::Statement>()) {
            continue;
        } else if (auto block = s->to<IR::BlockStatement>()) {
            convertActionBody(&block->components, result);
            continue;
        } else if (s->is<IR::ReturnStatement>()) {
            break;
        } else if (s->is<IR::ExitStatement>()) {
            auto primitive = mkPrimitive("exit", result);
            (void)mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            break;
        } else if (s->is<IR::AssignmentStatement>()) {
            const IR::Expression *l, *r;
            auto assign = s->to<IR::AssignmentStatement>();
            l = assign->left;
            r = assign->right;
            auto type = ctxt->typeMap->getType(l, true);
            cstring operation = jsonAssignment(type, false);
            auto primitive = mkPrimitive(operation, result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", assign->sourceInfoJsonObj());
            bool convertBool = type->is<IR::Type_Boolean>();
            Util::IJson *left;
            if (ctxt->conv->isArrayIndexRuntime(l)) {
                left = ctxt->conv->convert(l, true, true, convertBool);
            } else {
                left = ctxt->conv->convertLeftValue(l);
            }
            parameters->append(left);
            auto right = ctxt->conv->convert(r, true, true, convertBool);
            parameters->append(right);
            continue;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::MethodCallStatement>()) {
            LOG3("Visit " << dbp(s));
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc, ctxt->refMap, ctxt->typeMap);
            if (mi->is<P4::ActionCall>()) {
                BUG("%1%: action call should have been inlined", mc);
                continue;
            } else if (mi->is<P4::BuiltInMethod>()) {
                auto builtin = mi->to<P4::BuiltInMethod>();

                cstring prim;
                auto parameters = new Util::JsonArray();
                auto obj = ctxt->conv->convert(builtin->appliedTo);
                parameters->append(obj);

                if (builtin->name == IR::Type_Header::setValid) {
                    prim = "add_header";
                } else if (builtin->name == IR::Type_Header::setInvalid) {
                    prim = "remove_header";
                } else if (builtin->name == IR::Type_Stack::push_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = ctxt->conv->convert(mc->arguments->at(0)->expression);
                    prim = "push";
                    parameters->append(arg);
                } else if (builtin->name == IR::Type_Stack::pop_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = ctxt->conv->convert(mc->arguments->at(0)->expression);
                    prim = "pop";
                    parameters->append(arg);
                } else {
                    BUG("%1%: Unexpected built-in method", s);
                }
                auto primitive = mkPrimitive(prim, result);
                primitive->emplace("parameters", parameters);
                primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
                continue;
            } else if (mi->is<P4::ExternMethod>()) {
                auto em = mi->to<P4::ExternMethod>();
                LOG3("P4V1:: convert " << s);
                Util::IJson *json;
                if (isR) {
                    json = ExternConverter::cvtExternObject(ctxt, em, mce2, s, emitExterns);
                } else {
                    json = ExternConverter::cvtExternObject(ctxt, em, mc, s, emitExterns);
                }
                if (json) result->append(json);
                continue;
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
                auto json = ExternConverter::cvtExternFunction(ctxt, ef, mc, s, emitExterns);
                if (json) result->append(json);
                continue;
            }
        }
        ::error(ErrorType::ERR_UNSUPPORTED, "%1% not yet supported on this target", s);
    }
}

void ActionConverter::convertActionParams(const IR::ParameterList *parameters,
                                          Util::JsonArray *params) {
    for (auto p : *parameters->getEnumerator()) {
        if (!ctxt->refMap->isUsed(p))
            warn(ErrorType::WARN_UNUSED, "Unused action parameter %1%", p);

        auto param = new Util::JsonObject();
        param->emplace("name", p->externalName());
        auto type = ctxt->typeMap->getType(p, true);
        // TODO: added IR::Type_Enum here to support PSA_MeterColor_t
        // should re-consider how to support action parameters that is neither bit<> nor int<>
        if (!(type->is<IR::Type_Bits>() || type->is<IR::Type_Enum>()))
            ::error(ErrorType::ERR_INVALID,
                    "%1%: action parameters must be bit<> or int<> on this target", p);
        param->emplace("bitwidth", type->width_bits());
        params->append(param);
    }
}

void ActionConverter::postorder(const IR::P4Action *action) {
    cstring name = action->controlPlaneName();
    auto params = new Util::JsonArray();
    convertActionParams(action->parameters, params);
    auto body = new Util::JsonArray();
    convertActionBody(&action->body->components, body);
    auto id = ctxt->json->add_action(name, params, body);
    LOG3("add action with id " << id << " name " << name << " " << action);
    ctxt->structure->ids.emplace(action, id);
}

}  // namespace BMV2
