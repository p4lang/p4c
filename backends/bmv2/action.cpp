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

namespace BMV2 {

#ifdef PSA
void
ConvertActions::genExternMethod(Util::JsonArray* result, P4::ExternMethod *em) {
    auto name = "_" + em->actualExternType->name + "_" + em->method->name.name;
    auto primitive = mkPrimitive(name, result);
    auto parameters = mkParameters(primitive);

    auto ext = new Util::JsonObject();
    ext->emplace("type", "extern");

    // FIXME: PSA have extern pass building a map and lookup here.
    if (em->object->is<IR::Parameter>()) {
        auto param = em->object->to<IR::Parameter>();
        // auto packageObject = resolveParameter(param);
        // ext->emplace("value", packageObject->getName());
        ext->emplace("value", "FIXME");
    } else {
        ext->emplace("value", em->object->getName());
    }
    parameters->append(ext);

    for (auto a : *mc->arguments) {
        auto arg = conv->convert(a);
        parameters->append(arg);
    }
}
#endif


void
ConvertActions::convertActionBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result) {
    // FIXME: backend->getExpressionConverter()->createFieldList = true??
    for (auto s : *body) {
        // TODO(jafingerhut) - add line/col at all individual cases below,
        // or perhaps it can be done as a common case above or below
        // for all of them?
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
            const IR::Expression* l, *r;
            auto assign = s->to<IR::AssignmentStatement>();
            l = assign->left;
            r = assign->right;

            cstring operation;
            auto type = backend->getTypeMap()->getType(l, true);
            if (type->is<IR::Type_StructLike>())
                operation = "copy_header";
            else
                operation = "modify_field";
            auto primitive = mkPrimitive(operation, result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", assign->sourceInfoJsonObj());
            auto left = backend->getExpressionConverter()->convertLeftValue(l);
            parameters->append(left);
            bool convertBool = type->is<IR::Type_Boolean>();
            auto right = backend->getExpressionConverter()->convert(r, true, true, convertBool);
            parameters->append(right);
            continue;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::MethodCallStatement>()) {
            LOG1("Visit " << dbp(s));
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc, backend->getRefMap(), backend->getTypeMap());
            if (mi->is<P4::ActionCall>()) {
                BUG("%1%: action call should have been inlined", mc);
                continue;
            } else if (mi->is<P4::BuiltInMethod>()) {
                auto builtin = mi->to<P4::BuiltInMethod>();

                cstring prim;
                auto parameters = new Util::JsonArray();
                auto obj = backend->getExpressionConverter()->convert(builtin->appliedTo);
                parameters->append(obj);

                if (builtin->name == IR::Type_Header::setValid) {
                    prim = "add_header";
                } else if (builtin->name == IR::Type_Header::setInvalid) {
                    prim = "remove_header";
                } else if (builtin->name == IR::Type_Stack::push_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = backend->getExpressionConverter()->convert(mc->arguments->at(0));
                    prim = "push";
                    parameters->append(arg);
                } else if (builtin->name == IR::Type_Stack::pop_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = backend->getExpressionConverter()->convert(mc->arguments->at(0));
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
#ifdef PSA
                    genExternMethod(result, em);
#else
                    LOG1("P4V1:: convert " << s);
                    P4V1::V1Model::convertExternObjects(result, backend, em, mc, s);
#endif
                continue;
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
#ifdef PSA
                auto primitive = mkPrimitive(ef->method->name.name, result);
                auto parameters = mkParameters(primitive);
                primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
                for (auto a : *mc->arguments) {
                    parameters->append(conv->convert(a));
                }
#else
                P4V1::V1Model::convertExternFunctions(result, backend, ef, mc, s);
#endif
                continue;
            }
        }
        ::error("%1%: not yet supported on this target", s);
    }
}

void
ConvertActions::convertActionParams(const IR::ParameterList *parameters, Util::JsonArray* params) {
    for (auto p : *parameters->getEnumerator()) {
        if (!backend->getRefMap()->isUsed(p))
            ::warning("Unused action parameter %1%", p);

        auto param = new Util::JsonObject();
        param->emplace("name", p->name);
        auto type = backend->getTypeMap()->getType(p, true);
        if (!type->is<IR::Type_Bits>())
            ::error("%1%: Action parameters can only be bit<> or int<> on this target", p);
        param->emplace("bitwidth", type->width_bits());
        params->append(param);
    }
}

void
ConvertActions::createActions(Util::JsonArray* actions) {
    for (auto it : backend->getStructure().actions) {
        auto action = it.first;
        cstring name = extVisibleName(action);
        auto params = new Util::JsonArray();
        convertActionParams(action->parameters, params);
        auto body = new Util::JsonArray();
        convertActionBody(&action->body->components, body);
        auto id = backend->bm->add_action(name, &params, &body);
        backend->getStructure().ids.emplace(action, id);
    }
}

void
ConvertActions::end_apply(const IR::Node* node) {
    createActions(backend->bm->actions);
}

}
