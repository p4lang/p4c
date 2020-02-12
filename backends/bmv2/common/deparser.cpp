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

#include "backend.h"
#include "deparser.h"
#include "extern.h"

namespace BMV2 {

void DeparserConverter::convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body,
                                          Util::JsonArray* order, Util::JsonArray* primitives) {
    ctxt->conv->simpleExpressionsOnly = true;
    for (auto s : *body) {
        if (auto block = s->to<IR::BlockStatement>()) {
            convertDeparserBody(&block->components, order, primitives);
            continue;
        } else if (s->is<IR::ReturnStatement>() || s->is<IR::ExitStatement>()) {
            break;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::MethodCallStatement>()) {
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc, ctxt->refMap, ctxt->typeMap);
            if (mi->is<P4::ExternMethod>()) {
                auto em = mi->to<P4::ExternMethod>();
                if (em->originalExternType->name.name == corelib.packetOut.name) {
                    if (em->method->name.name == corelib.packetOut.emit.name) {
                        BUG_CHECK(mc->arguments->size() == 1,
                                  "Expected exactly 1 argument for %1%", mc);
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
                                    "%1%: emit only supports header arguments, not %2%",
                                    arg, type);
                        }
                    }
                    continue;
                }
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
                ctxt->conv->simpleExpressionsOnly = false;
                auto json = ExternConverter::cvtExternFunction(ctxt, ef, mc,
                                                                s, /* emitExterns */ true);
                ctxt->conv->simpleExpressionsOnly = true;
                if (json)
                    primitives->append(json);
                continue;
            }
        }
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: not supported within a deparser on this target", s);
    }
    ctxt->conv->simpleExpressionsOnly = false;
}

Util::IJson* DeparserConverter::convertDeparser(const IR::P4Control* ctrl) {
    auto result = new Util::JsonObject();
    result->emplace("name", name);
    result->emplace("id", nextId("deparser"));
    result->emplace_non_null("source_info", ctrl->sourceInfoJsonObj());
    auto order = mkArrayField(result, "order");
    auto primitives = mkArrayField(result, "primitives");
    convertDeparserBody(&ctrl->body->components, order, primitives);
    return result;
}

bool DeparserConverter::preorder(const IR::P4Control* control) {
    auto deparserJson = convertDeparser(control);
    ctxt->json->deparsers->append(deparserJson);
    return false;
}

}  // namespace BMV2
