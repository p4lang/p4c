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

#include "parser.h"
#include "JsonObjects.h"
#include "backend.h"

namespace BMV2 {

cstring ParserConverter::jsonAssignment(const IR::Type* type, bool inParser) {
    if (!inParser && type->is<IR::Type_Varbits>())
        return "assign_VL";
    if (type->is<IR::Type_HeaderUnion>())
        return "assign_union";
    if (type->is<IR::Type_Header>() || type->is<IR::Type_Struct>())
        return "assign_header";
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

Util::IJson* ParserConverter::convertParserStatement(const IR::StatOrDecl* stat) {
    auto result = new Util::JsonObject();
    auto params = mkArrayField(result, "parameters");
    if (stat->is<IR::AssignmentStatement>()) {
        auto assign = stat->to<IR::AssignmentStatement>();
        auto type = ctxt->typeMap->getType(assign->left, true);
        cstring operation = jsonAssignment(type, true);
        result->emplace("op", operation);
        auto l = ctxt->conv->convertLeftValue(assign->left);
        bool convertBool = type->is<IR::Type_Boolean>();
        auto r = ctxt->conv->convert(assign->right, true, true, convertBool);
        params->append(l);
        params->append(r);

        if (operation != "set") {
            // must wrap into another outer object
            auto wrap = new Util::JsonObject();
            wrap->emplace("op", "primitive");
            auto params = mkParameters(wrap);
            params->append(result);
            result = wrap;
        }

        return result;
    } else if (stat->is<IR::MethodCallStatement>()) {
        auto mce = stat->to<IR::MethodCallStatement>()->methodCall;
        auto minst = P4::MethodInstance::resolve(mce, ctxt->refMap, ctxt->typeMap);
        if (minst->is<P4::ExternMethod>()) {
            auto extmeth = minst->to<P4::ExternMethod>();
            if (extmeth->method->name.name == corelib.packetIn.extract.name) {
                int argCount = mce->arguments->size();
                if (argCount < 1 || argCount > 2) {
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "%1%: unknown extract method", mce);
                    return result;
                }

                cstring ename = argCount == 1 ? "extract" : "extract_VL";
                result->emplace("op", ename);
                auto arg = mce->arguments->at(0);
                auto argtype = ctxt->typeMap->getType(arg->expression, true);
                if (!argtype->is<IR::Type_Header>()) {
                    ::error(ErrorType::ERR_INVALID,
                            "%1%: extract only accepts arguments with header types, not %2%",
                            arg, argtype);
                    return result;
                }
                auto param = new Util::JsonObject();
                params->append(param);
                cstring type;
                Util::IJson* j = nullptr;

                if (auto mem = arg->expression->to<IR::Member>()) {
                    auto baseType = ctxt->typeMap->getType(mem->expr, true);
                    if (baseType->is<IR::Type_Stack>()) {
                        if (mem->member == IR::Type_Stack::next) {
                            // stack.next
                            type = "stack";
                            j = ctxt->conv->convert(mem->expr);
                        } else {
                            BUG("%1%: unsupported", mem);
                        }
                    } else if (baseType->is<IR::Type_HeaderUnion>()) {
                        auto parent = mem->expr->to<IR::Member>();
                        if (parent != nullptr) {
                            auto parentType = ctxt->typeMap->getType(parent->expr, true);
                            if (parentType->is<IR::Type_Stack>()) {
                                // stack.next.unionfield
                                if (parent->member == IR::Type_Stack::next) {
                                    type = "union_stack";
                                    j = ctxt->conv->convert(parent->expr);
                                    Util::JsonArray *a;
                                    if (j->is<Util::JsonArray>()) {
                                        a = j->to<Util::JsonArray>()->clone();
                                    } else if (j->is<Util::JsonObject>()) {
                                        a = new Util::JsonArray();
                                        a->push_back(j->to<Util::JsonObject>()->get("value"));
                                    } else {
                                        BUG("unexpected");
                                    }
                                    a->append(mem->member.name);
                                    auto j0 = new Util::JsonObject();
                                    j = j0->emplace("value", a);
                                } else {
                                    BUG("%1%: unsupported", mem);
                                }
                            }
                        }
                    }
                }
                if (j == nullptr) {
                    type = "regular";
                    j = ctxt->conv->convert(arg->expression);
                }
                auto value = j->to<Util::JsonObject>()->get("value");
                param->emplace("type", type);
                param->emplace("value", value);

                if (argCount == 2) {
                    auto arg2 = mce->arguments->at(1);
                    // The spec says that this must always be wrapped in an expression.
                    // However, calling convert with the third argument set to 'true'
                    // does not do that.
                    auto jexpr = ctxt->conv->convert(arg2->expression, true, false);
                    auto rwrap = new Util::JsonObject();
                    rwrap->emplace("type", "expression");
                    rwrap->emplace("value", jexpr);
                    params->append(rwrap);
                }
                return result;
            } else if (extmeth->method->name.name == corelib.packetIn.lookahead.name) {
                // bare lookahead call -- should flag an error if there's not enough
                // packet data, but ignore for now.
                return nullptr;
            } else if (extmeth->method->name.name == corelib.packetIn.advance.name) {
                if (mce->arguments->size() != 1) {
                    ::error(ErrorType::ERR_UNSUPPORTED, "%1%: expected 1 argument", mce);
                    return result;
                }
                auto arg = mce->arguments->at(0);
                auto jexpr = ctxt->conv->convert(arg->expression, true, false);
                result->emplace("op", "advance");
                params->append(jexpr);
                return result;
            }
        } else if (minst->is<P4::ExternFunction>()) {
            auto extfn = minst->to<P4::ExternFunction>();
            auto extFuncName = extfn->method->name.name;
            if (extFuncName == IR::ParserState::verify) {
                result->emplace("op", "verify");
                BUG_CHECK(mce->arguments->size() == 2, "%1%: Expected 2 arguments", mce);
                {
                    auto cond = mce->arguments->at(0);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = ctxt->conv->convert(cond->expression, true, false);
                    params->append(jexpr);
                }
                {
                    auto error = mce->arguments->at(1);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = ctxt->conv->convert(error->expression, true, false);
                    params->append(jexpr);
                }
                return result;
            } else if (extFuncName == "assert"
                       || extFuncName == "assume") {
                BUG_CHECK(mce->arguments->size() == 1, "%1%: Expected 1 argument ", mce);
                result->emplace("op", "primitive");
                auto paramValue = new Util::JsonObject();
                params->append(paramValue);
                auto paramsArray = mkArrayField(paramValue, "parameters");
                auto cond = mce->arguments->at(0);
                auto expr = ctxt->conv->convert(cond->expression, true, true, true);
                paramsArray->append(expr);
                paramValue->emplace("op", extFuncName);
                paramValue->emplace_non_null("source_info", mce->sourceInfoJsonObj());
                return result;
            }
        } else if (minst->is<P4::BuiltInMethod>()) {
            /* example result:
             {
                "parameters" : [
                {
                  "op" : "add_header",
                  "parameters" : [{"type" : "header", "value" : "ipv4"}]
                }
              ],
              "op" : "primitive"
            } */
            result->emplace("op", "primitive");

            auto bi = minst->to<P4::BuiltInMethod>();
            cstring primitive;
            auto paramsValue = new Util::JsonObject();
            params->append(paramsValue);

            auto pp = mkArrayField(paramsValue, "parameters");
            auto obj = ctxt->conv->convert(bi->appliedTo);
            pp->append(obj);

            if (bi->name == IR::Type_Header::setValid) {
                primitive = "add_header";
            } else if (bi->name == IR::Type_Header::setInvalid) {
                primitive = "remove_header";
            } else if (bi->name == IR::Type_Stack::push_front ||
                       bi->name == IR::Type_Stack::pop_front) {
                if (bi->name == IR::Type_Stack::push_front)
                    primitive = "push";
                else
                    primitive = "pop";

                BUG_CHECK(mce->arguments->size() == 1, "Expected 1 argument for %1%", mce);
                auto arg = ctxt->conv->convert(mce->arguments->at(0)->expression);
                pp->append(arg);
            } else {
                BUG("%1%: Unexpected built-in method", bi->name);
            }

            paramsValue->emplace("op", primitive);
            return result;
        }
    }
    ::error(ErrorType::ERR_UNSUPPORTED, "in parser on this target", stat);
    return result;
}

// Operates on a select keyset
void ParserConverter::convertSimpleKey(const IR::Expression* keySet,
                                       mpz_class& value, mpz_class& mask) const {
    if (keySet->is<IR::Mask>()) {
        auto mk = keySet->to<IR::Mask>();
        if (!mk->left->is<IR::Constant>()) {
            ::error(ErrorType::ERR_INVALID, "must evaluate to a compile-time constant", mk->left);
            return;
        }
        if (!mk->right->is<IR::Constant>()) {
            ::error(ErrorType::ERR_INVALID, "must evaluate to a compile-time constant", mk->right);
            return;
        }
        value = mk->left->to<IR::Constant>()->value;
        mask = mk->right->to<IR::Constant>()->value;
    } else if (keySet->is<IR::Constant>()) {
        value = keySet->to<IR::Constant>()->value;
        mask = -1;
    } else if (keySet->is<IR::BoolLiteral>()) {
        value = keySet->to<IR::BoolLiteral>()->value ? 1 : 0;
        mask = -1;
    } else if (keySet->is<IR::DefaultExpression>()) {
        value = 0;
        mask = 0;
    } else {
        ::error(ErrorType::ERR_INVALID, "must evaluate to a compile-time constant", keySet);
        value = 0;
        mask = 0;
    }
}

unsigned ParserConverter::combine(const IR::Expression* keySet,
                                const IR::ListExpression* select,
                                mpz_class& value, mpz_class& mask,
                                bool& is_vset, cstring& vset_name) const {
    // From the BMv2 spec: For values and masks, make sure that you
    // use the correct format. They need to be the concatenation (in
    // the right order) of all byte padded fields (padded with 0
    // bits). For example, if the transition key consists of a 12-bit
    // field and a 2-bit field, each value will need to have 3 bytes
    // (2 for the first field, 1 for the second one). If the
    // transition value is 0xaba, 0x3, the value attribute will be set
    // to 0x0aba03.
    // Return width in bytes
    value = 0;
    mask = 0;
    is_vset = false;
    unsigned totalWidth = 0;
    if (keySet->is<IR::DefaultExpression>()) {
        return totalWidth;
    } else if (keySet->is<IR::ListExpression>()) {
        auto le = keySet->to<IR::ListExpression>();
        BUG_CHECK(le->components.size() == select->components.size(),
                  "%1%: mismatched select", select);
        unsigned index = 0;

        bool noMask = true;
        for (auto it = select->components.begin();
             it != select->components.end(); ++it) {
            auto e = *it;
            auto keyElement = le->components.at(index);

            auto type = ctxt->typeMap->getType(e, true);
            int width = type->width_bits();
            BUG_CHECK(width > 0, "%1%: unknown width", e);

            mpz_class key_value, mask_value;
            convertSimpleKey(keyElement, key_value, mask_value);
            unsigned w = 8 * ROUNDUP(width, 8);
            totalWidth += ROUNDUP(width, 8);
            value = Util::shift_left(value, w) + key_value;
            if (mask_value != -1) {
                noMask = false;
            } else {
                // mask_value == -1 is a special value used to
                // indicate an exact match on all bit positions.  When
                // there is more than one keyElement, we must
                // represent such an exact match with 'width' 1 bits,
                // because it may be combined into a mask for other
                // keyElements that have their own independent masks.
                mask_value = Util::mask(width);
            }
            mask = Util::shift_left(mask, w) + mask_value;
            LOG3("Shifting " << " into key " << key_value << " &&& " << mask_value <<
                             " result is " << value << " &&& " << mask);
            index++;
        }

        if (noMask)
            mask = -1;
        return totalWidth;
    } else if (keySet->is<IR::PathExpression>()) {
        auto pe = keySet->to<IR::PathExpression>();
        auto decl = ctxt->refMap->getDeclaration(pe->path, true);
        vset_name = decl->controlPlaneName();
        is_vset = true;
        return totalWidth;
    } else {
        BUG_CHECK(select->components.size() == 1, "%1%: mismatched select/label", select);
        convertSimpleKey(keySet, value, mask);
        auto type = ctxt->typeMap->getType(select->components.at(0), true);
        return ROUNDUP(type->width_bits(), 8);
    }
}

Util::IJson* ParserConverter::stateName(IR::ID state) {
    if (state.name == IR::ParserState::accept) {
        return Util::JsonValue::null;
    } else if (state.name == IR::ParserState::reject) {
        ::warning(ErrorType::WARN_UNSUPPORTED,
                  "Explicit transition to %1% not supported on this target",
                  state);
        return Util::JsonValue::null;
    } else {
        return new Util::JsonValue(state.name);
    }
}

std::vector<Util::IJson*>
ParserConverter::convertSelectExpression(const IR::SelectExpression* expr) {
    std::vector<Util::IJson*> result;
    auto se = expr->to<IR::SelectExpression>();
    for (auto sc : se->selectCases) {
        auto trans = new Util::JsonObject();
        mpz_class value, mask;
        bool is_vset;
        cstring vset_name;
        unsigned bytes = combine(sc->keyset, se->select, value, mask, is_vset, vset_name);
        if (is_vset) {
            trans->emplace("type", "parse_vset");
            trans->emplace("value", vset_name);
            trans->emplace("mask", mask);
            trans->emplace("next_state", stateName(sc->state->path->name));
        } else {
            if (mask == 0) {
                trans->emplace("value", "default");
                trans->emplace("mask", Util::JsonValue::null);
                trans->emplace("next_state", stateName(sc->state->path->name));
            } else {
                trans->emplace("type", "hexstr");
                trans->emplace("value", stringRepr(value, bytes));
                if (mask == -1)
                    trans->emplace("mask", Util::JsonValue::null);
                else
                    trans->emplace("mask", stringRepr(mask, bytes));
                trans->emplace("next_state", stateName(sc->state->path->name));
            }
        }
        result.push_back(trans);
    }
    return result;
}

Util::IJson*
ParserConverter::convertSelectKey(const IR::SelectExpression* expr) {
    auto se = expr->to<IR::SelectExpression>();
    CHECK_NULL(se);
    auto key = ctxt->conv->convert(se->select, false);
    return key;
}

Util::IJson*
ParserConverter::convertPathExpression(const IR::PathExpression* pe) {
    auto trans = new Util::JsonObject();
    trans->emplace("value", "default");
    trans->emplace("mask", Util::JsonValue::null);
    trans->emplace("next_state", stateName(pe->path->name));
    return trans;
}

Util::IJson*
ParserConverter::createDefaultTransition() {
    auto trans = new Util::JsonObject();
    trans->emplace("value", "default");
    trans->emplace("mask", Util::JsonValue::null);
    trans->emplace("next_state", Util::JsonValue::null);
    return trans;
}

bool ParserConverter::preorder(const IR::P4Parser* parser) {
    auto parser_id = ctxt->json->add_parser(name);

    for (auto s : parser->parserLocals) {
        if (auto inst = s->to<IR::P4ValueSet>()) {
            auto bitwidth = inst->elementType->width_bits();
            auto name = inst->controlPlaneName();
            ctxt->json->add_parse_vset(name, bitwidth);
        }
    }

    // convert parse state
    for (auto state : parser->states) {
        if (state->name == IR::ParserState::reject || state->name == IR::ParserState::accept)
            continue;
        // For the state we use the internal name, not the control-plane name
        auto state_id = ctxt->json->add_parser_state(parser_id, state->name);
        // convert statements
        for (auto s : state->components) {
            auto op = convertParserStatement(s);
            if (op)
                ctxt->json->add_parser_op(state_id, op);
        }
        // convert transitions
        if (state->selectExpression != nullptr) {
            if (state->selectExpression->is<IR::SelectExpression>()) {
                auto expr = state->selectExpression->to<IR::SelectExpression>();
                auto transitions = convertSelectExpression(expr);
                for (auto transition : transitions) {
                    ctxt->json->add_parser_transition(state_id, transition);
                }
                auto key = convertSelectKey(expr);
                ctxt->json->add_parser_transition_key(state_id, key);
            } else if (state->selectExpression->is<IR::PathExpression>()) {
                auto expr = state->selectExpression->to<IR::PathExpression>();
                auto transition = convertPathExpression(expr);
                ctxt->json->add_parser_transition(state_id, transition);
            } else {
                BUG("%1%: unexpected selectExpression", state->selectExpression);
            }
        } else {
            auto transition = createDefaultTransition();
            ctxt->json->add_parser_transition(state_id, transition);
        }
    }
    return false;
}

}  // namespace BMV2
