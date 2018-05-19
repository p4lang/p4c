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

#include "frontends/common/model.h"
#include "portableSwitch.h"

namespace P4 {

const IR::P4Program* PsaProgramStructure::create(const IR::P4Program* program) {
    createTypes();
    createHeaders();
    createExterns();
    createParsers();
    createActions();
    createControls();
    createDeparsers();
    return program;
}

Util::IJson* PsaProgramStructure::convertParserStatement(const IR::StatOrDecl* stat) {
    auto result = new Util::JsonObject();
    cstring scalarsName = refMap->newName("scalars");
    auto conv = new BMV2::PsaExpressionConverter(refMap,typeMap,scalarsName,&scalarMetadataFields);
    auto params = BMV2::mkArrayField(result, "parameters");
    if (stat->is<IR::AssignmentStatement>()) {
        auto assign = stat->to<IR::AssignmentStatement>();
        auto type = typeMap->getType(assign->left, true);
        cstring operation = BMV2::Backend::jsonAssignment(type, true);
        result->emplace("op", operation);
        auto l = conv->convertLeftValue(assign->left);
        bool convertBool = type->is<IR::Type_Boolean>();
        auto r = conv->convert(assign->right, true, true, convertBool);
        params->append(l);
        params->append(r);

        if (operation != "set") {
            // must wrap into another outer object
            auto wrap = new Util::JsonObject();
            wrap->emplace("op", "primitive");
            auto params = BMV2::mkParameters(wrap);
            params->append(result);
            result = wrap;
        }

        return result;
    } else if (stat->is<IR::MethodCallStatement>()) {
        auto mce = stat->to<IR::MethodCallStatement>()->methodCall;
        auto minst = P4::MethodInstance::resolve(mce, refMap, typeMap);
        if (minst->is<P4::ExternMethod>()) {
            auto extmeth = minst->to<P4::ExternMethod>();
            if (extmeth->method->name.name == corelib.packetIn.extract.name) {
                int argCount = mce->arguments->size();
                if (argCount == 1 || argCount == 2) {
                    cstring ename = argCount == 1 ? "extract" : "extract_VL";
                    result->emplace("op", ename);
                    auto arg = mce->arguments->at(0);
                    auto argtype = typeMap->getType(arg->expression, true);
                    if (!argtype->is<IR::Type_Header>()) {
                        ::error("%1%: extract only accepts arguments with header types, not %2%",
                                arg, argtype);
                        return result;
                    }
                    auto param = new Util::JsonObject();
                    params->append(param);
                    cstring type;
                    Util::IJson* j = nullptr;

                    if (arg->expression->is<IR::Member>()) {
                        auto mem = arg->expression->to<IR::Member>();
                        auto baseType = typeMap->getType(mem->expr, true);
                        if (baseType->is<IR::Type_Stack>()) {
                            if (mem->member == IR::Type_Stack::next) {
                                type = "stack";
                                j = conv->convert(mem->expr);
                            } else {
                                BUG("%1%: unsupported", mem);
                            }
                        }
                    }
                    if (j == nullptr) {
                        type = "regular";
                        j = conv->convert(arg->expression);
                    }
                    auto value = j->to<Util::JsonObject>()->get("value");
                    param->emplace("type", type);
                    param->emplace("value", value);

                    if (argCount == 2) {
                        auto arg2 = mce->arguments->at(1);
                        auto jexpr = conv->convert(arg2->expression, true, false);
                        auto rwrap = new Util::JsonObject();
                        // The spec says that this must always be wrapped in an expression
                        rwrap->emplace("type", "expression");
                        rwrap->emplace("value", jexpr);
                        params->append(rwrap);
                    }
                    return result;
                }
            }
        } else if (minst->is<P4::ExternFunction>()) {
            auto extfn = minst->to<P4::ExternFunction>();
            if (extfn->method->name.name == IR::ParserState::verify) {
                result->emplace("op", "verify");
                BUG_CHECK(mce->arguments->size() == 2, "%1%: Expected 2 arguments", mce);
                {
                    auto cond = mce->arguments->at(0);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = conv->convert(cond->expression, true, false);
                    params->append(jexpr);
                }
                {
                    auto error = mce->arguments->at(1);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = conv->convert(error->expression, true, false);
                    params->append(jexpr);
                }
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

            auto pp = BMV2::mkArrayField(paramsValue, "parameters");
            auto obj = conv->convert(bi->appliedTo);
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
                auto arg = conv->convert(mce->arguments->at(0)->expression);
                pp->append(arg);
            } else {
                BUG("%1%: Unexpected built-in method", bi->name);
            }

            paramsValue->emplace("op", primitive);
            return result;
        }
    }
    ::error("%1%: not supported in parser on this target", stat);
    return result;
}


void PsaProgramStructure::convertSimpleKey(const IR::Expression* keySet,
                                       mpz_class& value, mpz_class& mask) const {
    if (keySet->is<IR::Mask>()) {
        auto mk = keySet->to<IR::Mask>();
        if (!mk->left->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->left);
            return;
        }
        if (!mk->right->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->right);
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
        ::error("%1% must evaluate to a compile-time constant", keySet);
        value = 0;
        mask = 0;
    }
}

unsigned PsaProgramStructure::combine(const IR::Expression* keySet,
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

            auto type = typeMap->getType(e, true);
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
        auto decl = refMap->getDeclaration(pe->path, true);
        vset_name = decl->controlPlaneName();
        is_vset = true;
        return totalWidth;
    } else {
        BUG_CHECK(select->components.size() == 1, "%1%: mismatched select/label", select);
        convertSimpleKey(keySet, value, mask);
        auto type = typeMap->getType(select->components.at(0), true);
        return ROUNDUP(type->width_bits(), 8);
    }
}

Util::IJson* PsaProgramStructure::stateName(IR::ID state) {
    if (state.name == IR::ParserState::accept) {
        return Util::JsonValue::null;
    } else if (state.name == IR::ParserState::reject) {
        ::warning("Explicit transition to %1% not supported on this target", state);
        return Util::JsonValue::null;
    } else {
        return new Util::JsonValue(state.name);
    }
}

std::vector<Util::IJson*>
PsaProgramStructure::convertSelectExpression(const IR::SelectExpression* expr) {
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
                trans->emplace("value", BMV2::stringRepr(value, bytes));
                if (mask == -1)
                    trans->emplace("mask", Util::JsonValue::null);
                else
                    trans->emplace("mask", BMV2::stringRepr(mask, bytes));
                trans->emplace("next_state", stateName(sc->state->path->name));
            }
        }
        result.push_back(trans);
    }
    return result;
}

Util::IJson* PsaProgramStructure::convertSelectKey(const IR::SelectExpression* expr) {
    auto se = expr->to<IR::SelectExpression>();
    CHECK_NULL(se);
    auto key = conv->convert(se->select, false);
    return key;
}

Util::IJson*
PsaProgramStructure::convertPathExpression(const IR::PathExpression* pe) {
    auto trans = new Util::JsonObject();
    trans->emplace("value", "default");
    trans->emplace("mask", Util::JsonValue::null);
    trans->emplace("next_state", stateName(pe->path->name));
    return trans;
}

Util::IJson*
PsaProgramStructure::createDefaultTransition() {
    auto trans = new Util::JsonObject();
    trans->emplace("value", "default");
    trans->emplace("mask", Util::JsonValue::null);
    trans->emplace("next_state", Util::JsonValue::null);
    return trans;
}

void PsaProgramStructure::convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body,
                                          Util::JsonArray* result) {
    conv->simpleExpressionsOnly = true;
    for (auto s : *body) {
        if (auto block = s->to<IR::BlockStatement>()) {
            convertDeparserBody(&block->components, result);
            continue;
        } else if (s->is<IR::ReturnStatement>() || s->is<IR::ExitStatement>()) {
            break;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::MethodCallStatement>()) {
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc,
                    refMap, typeMap);
            if (mi->is<P4::ExternMethod>()) {
                auto em = mi->to<P4::ExternMethod>();
                if (em->originalExternType->name.name == getCoreLibrary().packetOut.name) {
                    if (em->method->name.name == getCoreLibrary().packetOut.emit.name) {
                        BUG_CHECK(mc->arguments->size() == 1,
                                  "Expected exactly 1 argument for %1%", mc);
                        auto arg = mc->arguments->at(0);
                        auto type = typeMap->getType(arg, true);
                        if (type->is<IR::Type_Stack>()) {
                            // This branch is in fact never taken, because
                            // arrays are expanded into elements.
                            int size = type->to<IR::Type_Stack>()->getSize();
                            for (int i=0; i < size; i++) {
                                auto j = conv->convert(arg->expression);
                                auto e = j->to<Util::JsonObject>()->get("value");
                                BUG_CHECK(e->is<Util::JsonValue>(),
                                          "%1%: Expected a Json value", e->toString());
                                cstring ref = e->to<Util::JsonValue>()->getString();
                                ref += "[" + Util::toString(i) + "]";
                                result->append(ref);
                            }
                        } else if (type->is<IR::Type_Header>()) {
                            auto j = conv->convert(arg->expression);
                            auto val = j->to<Util::JsonObject>()->get("value");
                            result->append(val);
                        } else {
                            ::error("%1%: emit only supports header and stack arguments, not %2%",
                                    arg, type);
                        }
                    }
                    continue;
                }
            }
        }
        ::error("%1%: not supported with a deparser on this target", s);
    }
    conv->simpleExpressionsOnly = false;
}

Util::IJson* PsaProgramStructure::convertDeparser(const cstring& name, const IR::P4Control* ctrl) {
    auto result = new Util::JsonObject();
    result->emplace("name", name);
    result->emplace("id", BMV2::nextId("deparser"));
    result->emplace_non_null("source_info", ctrl->sourceInfoJsonObj());
    auto order = BMV2::mkArrayField(result, "order");
    convertDeparserBody(&ctrl->body->components, order);
    return result;
}

void PsaProgramStructure::convertTableEntries(const IR::P4Table *table,
                                           Util::JsonObject *jsonTable) {
    auto entriesList = table->getEntries();
    if (entriesList == nullptr) return;

    auto entries = BMV2::mkArrayField(jsonTable, "entries");
    int entryPriority = 1;  // default priority is defined by index position
    for (auto e : entriesList->entries) {
        // TODO(jafingerhut) - add line/col here?
        auto entry = new Util::JsonObject();

        auto keyset = e->getKeys();
        auto matchKeys = BMV2::mkArrayField(entry, "match_key");
        int keyIndex = 0;
        for (auto k : keyset->components) {
            auto key = new Util::JsonObject();
            auto tableKey = table->getKey()->keyElements.at(keyIndex);
            auto keyWidth = tableKey->expression->type->width_bits();
            auto k8 = ROUNDUP(keyWidth, 8);
            auto matchType = getKeyMatchType(tableKey);
            key->emplace("match_type", matchType);
            if (matchType == getCoreLibrary().exactMatch.name) {
                if (k->is<IR::Constant>())
                    key->emplace("key", BMV2::stringRepr(k->to<IR::Constant>()->value, k8));
                else if (k->is<IR::BoolLiteral>())
                    // booleans are converted to ints
                    key->emplace("key", BMV2::stringRepr(k->to<IR::BoolLiteral>()->value ? 1 : 0, k8));
                else
                    ::error("%1% unsupported exact key expression", k);
            } else if (matchType == getCoreLibrary().ternaryMatch.name) {
                if (k->is<IR::Mask>()) {
                    auto km = k->to<IR::Mask>();
                    key->emplace("key", BMV2::stringRepr(km->left->to<IR::Constant>()->value, k8));
                    key->emplace("mask", BMV2::stringRepr(km->right->to<IR::Constant>()->value, k8));
                } else if (k->is<IR::Constant>()) {
                    key->emplace("key", BMV2::stringRepr(k->to<IR::Constant>()->value, k8));
                    key->emplace("mask", BMV2::stringRepr(Util::mask(keyWidth), k8));
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("key", BMV2::stringRepr(0, k8));
                    key->emplace("mask", BMV2::stringRepr(0, k8));
                } else {
                    ::error("%1% unsupported ternary key expression", k);
                }
            } else if (matchType == getCoreLibrary().lpmMatch.name) {
                if (k->is<IR::Mask>()) {
                    auto km = k->to<IR::Mask>();
                    key->emplace("key", BMV2::stringRepr(km->left->to<IR::Constant>()->value, k8));
                    auto trailing_zeros = [](unsigned long n) { return n ? __builtin_ctzl(n) : 0; };
                    auto count_ones = [](unsigned long n) { return n ? __builtin_popcountl(n) : 0;};
                    unsigned long mask = km->right->to<IR::Constant>()->value.get_ui();
                    auto len = trailing_zeros(mask);
                    if (len + count_ones(mask) != keyWidth)  // any remaining 0s in the prefix?
                        ::error("%1% invalid mask for LPM key", k);
                    else
                        key->emplace("prefix_length", keyWidth - len);
                } else if (k->is<IR::Constant>()) {
                    key->emplace("key", BMV2::stringRepr(k->to<IR::Constant>()->value, k8));
                    key->emplace("prefix_length", keyWidth);
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("key", BMV2::stringRepr(0, k8));
                    key->emplace("prefix_length", 0);
                } else {
                    ::error("%1% unsupported LPM key expression", k);
                }
            } else if (matchType == "range") {
                if (k->is<IR::Range>()) {
                    auto kr = k->to<IR::Range>();
                    key->emplace("start", BMV2::stringRepr(kr->left->to<IR::Constant>()->value, k8));
                    key->emplace("end", BMV2::stringRepr(kr->right->to<IR::Constant>()->value, k8));
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("start", BMV2::stringRepr(0, k8));
                    key->emplace("end", BMV2::stringRepr((1 << keyWidth)-1, k8));  // 2^N -1
                } else {
                    ::error("%1% invalid range key expression", k);
                }
            } else {
                ::error("unkown key match type '%1%' for key %2%", matchType, k);
            }
            matchKeys->append(key);
            keyIndex++;
        }

        auto action = new Util::JsonObject();
        auto actionRef = e->getAction();
        if (!actionRef->is<IR::MethodCallExpression>())
            ::error("%1%: invalid action in entries list", actionRef);
        auto actionCall = actionRef->to<IR::MethodCallExpression>();
        auto method = actionCall->method->to<IR::PathExpression>()->path;
        auto decl = refMap->getDeclaration(method, true);
        auto actionDecl = decl->to<IR::P4Action>();
        unsigned id = get(getStructure().ids, actionDecl);
        action->emplace("action_id", id);
        auto actionData = BMV2::mkArrayField(action, "action_data");
        for (auto arg : *actionCall->arguments) {
            actionData->append(BMV2::stringRepr(arg->expression->to<IR::Constant>()->value, 0));
        }
        entry->emplace("action_entry", action);

        auto priorityAnnotation = e->getAnnotation("priority");
        if (priorityAnnotation != nullptr) {
            if (priorityAnnotation->expr.size() > 1)
                ::error("invalid priority value %1%", priorityAnnotation->expr);
            auto priValue = priorityAnnotation->expr.front();
            if (!priValue->is<IR::Constant>())
                ::error("invalid priority value %1%. must be constant", priorityAnnotation->expr);
            entry->emplace("priority", priValue->to<IR::Constant>()->value);
        } else {
            entry->emplace("priority", entryPriority);
        }
        entryPriority += 1;

        entries->append(entry);
    }
}

cstring PsaProgramStructure::getKeyMatchType(const IR::KeyElement *ke) {
    auto path = ke->matchType->path;
    auto mt = refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
    BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);

    if (mt->name.name == getCoreLibrary().exactMatch.name ||
        mt->name.name == getCoreLibrary().ternaryMatch.name ||
        mt->name.name == getCoreLibrary().lpmMatch.name ||
        match_kinds.count(mt->name.name)) {
        return mt->name.name;
    }

    ::error("%1%: match type not supported on this target", mt);
    return "invalid";
}

bool
PsaProgramStructure::handleTableImplementation(const IR::Property* implementation,
                                            const IR::Key* key,
                                            Util::JsonObject* table,
                                            Util::JsonArray* action_profiles,
                                            BMV2::SharedActionSelectorCheck& selector_check) {
    if (implementation == nullptr) {
        table->emplace("type", "simple");
        return true;
    }

    if (!implementation->value->is<IR::ExpressionValue>()) {
        ::error("%1%: expected expression for property", implementation);
        return false;
    }
    auto propv = implementation->value->to<IR::ExpressionValue>();

    bool isSimpleTable = true;
    Util::JsonObject* action_profile;
    cstring apname;

    if (propv->expression->is<IR::ConstructorCallExpression>()) {
        auto cc = P4::ConstructorCall::resolve(
            propv->expression->to<IR::ConstructorCallExpression>(),
            refMap, typeMap);
        if (!cc->is<P4::ExternConstructorCall>()) {
            ::error("%1%: expected extern object for property", implementation);
            return false;
        }
        auto ecc = cc->to<P4::ExternConstructorCall>();
        auto implementationType = ecc->type;
        auto arguments = ecc->cce->arguments;
        apname = implementation->controlPlaneName(refMap->newName("action_profile"));
        action_profile = new Util::JsonObject();
        action_profiles->append(action_profile);
        action_profile->emplace("name", apname);
        action_profile->emplace("id", BMV2::nextId("action_profiles"));
        // TODO(jafingerhut) - add line/col here?
        // TBD what about the else if cases below?

        auto add_size = [&action_profile, &arguments](size_t arg_index) {
            auto size_expr = arguments->at(arg_index)->expression;
            int size;
            if (!size_expr->is<IR::Constant>()) {
                ::error("%1% must be a constant", size_expr);
                size = 0;
            } else {
                size = size_expr->to<IR::Constant>()->asInt();
            }
            action_profile->emplace("max_size", size);
        };
        if (implementationType->name == BMV2::TableImplementation::actionSelectorName) {
            BUG_CHECK(arguments->size() == 3, "%1%: expected 3 arguments", arguments);
            isSimpleTable = false;
            auto selector = new Util::JsonObject();
            table->emplace("type", "indirect_ws");
            action_profile->emplace("selector", selector);
            add_size(1);
            auto hash = arguments->at(0)->expression;
            auto ei = P4::EnumInstance::resolve(hash, typeMap);
            if (ei == nullptr) {
                ::error("%1%: must be a constant on this target", hash);
            } else {
                cstring algo = ei->name;
                selector->emplace("algo", algo);
            }
            auto input = BMV2::mkArrayField(selector, "input");
            for (auto ke : key->keyElements) {
                auto mt = refMap->getDeclaration(ke->matchType->path, true)
                        ->to<IR::Declaration_ID>();
                BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
                if (mt->name.name != BMV2::MatchImplementation::selectorMatchTypeName)
                    continue;

                auto expr = ke->expression;
                auto jk = conv->convert(expr);
                input->append(jk);
            }
        } else if (implementationType->name == BMV2::TableImplementation::actionProfileName) {
            isSimpleTable = false;
            table->emplace("type", "indirect");
            add_size(0);
        } else {
            ::error("%1%: unexpected value for property", propv);
        }
    } else if (propv->expression->is<IR::PathExpression>()) {
        auto pathe = propv->expression->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pathe->path, true);
        if (!decl->is<IR::Declaration_Instance>()) {
            ::error("%1%: expected a reference to an instance", pathe);
            return false;
        }
        apname = decl->controlPlaneName();
        auto dcltype = typeMap->getType(pathe, true);
        if (!dcltype->is<IR::Type_Extern>()) {
            ::error("%1%: unexpected type for implementation", dcltype);
            return false;
        }
        auto type_extern_name = dcltype->to<IR::Type_Extern>()->name;
        if (type_extern_name == BMV2::TableImplementation::actionProfileName) {
            table->emplace("type", "indirect");
        } else if (type_extern_name == BMV2::TableImplementation::actionSelectorName) {
            table->emplace("type", "indirect_ws");
        } else {
            ::error("%1%: unexpected type for implementation", dcltype);
            return false;
        }
        isSimpleTable = false;
    } else {
        ::error("%1%: unexpected value for property", propv);
        return false;
    }
    table->emplace("action_profile", apname);
    return isSimpleTable;
}

Util::IJson*
PsaProgramStructure::convertTable(const BMV2::CFG::TableNode* node,
                               Util::JsonArray* action_profiles,
                               BMV2::SharedActionSelectorCheck& selector_check) {
    auto table = node->table;
    LOG3("Processing " << dbp(table));
    auto result = new Util::JsonObject();
    cstring name = table->controlPlaneName();
    result->emplace("name", name);
    result->emplace("id", BMV2::nextId("tables"));
    result->emplace_non_null("source_info", table->sourceInfoJsonObj());
    cstring table_match_type = getCoreLibrary().exactMatch.name;
    auto key = table->getKey();
    auto tkey = BMV2::mkArrayField(result, "key");
    conv->simpleExpressionsOnly = true;

    if (key != nullptr) {
        for (auto ke : key->keyElements) {
            auto expr = ke->expression;
            auto ket = typeMap->getType(expr, true);
            if (!ket->is<IR::Type_Bits>() && !ket->is<IR::Type_Boolean>())
                ::error("%1%: Unsupported key type %2%", expr, ket);

            auto match_type = getKeyMatchType(ke);
            if (match_type == BMV2::MatchImplementation::selectorMatchTypeName)
                continue;
            // Decreasing order of precedence (bmv2 specification):
            // 0) more than one LPM field is an error
            // 1) if there is at least one RANGE field, then the table is RANGE
            // 2) if there is at least one TERNARY field, then the table is TERNARY
            // 3) if there is a LPM field, then the table is LPM
            // 4) otherwise the table is EXACT
            if (match_type != table_match_type) {
                if (match_type == BMV2::MatchImplementation::rangeMatchTypeName)
                    table_match_type = BMV2::MatchImplementation::rangeMatchTypeName;
                if (match_type == getCoreLibrary().ternaryMatch.name &&
                    table_match_type != BMV2::MatchImplementation::rangeMatchTypeName)
                    table_match_type = getCoreLibrary().ternaryMatch.name;
                if (match_type == getCoreLibrary().lpmMatch.name &&
                    table_match_type == getCoreLibrary().exactMatch.name)
                    table_match_type = getCoreLibrary().lpmMatch.name;
            } else if (match_type == getCoreLibrary().lpmMatch.name) {
                ::error("%1%, Multiple LPM keys in table", table);
            }

            mpz_class mask;
            if (auto mexp = expr->to<IR::BAnd>()) {
                if (mexp->right->is<IR::Constant>()) {
                    mask = mexp->right->to<IR::Constant>()->value;
                    expr = mexp->left;
                } else if (mexp->left->is<IR::Constant>()) {
                    mask = mexp->left->to<IR::Constant>()->value;
                    expr = mexp->right;
                } else {
                    ::error("%1%: key mask must be a constant", expr); }
            } else if (auto slice = expr->to<IR::Slice>()) {
                expr = slice->e0;
                int h = slice->getH();
                int l = slice->getL();
                mask = Util::maskFromSlice(h, l);
            }

            auto keyelement = new Util::JsonObject();
            keyelement->emplace("match_type", match_type);
            if (auto na = ke->getAnnotation(IR::Annotation::nameAnnotation)) {
                BUG_CHECK(na->expr.size() == 1, "%1%: expected 1 name", na);
                auto name = na->expr[0]->to<IR::StringLiteral>();
                BUG_CHECK(name != nullptr, "%1%: expected a string", na);
                // This is a BMv2 JSON extension: specify a
                // control-plane name for this key
                keyelement->emplace("name", name->value);
            }

            auto jk = conv->convert(expr);
            keyelement->emplace("target", jk->to<Util::JsonObject>()->get("value"));
            if (mask != 0)
                keyelement->emplace("mask", BMV2::stringRepr(mask, ROUNDUP(expr->type->width_bits(), 8)));
            else
                keyelement->emplace("mask", Util::JsonValue::null);
            tkey->append(keyelement);
        }
    }
    result->emplace("match_type", table_match_type);
    conv->simpleExpressionsOnly = false;

    auto impl = table->properties->getProperty(BMV2::TableAttributes::implementationName);
    bool simple = handleTableImplementation(impl, key, result, action_profiles, selector_check);

    unsigned size = 0;
    auto sz = table->properties->getProperty(BMV2::TableAttributes::sizeName);
    if (sz != nullptr) {
        if (sz->value->is<IR::ExpressionValue>()) {
            auto expr = sz->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::Constant>()) {
                ::error("%1% must be a constant", sz);
                size = 0;
            } else {
                size = expr->to<IR::Constant>()->asInt();
            }
        } else {
            ::error("%1%: expected a number", sz);
        }
    }
    if (size == 0)
        size = BMV2::TableAttributes::defaultTableSize;

    result->emplace("max_size", size);
    auto ctrs = table->properties->getProperty(BMV2::TableAttributes::countersName);
    if (ctrs != nullptr) {
        // The counters attribute should list the counters of the table, accessed in
        // actions of the table.  We should be checking that this attribute and the
        // actions are consistent?
        if (ctrs->value->is<IR::ExpressionValue>()) {
            auto expr = ctrs->value->to<IR::ExpressionValue>()->expression;
            if (expr->is<IR::ConstructorCallExpression>()) {
                auto type = typeMap->getType(expr, true);
                if (type == nullptr)
                    return result;
                if (!type->is<IR::Type_Extern>()) {
                    ::error("%1%: Unexpected type %2% for property", ctrs, type);
                    return result;
                }
                auto te = type->to<IR::Type_Extern>();
                if (te->name != BMV2::TableImplementation::directCounterName &&
                    te->name != BMV2::TableImplementation::counterName) {
                    ::error("%1%: Unexpected type %2% for property", ctrs, type);
                    return result;
                }
                auto jctr = new Util::JsonObject();
                cstring ctrname = ctrs->controlPlaneName("counter");
                jctr->emplace("name", ctrname);
                jctr->emplace("id", BMV2::nextId("counter_arrays"));
                // TODO(jafingerhut) - what kind of P4_16 code causes this
                // code to run, if any?
                // TODO(jafingerhut):
                // jctr->emplace_non_null("source_info", ctrs->sourceInfoJsonObj());
                bool direct = te->name == BMV2::TableImplementation::directCounterName;
                jctr->emplace("is_direct", direct);
                jctr->emplace("binding", table->controlPlaneName());
                counters->append(jctr);
            } else if (expr->is<IR::PathExpression>()) {
                auto pe = expr->to<IR::PathExpression>();
                auto decl = refMap->getDeclaration(pe->path, true);
                if (!decl->is<IR::Declaration_Instance>()) {
                    ::error("%1%: expected an instance", decl->getNode());
                    return result;
                }
                cstring ctrname = decl->controlPlaneName();
                auto it = getDirectCounterMap().find(ctrname);
                LOG3("Looking up " << ctrname);
                if (it != getDirectCounterMap().end()) {
                   ::error("%1%: Direct counters cannot be attached to multiple tables %2% and %3%",
                           decl, it->second, table);
                   return result;
                }
                getDirectCounterMap().emplace(ctrname, table);
            } else {
                ::error("%1%: expected a counter", ctrs);
            }
        }
        result->emplace("with_counters", true);
    } else {
        result->emplace("with_counters", false);
    }

    bool sup_to = false;
    auto timeout = table->properties->getProperty(BMV2::TableAttributes::supportTimeoutName);
    if (timeout != nullptr) {
        if (timeout->value->is<IR::ExpressionValue>()) {
            auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::BoolLiteral>()) {
                ::error("%1% must be true/false", timeout);
            } else {
                sup_to = expr->to<IR::BoolLiteral>()->value;
            }
        } else {
            ::error("%1%: expected a Boolean", timeout);
        }
    }
    result->emplace("support_timeout", sup_to);

    auto dm = table->properties->getProperty(BMV2::TableAttributes::metersName);
    if (dm != nullptr) {
        if (dm->value->is<IR::ExpressionValue>()) {
            auto expr = dm->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::PathExpression>()) {
                ::error("%1%: expected a reference to a meter declaration", expr);
            } else {
                auto pe = expr->to<IR::PathExpression>();
                auto decl = refMap->getDeclaration(pe->path, true);
                auto type = typeMap->getType(expr, true);
                if (type == nullptr)
                    return result;
                if (type->is<IR::Type_SpecializedCanonical>())
                    type = type->to<IR::Type_SpecializedCanonical>()->baseType;
                if (!type->is<IR::Type_Extern>()) {
                    ::error("%1%: Unexpected type %2% for property", dm, type);
                    return result;
                }
                auto te = type->to<IR::Type_Extern>();
                if (te->name != BMV2::TableImplementation::directMeterName) {
                    ::error("%1%: Unexpected type %2% for property", dm, type);
                    return result;
                }
                if (!decl->is<IR::Declaration_Instance>()) {
                    ::error("%1%: expected an instance", decl->getNode());
                    return result;
                }
                getMeterMap().setTable(decl, table);
                getMeterMap().setSize(decl, size);
                BUG_CHECK(decl->is<IR::Declaration_Instance>(),
                          "%1%: expected an instance", decl->getNode());
                cstring name = decl->controlPlaneName();
                result->emplace("direct_meters", name);
            }
        } else {
            ::error("%1%: expected a meter", dm);
        }
    } else {
        result->emplace("direct_meters", Util::JsonValue::null);
    }

    auto action_ids = BMV2::mkArrayField(result, "action_ids");
    auto actions = BMV2::mkArrayField(result, "actions");
    auto al = table->getActionList();

    std::map<cstring, cstring> useActionName;
    for (auto a : al->actionList) {
        if (a->expression->is<IR::MethodCallExpression>()) {
            auto mce = a->expression->to<IR::MethodCallExpression>();
            if (mce->arguments->size() > 0)
                ::error("%1%: Actions in action list with arguments not supported", a);
        }
        auto decl = refMap->getDeclaration(a->getPath(), true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1%: should be an action name", a);
        auto action = decl->to<IR::P4Action>();
        unsigned id = get(getStructure().ids, action);
        LOG3("look up id " << action << " " << id);
        action_ids->append(id);
        auto name = action->controlPlaneName();
        actions->append(name);
        useActionName.emplace(action->name, name);
    }

    auto next_tables = new Util::JsonObject();

    BMV2::CFG::Node* nextDestination = nullptr;  // if no action is executed
    BMV2::CFG::Node* defaultLabelDestination = nullptr;  // if the "default" label is executed
    // Note: the "default" label is not the default_action.
    bool hitMiss = false;
    for (auto s : node->successors.edges) {
        if (s->isUnconditional())
            nextDestination = s->endpoint;
        else if (s->isBool())
            hitMiss = true;
        else if (s->label == "default")
            defaultLabelDestination = s->endpoint;
    }

    Util::IJson* nextLabel = nullptr;
    if (!hitMiss) {
        BUG_CHECK(nextDestination, "Could not find default destination for %1%", node->invocation);
        nextLabel = nodeName(nextDestination);
        result->emplace("base_default_next", nextLabel);
        // So if a "default:" switch case exists we set the nextLabel
        // to be the destination of the default: label.
        if (defaultLabelDestination != nullptr)
            nextLabel = nodeName(defaultLabelDestination);
    } else {
        result->emplace("base_default_next", Util::JsonValue::null);
    }

    std::set<cstring> labelsDone;
    for (auto s : node->successors.edges) {
        cstring label;
        if (s->isBool()) {
            label = s->getBool() ? "__HIT__" : "__MISS__";
        } else if (s->isUnconditional()) {
            continue;
        } else {
            label = s->label;
            if (label == "default")
                continue;
            label = ::get(useActionName, label);
        }
        next_tables->emplace(label, nodeName(s->endpoint));
        labelsDone.emplace(label);
    }

    // Generate labels which don't show up and send them to
    // the nextLabel.
    if (!hitMiss) {
        for (auto a : al->actionList) {
            cstring name = a->getName().name;
            cstring label = ::get(useActionName, name);
            if (labelsDone.find(label) == labelsDone.end())
                next_tables->emplace(label, nextLabel);
        }
    }

    result->emplace("next_tables", next_tables);
    auto defact = table->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
    if (defact != nullptr) {
        if (!simple) {
            ::warning("Target does not support default_action for %1% (due to action profiles)",
                      table);
            return result;
        }

        if (!defact->value->is<IR::ExpressionValue>()) {
            ::error("%1%: expected an action", defact);
            return result;
        }
        auto expr = defact->value->to<IR::ExpressionValue>()->expression;
        const IR::P4Action* action = nullptr;
        const IR::Vector<IR::Argument>* args = nullptr;

        if (expr->is<IR::PathExpression>()) {
            auto path = expr->to<IR::PathExpression>()->path;
            auto decl = refMap->getDeclaration(path, true);
            BUG_CHECK(decl->is<IR::P4Action>(), "%1%: should be an action name", expr);
            action = decl->to<IR::P4Action>();
        } else if (expr->is<IR::MethodCallExpression>()) {
            auto mce = expr->to<IR::MethodCallExpression>();
            auto mi = P4::MethodInstance::resolve(mce,
                    refMap, typeMap);
            BUG_CHECK(mi->is<P4::ActionCall>(), "%1%: expected an action", expr);
            action = mi->to<P4::ActionCall>()->action;
            args = mce->arguments;
        } else {
            BUG("%1%: unexpected expression", expr);
        }

        unsigned actionid = get(getStructure().ids, action);
        auto entry = new Util::JsonObject();
        entry->emplace("action_id", actionid);
        entry->emplace("action_const", defact->isConstant);
        auto fields = BMV2::mkArrayField(entry, "action_data");
        if (args != nullptr) {
            // TODO: use argument names
            for (auto a : *args) {
                if (a->expression->is<IR::Constant>()) {
                    cstring repr = BMV2::stringRepr(a->expression->to<IR::Constant>()->value);
                    fields->append(repr);
                } else {
                    ::error("%1%: argument must evaluate to a constant integer", a);
                    return result;
                }
            }
        }
        entry->emplace("action_entry_const", defact->isConstant);
        result->emplace("default_entry", entry);
    }
    convertTableEntries(table, result);
    return result;
}

Util::IJson* PsaProgramStructure::convertIf(const BMV2::CFG::IfNode* node, cstring prefix) {
    (void) prefix;
    auto result = new Util::JsonObject();
    result->emplace("name", node->name);
    result->emplace("id", BMV2::nextId("conditionals"));
    result->emplace_non_null("source_info", node->statement->condition->sourceInfoJsonObj());
    auto j = conv->convert(node->statement->condition, true, false);
    CHECK_NULL(j);
    result->emplace("expression", j);
    for (auto e : node->successors.edges) {
        Util::IJson* dest = nodeName(e->endpoint);
        cstring label = Util::toString(e->getBool());
        label += "_next";
        result->emplace(label, dest);
    }
    return result;
}

void PsaProgramStructure::createStructLike(const IR::Type_StructLike* st) {
    CHECK_NULL(st);
    cstring name = st->controlPlaneName();
    unsigned max_length = 0;  // for variable-sized headers
    bool varbitFound = false;
    auto fields = new Util::JsonArray();
    for (auto f : st->fields) {
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(f->name.name);
            field->append(1);
            field->append(0);
            max_length += 1;
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
            max_length += type->size;
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            field->append(f->name.name);
            max_length += type->size;
            field->append("*");
            if (varbitFound)
                ::error("%1%: headers with multiple varbit fields not supported", st);
            varbitFound = true;
        } else if (auto type = ftype->to<IR::Type_Error>()) {
            field->append(f->name.name);
            field->append(error_width);
            field->append(0);
            max_length += error_width;
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
        fields->append(field);
    }
    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = new Util::JsonArray();
        field->append(name);
        field->append(8 - padding);
        field->append(false);
        fields->append(field);
    }

    unsigned max_length_bytes = (max_length + padding) / 8;
    if (!varbitFound) {
        // ignore
        max_length = 0;
        max_length_bytes = 0;
    }
    json->add_header_type(name, fields, max_length_bytes);
}

void PsaProgramStructure::createTypes() {
    for (auto kv : header_types)
        createStructLike(kv.second);
    for (auto kv : metadata_types)
        createStructLike(kv.second);
    for (auto kv : header_union_types) {
        auto st = kv.second;
        auto fields = new Util::JsonArray();
        for (auto f : st->fields) {
            auto field = new Util::JsonArray();
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            field->append(f->name.name);
            field->append(ht->name.name);
        }
        json->add_union_type(st->name, fields);
    }
    // add errors to json
    // add enums to json
}

void PsaProgramStructure::createHeaders() {
    for (auto kv : headers) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : metadata) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : header_stacks) {

        // json->add_header_stack(stack_type, stack_name, stack_size, ids);
    }
    for (auto kv : header_unions) {
        auto header_name = kv.first;
        auto header_type = kv.second->to<IR::Type_StructLike>()->controlPlaneName();
        // We have to add separately a header instance for all
        // headers in the union.  Each instance will be named with
        // a prefix including the union name, e.g., "u.h"
        Util::JsonArray* fields = new Util::JsonArray();
        for (auto uf : kv.second->to<IR::Type_HeaderUnion>()->fields) {
            auto uft = typeMap->getType(uf, true);
            auto h_name = header_name + "." + uf->controlPlaneName();
            auto h_type = uft->to<IR::Type_StructLike>()->controlPlaneName();
            unsigned id = json->add_header(h_type, h_name);
            fields->append(id);
        }
        json->add_union(header_type, fields, header_name);
    }
}

void PsaProgramStructure::createParsers() {
    // add parsers to json
    for (auto kv : parsers) {
        LOG1("parser" << kv.first << kv.second);
        auto parser_id = json->add_parser(kv.first);
        for (auto s : kv.second->parserLocals) {
            if (auto inst = s->to<IR::P4ValueSet>()) {
                auto bitwidth = inst->elementType->width_bits();
                auto name = inst->controlPlaneName();
                json->add_parse_vset(name, bitwidth);
            }
        }

        // convert parse state
        for (auto state : kv.second->states) {
            if (state->name == IR::ParserState::reject || state->name == IR::ParserState::accept)
                continue;
            auto state_id = json->add_parser_state(parser_id, state->controlPlaneName());
            // convert statements
            for (auto s : state->components) {
                auto op = convertParserStatement(s);
                json->add_parser_op(state_id, op);
            }

            // convert transitions
            if (state->selectExpression != nullptr) {
                if (state->selectExpression->is<IR::SelectExpression>()) {
                    auto expr = state->selectExpression->to<IR::SelectExpression>();
                    auto transitions = convertSelectExpression(expr);
                    for (auto transition : transitions) {
                        json->add_parser_transition(state_id, transition);
                    }
                    auto key = convertSelectKey(expr);
                    json->add_parser_transition_key(state_id, key);
                } else if (state->selectExpression->is<IR::PathExpression>()) {
                    auto expr = state->selectExpression->to<IR::PathExpression>();
                    auto transition = convertPathExpression(expr);
                    json->add_parser_transition(state_id, transition);
                } else {
                    BUG("%1%: unexpected selectExpression", state->selectExpression);
                }
            } else {
                auto transition = createDefaultTransition();
                json->add_parser_transition(state_id, transition);
            }

        }

    }

}


void PsaProgramStructure::createExterns() {
    // add parse_vsets to json
    // add meter_arrays to json
    // add counter_arrays to json
    // add register_arrays to json
    // add checksums to json
    // add learn_list to json
    // add calculations to json
    // add extern_instances to json
}

void PsaProgramStructure::createActions() {
    // add actions to json
}



void PsaProgramStructure::createControls() {
    // add pipelines to json

    for (auto kv : pipelines) {
        LOG1("pipelines " << kv.first << kv.second);
        auto result = new Util::JsonObject();
        result->emplace("name", kv.first);
        result->emplace("id", BMV2::nextId("control"));
        result->emplace_non_null("source_info", kv.second->sourceInfoJsonObj());

        auto cfg = new BMV2::CFG();
        cfg->build(kv.second, refMap, typeMap);
        if (cfg->entryPoint->successors.size() == 0) {
            result->emplace("init_table", Util::JsonValue::null);
        }
        else {
            BUG_CHECK(cfg->entryPoint->successors.size() == 1, "Expected 1 start node for %1%", kv.second);
            auto start = (*(cfg->entryPoint->successors.edges.begin()))->endpoint;
            result->emplace("init_table", nodeName(start));
        }

        auto tables = BMV2::mkArrayField(result, "tables");
        auto action_profiles = BMV2::mkArrayField(result, "action_profiles");
        auto conditionals = BMV2::mkArrayField(result, "conditionals");

        BMV2::SharedActionSelectorCheck selector_check(refMap, typeMap);
        kv.second->apply(selector_check);

        std::set<const IR::P4Table*> done;

    // Tables are created prior to the other local declarations

        for (auto node : cfg->allNodes) {
            auto tn = node->to<BMV2::CFG::TableNode>();
            if (tn != nullptr) {
                if (done.find(tn->table) != done.end())
                // The same table may appear in multiple nodes in the CFG.
                // We emit it only once.  Other checks should ensure that
                // the CFG is implementable.
                    continue;
                done.emplace(tn->table);
                auto j = convertTable(tn, action_profiles, selector_check);
                if (::errorCount() > 0)
                    return;
                tables->append(j);
            } else if (node->is<BMV2::CFG::IfNode>()) {
                auto j = convertIf(node->to<BMV2::CFG::IfNode>(), kv.first);
                if (::errorCount() > 0)
                    return;
                conditionals->append(j);
            }
        }



        for (auto c : kv.second->controlLocals) {

            if (c->is<IR::Declaration_Instance>()) {

                auto block = psa_resourceMap.at(kv.second);
            }
        }

        json->pipelines->append(result);

    }

}
void PsaProgramStructure::createDeparsers() {
    // add deparsers to json

    for (auto kv : deparsers) {
        LOG1("deparser" << kv.first << kv.second);
        auto deparserJson = convertDeparser(kv.first, kv.second);
        json->deparsers->append(deparserJson);
}

}
bool ParsePsaArchitecture::preorder(const IR::ToplevelBlock* block) {
    return false;
}

void ParsePsaArchitecture::parse_pipeline(const IR::PackageBlock* block, gress_t gress) {
    if (gress == INGRESS) {
        auto parser = block->getParameterValue("ip")->to<IR::ParserBlock>();
        auto pipeline = block->getParameterValue("ig")->to<IR::ControlBlock>();
        auto deparser = block->getParameterValue("id")->to<IR::ControlBlock>();
        structure->block_type.emplace(parser->container, std::make_pair(gress, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(gress, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(gress, DEPARSER));
    } else if (gress == EGRESS) {
        auto parser = block->getParameterValue("ep")->to<IR::ParserBlock>();
        auto pipeline = block->getParameterValue("eg")->to<IR::ControlBlock>();
        auto deparser = block->getParameterValue("ed")->to<IR::ControlBlock>();
        structure->block_type.emplace(parser->container, std::make_pair(gress, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(gress, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(gress, DEPARSER));
    }
}

bool ParsePsaArchitecture::preorder(const IR::PackageBlock* block) {
    auto pkg = block->getParameterValue("ingress");
    if (auto ingress = pkg->to<IR::PackageBlock>()) {
        parse_pipeline(ingress, INGRESS);
    }
    pkg = block->getParameterValue("egress");
    if (auto egress = pkg->to<IR::PackageBlock>()) {
        parse_pipeline(egress, EGRESS);
    }
    return false;
}

void InspectPsaProgram::postorder(const IR::P4Parser* p) {
    // populate structure->parsers
}

void InspectPsaProgram::postorder(const IR::P4Control* cont) {




}

void InspectPsaProgram::postorder(const IR::Type_Header* h) {
    // inspect IR::Type_Header
    // populate structure->header_types;
}

void InspectPsaProgram::postorder(const IR::Type_HeaderUnion* hu) {
    // inspect IR::Type_HeaderUnion
    // populate structure->header_union_types;
}

void InspectPsaProgram::postorder(const IR::Declaration_Variable* var) {
    // inspect IR::Declaration_Variable
    // populate structure->headers or
    //          structure->header_stacks or
    //          structure->header_unions
    // based on the type of the variable
}

void InspectPsaProgram::postorder(const IR::Declaration_Instance* di) {
    // inspect IR::Declaration_Instance,
    // populate structure->meter_arrays or
    //          structure->counter_arrays or
    //          structure->register_arrays or
    //          structure->extern_instances or
    //          structure->checksums
    // based on the type of the instance
    cstring name = di->controlPlaneName();
    LOG1("di is " << name);




}


void InspectPsaProgram::postorder(const IR::P4Action* act) {
    // inspect IR::P4Action,
    // populate structure->actions
}

void InspectPsaProgram::postorder(const IR::Type_Error* err) {
    // inspect IR::Type_Error
    // populate structure->errors.
}

bool InspectPsaProgram::isHeaders(const IR::Type_StructLike* st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

void InspectPsaProgram::addHeaderType(const IR::Type_StructLike *st) {
    if (st->is<IR::Type_HeaderUnion>()) {
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
        }
        pinfo->header_union_types.emplace(st->getName(), st->to<IR::Type_HeaderUnion>());
        return;
    } else if (st->is<IR::Type_Header>()) {
        pinfo->header_types.emplace(st->getName(), st->to<IR::Type_Header>());
    } else if (st->is<IR::Type_Struct>()) {
        pinfo->metadata_types.emplace(st->getName(), st->to<IR::Type_Struct>());
    }
}

void InspectPsaProgram::addHeaderInstance(const IR::Type_StructLike *st, cstring name) {
    auto inst = new IR::Declaration_Variable(name, st);
    if (st->is<IR::Type_Header>())
        pinfo->headers.emplace(name, inst);
    else if (st->is<IR::Type_Struct>())
        pinfo->metadata.emplace(name, inst);
    else if (st->is<IR::Type_HeaderUnion>())
        pinfo->header_unions.emplace(name, inst);
}

void InspectPsaProgram::addTypesAndInstances(const IR::Type_StructLike* type, bool isHeader) {
    addHeaderType(type);
    addHeaderInstance(type, type->controlPlaneName());
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (isHeader && ft->is<IR::Type_Struct>()) {
                ::error("Type %1% should only contain headers, header stacks, or header unions",
                        type);
                return;
            }
            if (auto hft = ft->to<IR::Type_Header>()) {
                addHeaderType(hft);
                addHeaderInstance(hft, hft->controlPlaneName());
            } else if (ft->is<IR::Type_HeaderUnion>()) {
                for (auto uf : ft->to<IR::Type_HeaderUnion>()->fields) {
                    auto uft = typeMap->getType(uf, true);
                    if (auto h_type = uft->to<IR::Type_Header>()) {
                        addHeaderType(h_type);
                        addHeaderInstance(h_type, h_type->controlPlaneName());
                    } else {
                        ::error("Type %1% cannot contain type %2%", ft, uft);
                        return;
                    }
                }
                pinfo->header_union_types.emplace(type->getName(), type->to<IR::Type_HeaderUnion>());
                addHeaderInstance(type, type->controlPlaneName());
            } else {
                LOG1("add struct type " << type);
                pinfo->metadata_types.emplace(type->getName(), type->to<IR::Type_Struct>());
                addHeaderInstance(type, type->controlPlaneName());
            }
        } else if (ft->is<IR::Type_Stack>()) {
            auto stack = ft->to<IR::Type_Stack>();
            auto stack_name = f->controlPlaneName();
            auto stack_size = stack->getSize();
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            auto stack_type = stack->elementType->to<IR::Type_Header>();
            std::vector<unsigned> ids;
            for (unsigned i = 0; i < stack_size; i++) {
                cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
                // FIXME:
                // auto id = json->add_header(stack_type, hdrName);
                addHeaderInstance(stack_type, hdrName);
                // ids.push_back(id);
            }
            // addHeaderStackInstance();
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            if (ft->is<IR::Type_Bits>()) {
                auto tb = ft->to<IR::Type_Bits>();
                pinfo->scalars_width += tb->size;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                pinfo->scalars_width += 1;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                pinfo->scalars_width += 32;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

bool InspectPsaProgram::preorder(const IR::Parameter* param) {
    auto ft = typeMap->getType(param->getNode(), true);
    // only convert parameters that are IR::Type_StructLike
    if (!ft->is<IR::Type_StructLike>())
        return false;
    auto st = ft->to<IR::Type_StructLike>();
    // parameter must be a type that we have not seen before
    if (pinfo->hasVisited(st))
        return false;
    LOG1("type struct " << ft);
    auto isHeader = isHeaders(st);
    addTypesAndInstances(st, isHeader);
    return false;
}

bool InspectPsaProgram::preorder(const IR::P4Parser* p) {
    if (pinfo->block_type.count(p)) {
        auto info = pinfo->block_type.at(p);
        if (info.first == INGRESS && info.second == PARSER)
            pinfo->parsers.emplace("ingress", p);
        else if (info.first == EGRESS && info.second == PARSER)
            pinfo->parsers.emplace("egress", p);
    }
    return false;
}

bool InspectPsaProgram::preorder(const IR::P4Control *c) {
    if (pinfo->block_type.count(c)) {
        auto info = pinfo->block_type.at(c);
        if (info.first == INGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("egress", c);
        else if (info.first == INGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("egress", c);
    }
    return false;
}

bool InspectPsaProgram::preorder(const IR::Declaration_MatchKind* kind) {
    for (auto member : kind->members) {
        pinfo->match_kinds.insert(member->name);
    }
    return false;
}


}  // namespace P4
