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
#include "control.h"

namespace BMV2 {

/**
    Converts a list of P4 defined table entries to JSON.

    Assumes that all the checking on the entries being valid has been
    done in the validateParsedProgram pass, so this is simply the serialization.

    This function still checks and reports errors in key entry expressions.
    The JSON generated after a reported error will not be accepted in BMv2.

    @param table A P4 table IR node
    @param jsonTable the JSON table object to add predefined entries
*/
void Control::convertTableEntries(const IR::P4Table *table,
                                  Util::JsonObject *jsonTable) {
    auto entriesList = table->getEntries();
    if (entriesList == nullptr) return;

    auto entries = mkArrayField(jsonTable, "entries");
    int entryPriority = 1;  // default priority is defined by index position
    for (auto e : entriesList->entries) {
        auto entry = new Util::JsonObject();

        auto keyset = e->getKeys();
        auto matchKeys = mkArrayField(entry, "match_key");
        int keyIndex = 0;
        for (auto k : keyset->components) {
            auto key = new Util::JsonObject();
            auto tableKey = table->getKey()->keyElements.at(keyIndex);
            auto keyWidth = tableKey->expression->type->width_bits();
            auto k8 = ROUNDUP(keyWidth, 8);
            auto matchType = getKeyMatchType(tableKey);
            key->emplace("match_type", matchType);
            if (matchType == "valid") {
                if (k->is<IR::BoolLiteral>())
                    key->emplace("key", k->to<IR::BoolLiteral>()->toString());
                else
                    ::error("%1% invalid 'valid' key expression", k);
            } else if (matchType == backend->getCoreLibrary().exactMatch.name) {
                if (k->is<IR::Constant>())
                    key->emplace("key", stringRepr(k->to<IR::Constant>()->value, k8));
                else
                    ::error("%1% invalid exact key expression", k);
            } else if (matchType == backend->getCoreLibrary().ternaryMatch.name) {
                if (k->is<IR::Mask>()) {
                    auto km = k->to<IR::Mask>();
                    key->emplace("key", stringRepr(km->left->to<IR::Constant>()->value, k8));
                    key->emplace("mask", stringRepr(km->right->to<IR::Constant>()->value, k8));
                } else if (k->is<IR::Constant>()) {
                    key->emplace("key", stringRepr(k->to<IR::Constant>()->value, k8));
                    key->emplace("mask", stringRepr(Util::mask(keyWidth), k8));
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("key", stringRepr(0, k8));
                    key->emplace("mask", stringRepr(0, k8));
                } else {
                    ::error("%1% invalid ternary key expression", k);
                }
            } else if (matchType == backend->getCoreLibrary().lpmMatch.name) {
                if (k->is<IR::Mask>()) {
                    auto km = k->to<IR::Mask>();
                    key->emplace("key", stringRepr(km->left->to<IR::Constant>()->value, k8));
                    auto trailing_zeros = [](unsigned long n) { return n ? __builtin_ctzl(n) : 0; };
                    auto count_ones = [](unsigned long n) { return n ? __builtin_popcountl(n) : 0;};
                    unsigned long mask = km->right->to<IR::Constant>()->value.get_ui();
                    auto len = trailing_zeros(mask);
                    if (len + count_ones(mask) != keyWidth)  // any remaining 0s in the prefix?
                        ::error("%1% invalid mask for LPM key", k);
                    else
                        key->emplace("prefix_length", keyWidth - len);
                } else if (k->is<IR::Constant>()) {
                    key->emplace("key", stringRepr(k->to<IR::Constant>()->value, k8));
                    key->emplace("prefix_length", keyWidth);
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("key", stringRepr(0, k8));
                    key->emplace("prefix_length", 0);
                } else {
                    ::error("%1% invalid LPM key expression", k);
                }
            } else if (matchType == "range") {
                if (k->is<IR::Range>()) {
                    auto kr = k->to<IR::Range>();
                    key->emplace("start", stringRepr(kr->left->to<IR::Constant>()->value, k8));
                    key->emplace("end", stringRepr(kr->right->to<IR::Constant>()->value, k8));
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("start", stringRepr(0, k8));
                    key->emplace("end", stringRepr((1 << keyWidth)-1, k8));  // 2^N -1
                } else {
                    ::error("%1% invalid range key expression", k);
                }
            } else {
                ::error("unkown key type '%1%' for key %2%", matchType, k);
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
        auto decl = backend->getRefMap().getDeclaration(method, true);
        auto actionDecl = decl->to<IR::P4Action>();
        unsigned id = get(backend->getStructure().ids, actionDecl);
        action->emplace("action_id", id);
        auto actionData = mkArrayField(action, "action_data");
        for (auto arg : *actionCall->arguments) {
            actionData->append(stringRepr(arg->to<IR::Constant>()->value, 0));
        }
        entry->emplace("action_entry", action);

        auto priorityAnnotation = e->getAnnotation("priority");
        if (priorityAnnotation != nullptr) {
            if (priorityAnnotation->expr.size() > 1)
                ::error("invalid priority value %1%", priorityAnnotation->expr);
            auto priValue = priorityAnnotation->expr.front();
            if ( !priValue->is<IR::Constant>())
                ::error("invalid priority value %1%. must be constant", priorityAnnotation->expr);
            entry->emplace("priority", priValue->to<IR::Constant>()->value);
        } else {
            entry->emplace("priority", entryPriority);
        }
        entryPriority += 1;

        entries->append(entry);
    }
}


/**
    Computes the type of the key based on the declaration in the path.

    If the key expression invokes .isValid(), we return "valid".
    @param ke A table match key element
    @return the key type
*/
cstring Control::getKeyMatchType(const IR::KeyElement *ke) {
    auto mt = backend->getRefMap().getDeclaration(ke->matchType->path, true)->to<IR::Declaration_ID>();
    BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
    auto expr = ke->expression;

    cstring match_type = "invalid";
    if (mt->name.name == backend->getCoreLibrary().exactMatch.name ||
        mt->name.name == backend->getCoreLibrary().ternaryMatch.name) {
        match_type = mt->name.name;
        if (expr->is<IR::MethodCallExpression>()) {
            auto mi = P4::MethodInstance::resolve(expr->to<IR::MethodCallExpression>(),
                                                  &backend->getRefMap(), &backend->getTypeMap());
            if (mi->is<P4::BuiltInMethod>())
                if (mi->to<P4::BuiltInMethod>()->name == IR::Type_Header::isValid)
                    match_type = "valid";
        }
    } else if (mt->name.name == backend->getCoreLibrary().lpmMatch.name) {
        match_type = mt->name.name;
    } else if (backend->getModel().find_match_kind(mt->name.name)) {
        match_type = mt->name.name;
    } else {
        ::error("%1%: match type not supported on this target", mt);
    }
    return match_type;
}

bool
Control::handleTableImplementation(const IR::Property* implementation,
                                                    const IR::Key* key,
                                                    Util::JsonObject* table,
                                                    Util::JsonArray* action_profiles) {
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
            &backend->getRefMap(), &backend->getTypeMap());
        if (!cc->is<P4::ExternConstructorCall>()) {
            ::error("%1%: expected extern object for property", implementation);
            return false;
        }
        auto ecc = cc->to<P4::ExternConstructorCall>();
        auto implementationType = ecc->type;
        auto arguments = ecc->cce->arguments;
        apname = implementation->externalName(backend->getRefMap().newName("action_profile"));
        action_profile = new Util::JsonObject();
        action_profiles->append(action_profile);
        action_profile->emplace("name", apname);
        action_profile->emplace("id", nextId("action_profiles"));

        auto add_size = [&action_profile, &arguments](size_t arg_index) {
            auto size_expr = arguments->at(arg_index);
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
            auto hash = arguments->at(0);
            auto ei = P4::EnumInstance::resolve(hash, &backend->getTypeMap());
            if (ei == nullptr) {
                ::error("%1%: must be a constant on this target", hash);
            } else {
                cstring algo = ei->name; //FIXME: translate algorithm name?
                selector->emplace("algo", algo);
            }
            auto input = mkArrayField(selector, "input");
            for (auto ke : key->keyElements) {
                auto mt = backend->getRefMap().getDeclaration(ke->matchType->path, true)
                        ->to<IR::Declaration_ID>();
                BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
                if (mt->name.name != BMV2::MatchImplementation::selectorMatchTypeName)
                    continue;

                auto expr = ke->expression;
                auto jk = backend->getExpressionConverter()->convert(expr);
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
        auto decl = backend->getRefMap().getDeclaration(pathe->path, true);
        if (!decl->is<IR::Declaration_Instance>()) {
            ::error("%1%: expected a reference to an instance", pathe);
            return false;
        }
        apname = extVisibleName(decl);
        auto dcltype = backend->getTypeMap().getType(pathe, true);
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
Control::convertTable(const CFG::TableNode* node,
                      Util::JsonArray* action_profiles) {
    auto table = node->table;
    LOG3("Processing " << dbp(table));
    auto result = new Util::JsonObject();
    cstring name = extVisibleName(table);
    result->emplace("name", name);
    result->emplace("id", nextId("tables"));
    cstring table_match_type = backend->getCoreLibrary().exactMatch.name;
    auto key = table->getKey();
    auto tkey = mkArrayField(result, "key");
    backend->getExpressionConverter()->simpleExpressionsOnly = true;

    if (key != nullptr) {
        for (auto ke : key->keyElements) {
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
                if (match_type == backend->getCoreLibrary().ternaryMatch.name &&
                    table_match_type != BMV2::MatchImplementation::rangeMatchTypeName)
                    table_match_type = backend->getCoreLibrary().ternaryMatch.name;
                if (match_type == backend->getCoreLibrary().lpmMatch.name &&
                    table_match_type == backend->getCoreLibrary().exactMatch.name)
                    table_match_type = backend->getCoreLibrary().lpmMatch.name;
            } else if (match_type == backend->getCoreLibrary().lpmMatch.name) {
                ::error("%1%, Multiple LPM keys in table", table);
            }
            auto expr = ke->expression;
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
            if (match_type == "valid") {
                auto mt = backend->getRefMap().getDeclaration(ke->matchType->path, true)
                          ->to<IR::Declaration_ID>();
                if (expr->is<IR::MethodCallExpression>()) {
                    auto mi = P4::MethodInstance::resolve(expr->to<IR::MethodCallExpression>(),
                                                          &backend->getRefMap(), &backend->getTypeMap());
                    if (mi->is<P4::BuiltInMethod>()) {
                        auto bim = mi->to<P4::BuiltInMethod>();
                        if (bim->name == IR::Type_Header::isValid) {
                            if (mt->name.name == backend->getCoreLibrary().ternaryMatch.name) {
                                expr = new IR::Member(IR::Type::Boolean::get(), bim->appliedTo,
                                                      "$valid$");
                                backend->getTypeMap().setType(expr, expr->type);
                                match_type = backend->getCoreLibrary().ternaryMatch.name;
                                if (match_type != table_match_type &&
                                    table_match_type != BMV2::MatchImplementation::rangeMatchTypeName)
                                    table_match_type = backend->getCoreLibrary().ternaryMatch.name;
                            } else {
                                expr = bim->appliedTo; }}}}
            }

            auto keyelement = new Util::JsonObject();
            keyelement->emplace("match_type", match_type);
            auto jk = backend->getExpressionConverter()->convert(expr);
            keyelement->emplace("target", jk->to<Util::JsonObject>()->get("value"));
            if (mask != 0)
                keyelement->emplace("mask", stringRepr(mask, (expr->type->width_bits() + 7) / 8));
            else
                keyelement->emplace("mask", Util::JsonValue::null);
            tkey->append(keyelement);
        }
    }
    result->emplace("match_type", table_match_type);
    backend->getExpressionConverter()->simpleExpressionsOnly = false;

    auto impl = table->properties->getProperty(BMV2::TableAttributes::implementationName);
    bool simple = handleTableImplementation(impl, key, result, action_profiles);

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
    auto ctrs = table->properties->getProperty(BMV2::TableImplementation::directCounterName);
    if (ctrs != nullptr) {
        if (ctrs->value->is<IR::ExpressionValue>()) {
            auto expr = ctrs->value->to<IR::ExpressionValue>()->expression;
            if (expr->is<IR::ConstructorCallExpression>()) {
                auto type = backend->getTypeMap().getType(expr, true);
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
                result->emplace("with_counters", true);
                auto jctr = new Util::JsonObject();
                cstring ctrname = ctrs->externalName("counter");
                jctr->emplace("name", ctrname);
                jctr->emplace("id", nextId("counter_arrays"));
                bool direct = te->name == BMV2::TableImplementation::directCounterName;
                jctr->emplace("is_direct", direct);
                jctr->emplace("binding", name);
                backend->counters->append(jctr);
            } else if (expr->is<IR::PathExpression>()) {
                auto pe = expr->to<IR::PathExpression>();
                auto decl = backend->getRefMap().getDeclaration(pe->path, true);
                if (!decl->is<IR::Declaration_Instance>()) {
                    ::error("%1%: expected an instance", decl->getNode());
                    return result;
                }
                cstring ctrname = decl->externalName();
                auto it = backend->getDirectCounterMap().find(ctrname);
                LOG3("Looking up " << ctrname);
                if (it != backend->getDirectCounterMap().end()) {
                   ::error("%1%: Direct cannot be attached to multiple tables %2% and %3%",
                           decl, it->second, table);
                   return result;
                }
                backend->getDirectCounterMap().emplace(ctrname, table);
            } else {
                ::error("%1%: expected a counter", ctrs);
            }
        }
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

    auto dm = table->properties->getProperty(BMV2::TableImplementation::directMeterName);
    if (dm != nullptr) {
        if (dm->value->is<IR::ExpressionValue>()) {
            auto expr = dm->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::PathExpression>()) {
                ::error("%1%: expected a reference to a meter declaration", expr);
            } else {
                auto pe = expr->to<IR::PathExpression>();
                auto decl = backend->getRefMap().getDeclaration(pe->path, true);
                auto type = backend->getTypeMap().getType(expr, true);
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
                //FIXME
                //meterMap.setTable(decl, table);
                //meterMap.setSize(decl, size);
                BUG_CHECK(decl->is<IR::Declaration_Instance>(),
                          "%1%: expected an instance", decl->getNode());
                cstring name = extVisibleName(decl);
                result->emplace("direct_meters", name);
            }
        } else {
            ::error("%1%: expected a meter", dm);
        }
    } else {
        result->emplace("direct_meters", Util::JsonValue::null);
    }

    auto action_ids = mkArrayField(result, "action_ids");
    auto actions = mkArrayField(result, "actions");
    auto al = table->getActionList();

    std::map<cstring, cstring> useActionName;
    for (auto a : al->actionList) {
        if (a->expression->is<IR::MethodCallExpression>()) {
            auto mce = a->expression->to<IR::MethodCallExpression>();
            if (mce->arguments->size() > 0)
                ::error("%1%: Actions in action list with arguments not supported", a);
        }
        auto decl = backend->getRefMap().getDeclaration(a->getPath(), true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1%: should be an action name", a);
        auto action = decl->to<IR::P4Action>();
        unsigned id = get(backend->getStructure().ids, action);
        LOG1("look up id " << action << " " << id);
        action_ids->append(id);
        auto name = extVisibleName(action);
        actions->append(name);
        useActionName.emplace(action->name, name);
    }

    auto next_tables = new Util::JsonObject();

    CFG::Node* nextDestination = nullptr;  // if no action is executed
    CFG::Node* defaultLabelDestination = nullptr;  // if the "default" label is executed
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
        const IR::Vector<IR::Expression>* args = nullptr;

        if (expr->is<IR::PathExpression>()) {
            auto decl = backend->getRefMap().getDeclaration(expr->to<IR::PathExpression>()->path, true);
            BUG_CHECK(decl->is<IR::P4Action>(), "%1%: should be an action name", expr);
            action = decl->to<IR::P4Action>();
        } else if (expr->is<IR::MethodCallExpression>()) {
            auto mce = expr->to<IR::MethodCallExpression>();
            auto mi = P4::MethodInstance::resolve(mce, &backend->getRefMap(), &backend->getTypeMap());
            BUG_CHECK(mi->is<P4::ActionCall>(), "%1%: expected an action", expr);
            action = mi->to<P4::ActionCall>()->action;
            args = mce->arguments;
        } else {
            BUG("%1%: unexpected expression", expr);
        }

        unsigned actionid = get(backend->getStructure().ids, action);
        auto entry = new Util::JsonObject();
        entry->emplace("action_id", actionid);
        entry->emplace("action_const", false);
        auto fields = mkArrayField(entry, "action_data");
        if (args != nullptr) {
            for (auto a : *args) {
                if (a->is<IR::Constant>()) {
                    cstring repr = stringRepr(a->to<IR::Constant>()->value);
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

Util::IJson* Control::convertIf(const CFG::IfNode* node, cstring prefix) {
    auto result = new Util::JsonObject();
    result->emplace("name", node->name);
    result->emplace("id", nextId("conditionals"));
    auto j = backend->getExpressionConverter()->convert(node->statement->condition, true, false);
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

/**
    Custom visitor to enable traversal on other blocks
*/
bool Control::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::ControlBlock>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}

bool Control::preorder(const IR::ControlBlock* block) {
    auto bt = backend->blockTypeMap.find(block);
    if (bt != backend->blockTypeMap.end()) {
        LOG3(bt->second->getAnnotation("pipeline"));
        // only generate control block marked with @pipeline
        if(!bt->second->getAnnotation("pipeline")) {
            return false;
        }
    }

    const IR::P4Control* cont = block->container;
    auto result = new Util::JsonObject();
    result->emplace("name", cont->name); //FIXME: check name is correct, externalName()?
    result->emplace("id", nextId("control"));

    auto cfg = new CFG();
    cfg->build(cont, &backend->getRefMap(), &backend->getTypeMap());
    cfg->checkForCycles();

    if (cfg->entryPoint->successors.size() == 0) {
        result->emplace("init_table", Util::JsonValue::null);
    } else {
        BUG_CHECK(cfg->entryPoint->successors.size() == 1, "Expected 1 start node for %1%", cont);
        auto start = (*(cfg->entryPoint->successors.edges.begin()))->endpoint;
        result->emplace("init_table", nodeName(start));
    }

    auto tables = mkArrayField(result, "tables");
    auto action_profiles = mkArrayField(result, "action_profiles");
    auto conditionals = mkArrayField(result, "conditionals");

    // Tables are created prior to the other local declarations
    for (auto node : cfg->allNodes) {
        if (node->is<CFG::TableNode>()) {
            auto j = convertTable(node->to<CFG::TableNode>(), action_profiles);
            if (::errorCount() > 0)
                return false;
            tables->append(j);
        } else if (node->is<CFG::IfNode>()) {
            auto j = convertIf(node->to<CFG::IfNode>(), cont->name);
            if (::errorCount() > 0)
                return false;
            conditionals->append(j);
        }
    }

    for (auto c : cont->controlLocals) {
        visit(c);
    }
    backend->pipelines->append(result);
    return false;
}

/**
    Process extern instance inside a control block

    Control C ( ... ) {
        Register<bit<32>, bit<32>> reg;  <- visit this line
    }
*/
bool Control::preorder(const IR::Declaration_Instance* instance) {
    LOG3("Visiting " << dbp(instance));
    auto parent = getContext()->node;
    auto bl = parent->to<IR::ControlBlock>()->getValue(instance);
    CHECK_NULL(bl);
    cstring name = instance->externalName();
    LOG1("Declaration_Instance name = " << name.c_str());
    if (bl->is<IR::ExternBlock>()) {
        visit(bl->to<IR::ExternBlock>());
    }
    return false;
}

Util::IJson* Control::createExternInstance(cstring name, cstring type) {
    auto j = new Util::JsonObject();
    j->emplace("name", name);
    j->emplace("id", nextId("extern_instances"));
    j->emplace("type", type);
    return j;
}

}  // namespace BMV2
