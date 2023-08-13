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

#include "deparser.h"

#include <algorithm>
#include <string>
#include <vector>

#include "backends/bmv2/common/JsonObjects.h"
#include "backends/bmv2/common/expression.h"
#include "backends/bmv2/common/programStructure.h"
#include "extern.h"
#include "frontends/common/model.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace BMV2 {

void DeparserConverter::convertDeparserBody(const IR::Vector<IR::StatOrDecl> *body,
                                            Util::JsonArray *order, Util::JsonArray *primitives) {
    ctxt->conv->simpleExpressionsOnly = true;
    for (auto s : *body) {
        auto isR = false;
        IR::MethodCallExpression *mce2 = nullptr;
        if (auto block = s->to<IR::BlockStatement>()) {
            convertDeparserBody(&block->components, order, primitives);
            continue;
        } else if (s->is<IR::ReturnStatement>() || s->is<IR::ExitStatement>()) {
            break;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::AssignmentStatement>()) {
            auto assign = s->to<IR::AssignmentStatement>();
            if (assign->right->is<IR::MethodCallExpression>()) {
                auto mce = assign->right->to<IR::MethodCallExpression>();
                auto minst = P4::MethodInstance::resolve(mce, ctxt->refMap, ctxt->typeMap);
                if (minst->is<P4::ExternMethod>()) {
                    auto extmeth = minst->to<P4::ExternMethod>();
                    // PSA backend extern
                    if ((extmeth->method->name.name == "get" ||
                         extmeth->method->name.name == "get_state") &&
                        extmeth->originalExternType->name == "InternetChecksum") {
                        const IR::Expression *l;
                        l = assign->left;
                        isR = true;
                        auto dest = new IR::Argument(l);
                        auto args = new IR::Vector<IR::Argument>();
                        args->push_back(dest);  // dest
                        mce2 = new IR::MethodCallExpression(mce->method, mce->typeArguments);
                        mce2->arguments = args;
                        s = new IR::MethodCallStatement(mce);
                    }
                }
            } else if (assign->right->is<IR::Constant>()) {
                ctxt->conv->simpleExpressionsOnly = false;
                auto json = new Util::JsonObject();
                auto params = mkArrayField(json, "parameters");
                auto type = ctxt->typeMap->getType(assign->left, true);
                json->emplace("op", "set");
                auto l = ctxt->conv->convertLeftValue(assign->left);
                bool convertBool = type->is<IR::Type_Boolean>();
                auto r = ctxt->conv->convert(assign->right, true, true, convertBool);
                params->append(l);
                params->append(r);
                primitives->append(json);
                ctxt->conv->simpleExpressionsOnly = true;
                continue;
            }
        }
        if (s->is<IR::MethodCallStatement>()) {
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc, ctxt->refMap, ctxt->typeMap);
            if (mi->is<P4::ExternMethod>()) {
                auto em = mi->to<P4::ExternMethod>();
                if (em->originalExternType->name.name == corelib.packetOut.name) {
                    if (em->method->name.name == corelib.packetOut.emit.name) {
                        BUG_CHECK(mc->arguments->size() == 1, "Expected exactly 1 argument for %1%",
                                  mc);
                        auto arg = mc->arguments->at(0);
                        auto type = ctxt->typeMap->getType(arg, true);
                        if (type->is<IR::Type_Header>()) {
                            auto j = ctxt->conv->convert(arg->expression);
                            auto val = j->to<Util::JsonObject>()->get("value");
                            order->append(val);
                        } else {
                            // We don't need to handle other types,
                            // like header unions or stacks; they were
                            // expanded by the expandEmit pass.
                            ::error(ErrorType::ERR_UNSUPPORTED,
                                    "%1%: emit only supports header arguments, not %2%", arg, type);
                        }
                    }
                    continue;
                } else if (em->originalExternType->name == "InternetChecksum" &&
                           (em->method->name.name == "clear" || em->method->name.name == "add" ||
                            em->method->name.name == "subtract" ||
                            em->method->name.name == "get_state" ||
                            em->method->name.name == "set_state" ||
                            em->method->name.name == "get")) {
                    // PSA backend extern
                    ctxt->conv->simpleExpressionsOnly = false;
                    Util::IJson *json;
                    if (isR) {
                        json = ExternConverter::cvtExternObject(ctxt, em, mce2, s, true);
                    } else {
                        json = ExternConverter::cvtExternObject(ctxt, em, mc, s, true);
                    }
                    if (json) primitives->append(json);
                    ctxt->conv->simpleExpressionsOnly = true;
                    continue;
                }
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
                ctxt->conv->simpleExpressionsOnly = false;
                auto json =
                    ExternConverter::cvtExternFunction(ctxt, ef, mc, s, /* emitExterns */ true);
                ctxt->conv->simpleExpressionsOnly = true;
                if (json) primitives->append(json);
                continue;
            }
        }
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: not supported within a deparser on this target",
                s);
    }
    ctxt->conv->simpleExpressionsOnly = false;
}

Util::IJson *DeparserConverter::convertDeparser(const IR::P4Control *ctrl) {
    auto result = new Util::JsonObject();
    result->emplace("name", name);
    result->emplace("id", nextId("deparser"));
    result->emplace_non_null("source_info", ctrl->sourceInfoJsonObj());
    auto order = mkArrayField(result, "order");
    auto primitives = mkArrayField(result, "primitives");
    convertDeparserBody(&ctrl->body->components, order, primitives);
    return result;
}

bool DeparserConverter::preorder(const IR::P4Control *control) {
    auto deparserJson = convertDeparser(control);
    ctxt->json->deparsers->append(deparserJson);
    for (auto c : control->controlLocals) {
        if (c->is<IR::Declaration_Constant>() || c->is<IR::Declaration_Variable>() ||
            c->is<IR::P4Action>() || c->is<IR::P4Table>())
            continue;
        if (c->is<IR::Declaration_Instance>()) {
            auto bl = ctxt->structure->resourceMap.at(c);
            CHECK_NULL(bl);
            if (bl->is<IR::ExternBlock>()) {
                auto eb = bl->to<IR::ExternBlock>();
                ExternConverter::cvtExternInstance(ctxt, c, eb, true);
                continue;
            }
        }
        P4C_UNIMPLEMENTED("%1%: not yet handled", c);
    }
    return false;
}

}  // namespace BMV2
