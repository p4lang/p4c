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

#include "control.h"
#include "extern.h"
#include "sharedActionSelectorCheck.h"

namespace BMV2 {

static constexpr unsigned INVALID_ACTION_ID = 0xffffffff;

void ControlConverter::convertTableEntries(const IR::P4Table *table,
                                           Util::JsonObject *jsonTable) {
    auto entriesList = table->getEntries();
    if (entriesList == nullptr) return;

    auto entries = mkArrayField(jsonTable, "entries");
    int entryPriority = 1;  // default priority is defined by index position
    for (auto e : entriesList->entries) {
        auto entry = new Util::JsonObject();
        entry->emplace_non_null("source_info", e->sourceInfoJsonObj());

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
            if (matchType == corelib.exactMatch.name) {
                if (k->is<IR::Constant>())
                    key->emplace("key", stringRepr(k->to<IR::Constant>()->value, k8));
                else if (k->is<IR::BoolLiteral>())
                    // booleans are converted to ints
                    key->emplace("key", stringRepr(k->to<IR::BoolLiteral>()->value ? 1 : 0, k8));
                else
                    ::error(ErrorType::ERR_UNSUPPORTED, "exact key expression", k);
            } else if (matchType == corelib.ternaryMatch.name) {
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
                    ::error(ErrorType::ERR_UNSUPPORTED, "ternary key expression", k);
                }
            } else if (matchType == corelib.lpmMatch.name) {
                if (k->is<IR::Mask>()) {
                    auto km = k->to<IR::Mask>();
                    key->emplace("key", stringRepr(km->left->to<IR::Constant>()->value, k8));
                    auto trailing_zeros = [](unsigned long n) { return n ? __builtin_ctzl(n) : 0; };
                    auto count_ones = [](unsigned long n) { return n ? __builtin_popcountl(n) : 0;};
                    unsigned long mask = km->right->to<IR::Constant>()->value.get_ui();
                    auto len = trailing_zeros(mask);
                    if (len + count_ones(mask) != keyWidth)  // any remaining 0s in the prefix?
                        ::error(ErrorType::ERR_INVALID, "mask for LPM key", k);
                    else
                        key->emplace("prefix_length", keyWidth - len);
                } else if (k->is<IR::Constant>()) {
                    key->emplace("key", stringRepr(k->to<IR::Constant>()->value, k8));
                    key->emplace("prefix_length", keyWidth);
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("key", stringRepr(0, k8));
                    key->emplace("prefix_length", 0);
                } else {
                    ::error(ErrorType::ERR_UNSUPPORTED, "LPM key expression", k);
                }
            } else if (matchType == "range") {
                if (k->is<IR::Range>()) {
                    auto kr = k->to<IR::Range>();
                    key->emplace("start", stringRepr(kr->left->to<IR::Constant>()->value, k8));
                    key->emplace("end", stringRepr(kr->right->to<IR::Constant>()->value, k8));
                } else if (k->is<IR::Constant>()) {
                    key->emplace("start", stringRepr(k->to<IR::Constant>()->value, k8));
                    key->emplace("end", stringRepr(k->to<IR::Constant>()->value, k8));
                } else if (k->is<IR::DefaultExpression>()) {
                    key->emplace("start", stringRepr(0, k8));
                    key->emplace("end", stringRepr((1 << keyWidth)-1, k8));  // 2^N -1
                } else {
                    ::error(ErrorType::ERR_UNSUPPORTED, "range key expression", k);
                }
            } else {
                ::error(ErrorType::ERR_UNKNOWN, "key match type '%2%' for key %1%", k, matchType);
            }
            matchKeys->append(key);
            keyIndex++;
        }

        auto action = new Util::JsonObject();
        auto actionRef = e->getAction();
        if (!actionRef->is<IR::MethodCallExpression>())
            ::error(ErrorType::ERR_INVALID, "Invalid action %1% in entries list.", actionRef);
        auto actionCall = actionRef->to<IR::MethodCallExpression>();
        auto method = actionCall->method->to<IR::PathExpression>()->path;
        auto decl = ctxt->refMap->getDeclaration(method, true);
        auto actionDecl = decl->to<IR::P4Action>();
        unsigned id = get(ctxt->structure->ids, actionDecl, INVALID_ACTION_ID);
        BUG_CHECK(id != INVALID_ACTION_ID,
                  "Could not find id for %1%", actionDecl);
        action->emplace("action_id", id);
        auto actionData = mkArrayField(action, "action_data");
        for (auto arg : *actionCall->arguments) {
            actionData->append(stringRepr(arg->expression->to<IR::Constant>()->value, 0));
        }
        entry->emplace("action_entry", action);

        auto priorityAnnotation = e->getAnnotation("priority");
        if (priorityAnnotation != nullptr) {
            if (priorityAnnotation->expr.size() > 1)
                ::error(ErrorType::ERR_INVALID, "Invalid priority value %1%",
                        priorityAnnotation->expr);
            auto priValue = priorityAnnotation->expr.front();
            if (!priValue->is<IR::Constant>())
                ::error(ErrorType::ERR_INVALID, "Invalid priority value %1%. Must be constant.",
                        priorityAnnotation->expr);
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

    @param ke A table match key element
    @return the key type
*/
cstring ControlConverter::getKeyMatchType(const IR::KeyElement *ke) {
    auto path = ke->matchType->path;
    auto mt = ctxt->refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
    BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);

    if (mt->name.name == corelib.exactMatch.name ||
        mt->name.name == corelib.ternaryMatch.name ||
        mt->name.name == corelib.lpmMatch.name ||
        ctxt->structure->match_kinds.count(mt->name.name)) {
        return mt->name.name;
    }

    ::error(ErrorType::ERR_UNSUPPORTED, "match type not supported on this target", mt);
    return "invalid";
}

bool
ControlConverter::handleTableImplementation(const IR::Property* implementation,
                                            const IR::Key* key,
                                            Util::JsonObject* table,
                                            Util::JsonArray* action_profiles,
                                            BMV2::SharedActionSelectorCheck&) {
    if (implementation == nullptr) {
        table->emplace("type", "simple");
        return true;
    }

    if (!implementation->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED, "expected expression for property", implementation);
        return false;
    }
    auto propv = implementation->value->to<IR::ExpressionValue>();

    bool isSimpleTable = true;
    Util::JsonObject* action_profile;
    cstring apname;

    if (propv->expression->is<IR::ConstructorCallExpression>()) {
        auto cc = P4::ConstructorCall::resolve(
            propv->expression->to<IR::ConstructorCallExpression>(),
            ctxt->refMap, ctxt->typeMap);
        if (!cc->is<P4::ExternConstructorCall>()) {
            ::error(ErrorType::ERR_EXPECTED, "extern object for property", implementation);
            return false;
        }
        auto ecc = cc->to<P4::ExternConstructorCall>();
        auto implementationType = ecc->type;
        auto arguments = ecc->cce->arguments;
        apname = implementation->controlPlaneName(ctxt->refMap->newName("action_profile"));
        action_profile = new Util::JsonObject();
        action_profiles->append(action_profile);
        action_profile->emplace("name", apname);
        action_profile->emplace("id", nextId("action_profiles"));
        action_profile->emplace_non_null("source_info", propv->expression->sourceInfoJsonObj());
        // TODO(jafingerhut) - add line/col here?
        // TBD what about the else if cases below?

        auto add_size = [&action_profile, &arguments](size_t arg_index) {
            auto size_expr = arguments->at(arg_index)->expression;
            int size;
            if (!size_expr->is<IR::Constant>()) {
                ::error(ErrorType::ERR_EXPECTED, "%1% must be a constant", size_expr);
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
            auto ei = P4::EnumInstance::resolve(hash, ctxt->typeMap);
            if (ei == nullptr) {
                ::error(ErrorType::ERR_EXPECTED, "hash must be a constant on this target", hash);
            } else {
                cstring algo = ei->name;
                selector->emplace("algo", algo);
            }
            auto input = mkArrayField(selector, "input");
            for (auto ke : key->keyElements) {
                auto mt = ctxt->refMap->getDeclaration(ke->matchType->path, true)
                        ->to<IR::Declaration_ID>();
                BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
                if (mt->name.name != BMV2::MatchImplementation::selectorMatchTypeName)
                    continue;

                auto expr = ke->expression;
                auto jk = ctxt->conv->convert(expr);
                input->append(jk);
            }
        } else if (implementationType->name == BMV2::TableImplementation::actionProfileName) {
            isSimpleTable = false;
            table->emplace("type", "indirect");
            add_size(0);
        } else {
            ::error(ErrorType::ERR_UNEXPECTED, "value for property", propv);
        }
    } else if (propv->expression->is<IR::PathExpression>()) {
        auto pathe = propv->expression->to<IR::PathExpression>();
        auto decl = ctxt->refMap->getDeclaration(pathe->path, true);
        if (!decl->is<IR::Declaration_Instance>()) {
            ::error(ErrorType::ERR_EXPECTED, "a reference to an instance", pathe);
            return false;
        }
        apname = decl->controlPlaneName();
        auto dcltype = ctxt->typeMap->getType(pathe, true);
        if (!dcltype->is<IR::Type_Extern>()) {
            ::error(ErrorType::ERR_UNEXPECTED, "type for implementation", dcltype);
            return false;
        }
        auto type_extern_name = dcltype->to<IR::Type_Extern>()->name;
        if (type_extern_name == BMV2::TableImplementation::actionProfileName) {
            table->emplace("type", "indirect");
        } else if (type_extern_name == BMV2::TableImplementation::actionSelectorName) {
            table->emplace("type", "indirect_ws");
        } else {
            ::error(ErrorType::ERR_UNEXPECTED, "type for implementation", dcltype);
            return false;
        }
        isSimpleTable = false;
        if (ctxt->toplevel->hasValue(decl->getNode())) {
            auto eb = ctxt->toplevel->getValue(decl->getNode());
            BUG_CHECK(eb->is<IR::ExternBlock>(), "Not an extern block?");
            ExternConverter::cvtExternInstance(ctxt, decl->to<IR::Declaration>(),
                eb->to<IR::ExternBlock>(), emitExterns);
        }
    } else {
        ::error(ErrorType::ERR_UNEXPECTED, "value for property", propv);
        return false;
    }
    table->emplace("action_profile", apname);
    return isSimpleTable;
}

Util::IJson*
ControlConverter::convertTable(const CFG::TableNode* node,
                               Util::JsonArray* action_profiles,
                               BMV2::SharedActionSelectorCheck& selector_check) {
    auto table = node->table;
    LOG3("Processing " << dbp(table));
    auto result = new Util::JsonObject();
    cstring name = table->controlPlaneName();
    result->emplace("name", name);
    result->emplace("id", nextId("tables"));
    result->emplace_non_null("source_info", table->sourceInfoJsonObj());
    cstring table_match_type = corelib.exactMatch.name;
    auto key = table->getKey();
    auto tkey = mkArrayField(result, "key");
    ctxt->conv->simpleExpressionsOnly = true;

    if (key != nullptr) {
        for (auto ke : key->keyElements) {
            auto expr = ke->expression;
            auto ket = ctxt->typeMap->getType(expr, true);
            if (!ket->is<IR::Type_Bits>() && !ket->is<IR::Type_Boolean>())
                ::error(ErrorType::ERR_UNSUPPORTED, "%1%: unsupporded key type %2%. "
                        "Supported key types are be bit<> or boolean.", expr, ket);

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
                if (match_type == corelib.ternaryMatch.name &&
                    table_match_type != BMV2::MatchImplementation::rangeMatchTypeName)
                    table_match_type = corelib.ternaryMatch.name;
                if (match_type == corelib.lpmMatch.name &&
                    table_match_type == corelib.exactMatch.name)
                    table_match_type = corelib.lpmMatch.name;
            } else if (match_type == corelib.lpmMatch.name) {
                ::error(ErrorType::ERR_UNSUPPORTED, "multiple LPM keys in table %1%", table);
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
                    ::error(ErrorType::ERR_EXPECTED, "must be a constant", expr); }
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

            auto jk = ctxt->conv->convert(expr);
            keyelement->emplace("target", jk->to<Util::JsonObject>()->get("value"));
            if (mask != 0)
                keyelement->emplace("mask", stringRepr(mask, ROUNDUP(expr->type->width_bits(), 8)));
            else
                keyelement->emplace("mask", Util::JsonValue::null);
            tkey->append(keyelement);
        }
    }
    result->emplace("match_type", table_match_type);
    ctxt->conv->simpleExpressionsOnly = false;

    auto impl = table->properties->getProperty("implementation");
    bool simple = handleTableImplementation(impl, key, result, action_profiles, selector_check);

    unsigned size = 0;
    auto sz = table->properties->getProperty("size");
    if (sz != nullptr) {
        if (sz->value->is<IR::ExpressionValue>()) {
            auto expr = sz->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::Constant>()) {
                ::error(ErrorType::ERR_EXPECTED, "must be a constant", sz);
                size = 0;
            } else {
                size = expr->to<IR::Constant>()->asInt();
            }
        } else {
            ::error(ErrorType::ERR_EXPECTED, "a number", sz);
        }
    }
    if (size == 0)
        size = BMV2::TableAttributes::defaultTableSize;

    result->emplace("max_size", size);
    auto ctrs = table->properties->getProperty("counters");
    if (ctrs != nullptr) {
        // The counters attribute should list the counters of the table, accessed in
        // actions of the table.  We should be checking that this attribute and the
        // actions are consistent?
        if (ctrs->value->is<IR::ExpressionValue>()) {
            auto expr = ctrs->value->to<IR::ExpressionValue>()->expression;
            if (expr->is<IR::ConstructorCallExpression>()) {
                auto type = ctxt->typeMap->getType(expr, true);
                if (type == nullptr)
                    return result;
                if (!type->is<IR::Type_Extern>()) {
                    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected type %2% for property. "
                            "Must be extern.", ctrs, type);
                    return result;
                }
                auto te = type->to<IR::Type_Extern>();
                if (te->name != "direct_counter" && te->name != "counter") {
                    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected type %2% for property. "
                            "Must be 'counter' or 'direct_counter'.", ctrs, type);
                    return result;
                }
                auto jctr = new Util::JsonObject();
                cstring ctrname = ctrs->controlPlaneName("counter");
                jctr->emplace("name", ctrname);
                jctr->emplace("id", nextId("counter_arrays"));
                jctr->emplace_non_null("source_info", ctrs->sourceInfoJsonObj());
                // TODO(jafingerhut) - what kind of P4_16 code causes this
                // code to run, if any?
                bool direct = te->name == "direct_counter";
                jctr->emplace("is_direct", direct);
                jctr->emplace("binding", table->controlPlaneName());
                ctxt->json->counters->append(jctr);
            } else if (expr->is<IR::PathExpression>()) {
                auto pe = expr->to<IR::PathExpression>();
                auto decl = ctxt->refMap->getDeclaration(pe->path, true);
                if (!decl->is<IR::Declaration_Instance>()) {
                    ::error(ErrorType::ERR_EXPECTED, "an instance", decl->getNode());
                    return result;
                }
                cstring ctrname = decl->controlPlaneName();
                auto it = ctxt->structure->directCounterMap.find(ctrname);
                LOG3("Looking up " << ctrname);
                if (it != ctxt->structure->directCounterMap.end()) {
                    ::error(ErrorType::ERR_INVALID,
                            "Direct counters cannot be attached to multiple tables %2% and %3%",
                            decl, it->second, table);
                   return result;
                }
                ctxt->structure->directCounterMap.emplace(ctrname, table);
            } else {
                ::error(ErrorType::ERR_EXPECTED, "a counter", ctrs);
            }
        }
        result->emplace("with_counters", true);
    } else {
        result->emplace("with_counters", false);
    }

    bool sup_to = false;
    auto timeout = table->properties->getProperty("support_timeout");
    if (timeout != nullptr) {
        if (timeout->value->is<IR::ExpressionValue>()) {
            auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::BoolLiteral>()) {
                ::error(ErrorType::ERR_EXPECTED, "must true/false", timeout);
            } else {
                sup_to = expr->to<IR::BoolLiteral>()->value;
            }
        } else {
            ::error(ErrorType::ERR_EXPECTED, "a Boolean", timeout);
        }
    }
    result->emplace("support_timeout", sup_to);

    auto dm = table->properties->getProperty("meters");
    if (dm != nullptr) {
        if (dm->value->is<IR::ExpressionValue>()) {
            auto expr = dm->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::PathExpression>()) {
                ::error(ErrorType::ERR_EXPECTED, "a reference to a meter declaration", expr);
            } else {
                auto pe = expr->to<IR::PathExpression>();
                auto decl = ctxt->refMap->getDeclaration(pe->path, true);
                auto type = ctxt->typeMap->getType(expr, true);
                if (type == nullptr)
                    return result;
                if (type->is<IR::Type_SpecializedCanonical>())
                    type = type->to<IR::Type_SpecializedCanonical>()->baseType;
                if (!type->is<IR::Type_Extern>()) {
                    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected type %2% for property",
                            dm, type);
                    return result;
                }
                auto te = type->to<IR::Type_Extern>();
                if (te->name != "direct_meter") {
                    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected type %2% for property",
                            dm, type);
                    return result;
                }
                if (!decl->is<IR::Declaration_Instance>()) {
                    ::error(ErrorType::ERR_EXPECTED, "an instance", decl->getNode());
                    return result;
                }
                ctxt->structure->directMeterMap.setTable(decl, table);
                ctxt->structure->directMeterMap.setSize(decl, size);
                BUG_CHECK(decl->is<IR::Declaration_Instance>(),
                          "%1%: expected an instance", decl->getNode());
                cstring name = decl->controlPlaneName();
                result->emplace("direct_meters", name);
            }
        } else {
            ::error(ErrorType::ERR_EXPECTED, "a meter", dm);
        }
    } else {
        result->emplace("direct_meters", Util::JsonValue::null);
    }

    auto psa_counter = table->properties->getProperty("psa_direct_counter");
    if (psa_counter != nullptr) { /* TODO */ }

    auto psa_meter = table->properties->getProperty("psa_direct_meter");
    if (psa_meter != nullptr) { /* TODO */ }

    auto psa_implementation = table->properties->getProperty("psa_implementation");
    if (psa_implementation != nullptr) { /* TODO */ }

    auto action_ids = mkArrayField(result, "action_ids");
    auto actions = mkArrayField(result, "actions");
    auto al = table->getActionList();

    std::map<cstring, cstring> useActionName;
    for (auto a : al->actionList) {
        if (a->expression->is<IR::MethodCallExpression>()) {
            auto mce = a->expression->to<IR::MethodCallExpression>();
            if (mce->arguments->size() > 0)
                ::error(ErrorType::ERR_UNSUPPORTED, "actions in action list with arguments", a);
        }
        auto decl = ctxt->refMap->getDeclaration(a->getPath(), true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1%: should be an action name", a);
        auto action = decl->to<IR::P4Action>();
        unsigned id = get(ctxt->structure->ids, action, INVALID_ACTION_ID);
        LOG3("look up id " << action << " " << id);
        BUG_CHECK(id != INVALID_ACTION_ID, "Could not find id for %1%", action);
        action_ids->append(id);
        auto name = action->controlPlaneName();
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
            ::warning(ErrorType::WARN_UNSUPPORTED,
                      "Target does not support default_action for %1% (due to action profiles)",
                      table);
            return result;
        }

        if (!defact->value->is<IR::ExpressionValue>()) {
            ::error(ErrorType::ERR_EXPECTED, "an action", defact);
            return result;
        }
        auto expr = defact->value->to<IR::ExpressionValue>()->expression;
        const IR::P4Action* action = nullptr;
        const IR::Vector<IR::Argument>* args = nullptr;

        if (expr->is<IR::PathExpression>()) {
            auto path = expr->to<IR::PathExpression>()->path;
            auto decl = ctxt->refMap->getDeclaration(path, true);
            BUG_CHECK(decl->is<IR::P4Action>(), "%1%: should be an action name", expr);
            action = decl->to<IR::P4Action>();
        } else if (expr->is<IR::MethodCallExpression>()) {
            auto mce = expr->to<IR::MethodCallExpression>();
            auto mi = P4::MethodInstance::resolve(mce,
                    ctxt->refMap, ctxt->typeMap);
            BUG_CHECK(mi->is<P4::ActionCall>(), "%1%: expected an action", expr);
            action = mi->to<P4::ActionCall>()->action;
            args = mce->arguments;
        } else {
            BUG("%1%: unexpected expression", expr);
        }

        unsigned actionid = get(ctxt->structure->ids, action,
                                INVALID_ACTION_ID);
        BUG_CHECK(actionid != INVALID_ACTION_ID,
                  "Could not find id for %1%", action);
        auto entry = new Util::JsonObject();
        entry->emplace("action_id", actionid);
        entry->emplace("action_const", defact->isConstant);
        auto fields = mkArrayField(entry, "action_data");
        if (args != nullptr) {
            // TODO: use argument names
            for (auto a : *args) {
                if (a->expression->is<IR::Constant>()) {
                    cstring repr = stringRepr(a->expression->to<IR::Constant>()->value);
                    fields->append(repr);
                } else {
                    ::error(ErrorType::ERR_EXPECTED,
                            "argument must evaluate to a constant integer", a);
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

Util::IJson* ControlConverter::convertIf(const CFG::IfNode* node, cstring prefix) {
    (void) prefix;
    auto result = new Util::JsonObject();
    result->emplace("name", node->name);
    result->emplace("id", nextId("conditionals"));
    result->emplace_non_null("source_info", node->statement->condition->sourceInfoJsonObj());
    auto j = ctxt->conv->convert(node->statement->condition, true, false);
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

bool ControlConverter::preorder(const IR::P4Control* cont) {
    auto result = new Util::JsonObject();

    result->emplace("name", name);
    result->emplace("id", nextId("control"));
    result->emplace_non_null("source_info", cont->sourceInfoJsonObj());

    auto cfg = new CFG();
    cfg->build(cont, ctxt->refMap, ctxt->typeMap);
    bool success = cfg->checkImplementable();
    if (!success)
        return false;

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
    ctxt->action_profiles = action_profiles;

    SharedActionSelectorCheck selector_check(ctxt->refMap, ctxt->typeMap);
    cont->apply(selector_check);
    ctxt->selector_check = &selector_check;

    std::set<const IR::P4Table*> done;

    // Tables are created prior to the other local declarations
    for (auto node : cfg->allNodes) {
        auto tn = node->to<CFG::TableNode>();
        if (tn != nullptr) {
            if (done.find(tn->table) != done.end())
                // The same table may appear in multiple nodes in the CFG.
                // We emit it only once.  Other checks should ensure that
                // the CFG is implementable.
                continue;
            done.emplace(tn->table);
            auto j = convertTable(tn, action_profiles, selector_check);
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
        if (c->is<IR::Declaration_Constant>() ||
            c->is<IR::Declaration_Variable>() ||
            c->is<IR::P4Action>() ||
            c->is<IR::P4Table>())
            continue;
        if (c->is<IR::Declaration_Instance>()) {
            auto bl = ctxt->structure->resourceMap.at(c);
            CHECK_NULL(bl);
            if (bl->is<IR::ControlBlock>() || bl->is<IR::ParserBlock>())
                // Since this block has not been inlined, it is probably unused
                // So we don't do anything.
                continue;
            if (bl->is<IR::ExternBlock>()) {
                auto eb = bl->to<IR::ExternBlock>();
                ExternConverter::cvtExternInstance(ctxt, c, eb, emitExterns);
                continue;
            }
        }
        P4C_UNIMPLEMENTED("%1%: not yet handled", c);
    }

    ctxt->json->pipelines->append(result);
    return false;
}

}  // namespace BMV2
