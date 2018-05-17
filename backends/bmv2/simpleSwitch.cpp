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

/**
 * This file implements the simple switch model
 */

#include <algorithm>
#include <cstring>
#include <set>
#include "frontends/p4/fromv1.0/v1model.h"
#include "simpleSwitch.h"
#include "control.h"
#include "deparser.h"
#include "globals.h"
#include "header.h"

using BMV2::mkArrayField;
using BMV2::mkParameters;
using BMV2::mkPrimitive;
using BMV2::nextId;
using BMV2::stringRepr;

namespace BMV2 {

void
SimpleSwitchBackend::modelError(const char* format, const IR::Node* node) const {
    ::error(format, node);
    ::error("Are you using an up-to-date v1model.p4?");
}

void
SimpleSwitchBackend::addToFieldList(const IR::Expression* expr, Util::JsonArray* fl) {
    if (expr->is<IR::ListExpression>()) {
        auto le = expr->to<IR::ListExpression>();
        for (auto e : le->components) {
            addToFieldList(e, fl);
        }
        return;
    }

    auto type = typeMap->getType(expr, true);
    if (type->is<IR::Type_StructLike>()) {
        // recursively add all fields
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields) {
            auto member = new IR::Member(expr, f->name);
            typeMap->setType(member, typeMap->getType(f, true));
            addToFieldList(member, fl);
        }
        return;
    }

    auto j = conv->convert(expr);
    if (auto jo = j->to<Util::JsonObject>()) {
        if (auto t = jo->get("type")) {
            if (auto type = t->to<Util::JsonValue>()) {
                if (*type == "runtime_data") {
                    // Can't have runtime_data in field lists -- need hexstr instead
                    auto val = jo->get("value")->to<Util::JsonValue>();
                    j = jo = new Util::JsonObject();
                    jo->emplace("type", "hexstr");
                    jo->emplace("value", stringRepr(val->getValue()));
                }
            }
        }
    }
    fl->append(j);
}

// returns id of created field list
int
SimpleSwitchBackend::createFieldList(const IR::Expression* expr, cstring group,
                              cstring listName, Util::JsonArray* field_lists) {
    auto fl = new Util::JsonObject();
    field_lists->append(fl);
    int id = nextId(group);
    fl->emplace("id", id);
    fl->emplace("name", listName);
    // TODO(jafingerhut) - add line/col here?
    auto elements = mkArrayField(fl, "elements");
    addToFieldList(expr, elements);
    return id;
}

cstring
SimpleSwitchBackend::convertHashAlgorithm(cstring algorithm) {
    cstring result;
    if (algorithm == v1model.algorithm.crc32.name)
        result = "crc32";
    else if (algorithm == v1model.algorithm.crc32_custom.name)
        result = "crc32_custom";
    else if (algorithm == v1model.algorithm.crc16.name)
        result = "crc16";
    else if (algorithm == v1model.algorithm.crc16_custom.name)
        result = "crc16_custom";
    else if (algorithm == v1model.algorithm.random.name)
        result = "random";
    else if (algorithm == v1model.algorithm.identity.name)
        result = "identity";
    else
        ::error("%1%: unexpected algorithm", algorithm);
    return result;
}

void
SimpleSwitchBackend::convertExternObjects(Util::JsonArray *result,
                                   const P4::ExternMethod *em,
                                   const IR::MethodCallExpression *mc,
                                   const IR::StatOrDecl *s,
                                   const bool& emitExterns) {
    if (em->originalExternType->name == v1model.counter.name) {
        if (em->method->name == v1model.counter.increment.name) {
            if (mc->arguments->size() != 1) {
                modelError("Expected 1 argument for %1%", mc);
                return;
            }
            auto primitive = mkPrimitive("count", result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            auto ctr = new Util::JsonObject();
            ctr->emplace("type", "counter_array");
            ctr->emplace("value", em->object->controlPlaneName());
            parameters->append(ctr);
            auto index = conv->convert(mc->arguments->at(0)->expression);
            parameters->append(index);
        }
    } else if (em->originalExternType->name == v1model.meter.name) {
        if (em->method->name == v1model.meter.executeMeter.name) {
            if (mc->arguments->size() != 2) {
                modelError("Expected 2 arguments for %1%", mc);
                return;
            }
            auto primitive = mkPrimitive("execute_meter", result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            auto mtr = new Util::JsonObject();
            mtr->emplace("type", "meter_array");
            mtr->emplace("value", em->object->controlPlaneName());
            parameters->append(mtr);
            auto index = conv->convert(mc->arguments->at(0)->expression);
            parameters->append(index);
            auto result = conv->convert(mc->arguments->at(1)->expression);
            parameters->append(result);
        }
    } else if (em->originalExternType->name == v1model.registers.name) {
        if (mc->arguments->size() != 2) {
            modelError("Expected 2 arguments for %1%", mc);
            return;
        }
        auto reg = new Util::JsonObject();
        reg->emplace("type", "register_array");
        cstring name = em->object->controlPlaneName();
        reg->emplace("value", name);
        if (em->method->name == v1model.registers.read.name) {
            auto primitive = mkPrimitive("register_read", result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            auto dest = conv->convert(mc->arguments->at(0)->expression);
            parameters->append(dest);
            parameters->append(reg);
            auto index = conv->convert(mc->arguments->at(1)->expression);
            parameters->append(index);
        } else if (em->method->name == v1model.registers.write.name) {
            auto primitive = mkPrimitive("register_write", result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            parameters->append(reg);
            auto index = conv->convert(mc->arguments->at(0)->expression);
            parameters->append(index);
            auto value = conv->convert(mc->arguments->at(1)->expression);
            parameters->append(value);
        }
    } else if (em->originalExternType->name == v1model.directMeter.name) {
        if (em->method->name == v1model.directMeter.read.name) {
            if (mc->arguments->size() != 1) {
                modelError("Expected 1 argument for %1%", mc);
                return;
            }
            auto dest = mc->arguments->at(0);
            meterMap.setDestination(em->object, dest->expression);
            // Do not generate any code for this operation
        }
    } else if (em->originalExternType->name == v1model.directCounter.name) {
        if (em->method->name == v1model.directCounter.count.name) {
            if (mc->arguments->size() != 0) {
                modelError("Expected 0 argument for %1%", mc);
                return;
            }
            // Do not generate any code for this operation
        }
    } else {
        if (emitExterns) {
            auto primitive = mkPrimitive("_" + em->originalExternType->name +
                                         "_" + em->method->name, result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            auto etr = new Util::JsonObject();
            etr->emplace("type", "extern");
            etr->emplace("value", em->object->getName());
            parameters->append(etr);
            for (auto arg : *mc->arguments) {
                auto args = conv->convert(arg->expression);
                parameters->append(args);
            }
        } else {
            error("Unknown extern method %1% from type %2%",
                em->method->name, em->originalExternType->name);
        }
    }
}

void
SimpleSwitchBackend::convertExternFunctions(Util::JsonArray *result,
                                     const P4::ExternFunction *ef,
                                     const IR::MethodCallExpression *mc,
                                     const IR::StatOrDecl* s) {
    if (ef->method->name == v1model.clone.name ||
        ef->method->name == v1model.clone.clone3.name) {
        int id = -1;
        if (ef->method->name == v1model.clone.name) {
            if (mc->arguments->size() != 2) {
                modelError("Expected 2 arguments for %1%", mc);
                return;
            }
            cstring name = refMap->newName("fl");
            auto emptylist = new IR::ListExpression({});
            id = createFieldList(emptylist, "field_lists", name, json->field_lists);
        } else {
            if (mc->arguments->size() != 3) {
                modelError("Expected 3 arguments for %1%", mc);
                return;
            }
            cstring name = refMap->newName("fl");
            id = createFieldList(mc->arguments->at(2)->expression, "field_lists", name,
                                 json->field_lists);
        }
        auto cloneType = mc->arguments->at(0);
        auto ei = P4::EnumInstance::resolve(cloneType->expression, typeMap);
        if (ei == nullptr) {
            modelError("%1%: must be a constant on this target", cloneType);
            return;
        } else {
            cstring prim = ei->name == "I2E" ? "clone_ingress_pkt_to_egress" :
                    "clone_egress_pkt_to_egress";
            auto session = conv->convert(mc->arguments->at(1)->expression);
            auto primitive = mkPrimitive(prim, result);
            auto parameters = mkParameters(primitive);
            // TODO(jafingerhut):
            // primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            parameters->append(session);

            if (id >= 0) {
                auto cst = new IR::Constant(id);
                typeMap->setType(cst, IR::Type_Bits::get(32));
                auto jcst = conv->convert(cst);
                parameters->append(jcst);
            }
        }
    } else if (ef->method->name == v1model.hash.name) {
        static std::set<cstring> supportedHashAlgorithms = {
            v1model.algorithm.crc32.name, v1model.algorithm.crc32_custom.name,
            v1model.algorithm.crc16.name, v1model.algorithm.crc16_custom.name,
            v1model.algorithm.random.name, v1model.algorithm.identity.name,
            v1model.algorithm.csum16.name, v1model.algorithm.xor16.name
        };

        if (mc->arguments->size() != 5) {
            modelError("Expected 5 arguments for %1%", mc);
            return;
        }
        auto primitive = mkPrimitive("modify_field_with_hash_based_offset", result);
        auto parameters = mkParameters(primitive);
        primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        auto dest = conv->convert(mc->arguments->at(0)->expression);
        parameters->append(dest);
        auto base = conv->convert(mc->arguments->at(2)->expression);
        parameters->append(base);
        auto calculation = new Util::JsonObject();
        auto ei = P4::EnumInstance::resolve(mc->arguments->at(1)->expression, typeMap);
        CHECK_NULL(ei);
        if (supportedHashAlgorithms.find(ei->name) == supportedHashAlgorithms.end()) {
            ::error("%1%: unexpected algorithm", ei->name);
            return;
        }
        auto fields = mc->arguments->at(3);
        auto calcName = createCalculation(ei->name, fields->expression, json->calculations,
                                          false, nullptr);
        calculation->emplace("type", "calculation");
        calculation->emplace("value", calcName);
        parameters->append(calculation);
        auto max = conv->convert(mc->arguments->at(4)->expression);
        parameters->append(max);
    } else if (ef->method->name == v1model.digest_receiver.name) {
        if (mc->arguments->size() != 2) {
            modelError("Expected 2 arguments for %1%", mc);
            return;
        }
        auto primitive = mkPrimitive("generate_digest", result);
        auto parameters = mkParameters(primitive);
        // TODO(jafingerhut):
        // primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        auto dest = conv->convert(mc->arguments->at(0)->expression);
        parameters->append(dest);
        cstring listName = "digest";
        // If we are supplied a type argument that is a named type use
        // that for the list name.
        if (mc->typeArguments->size() == 1) {
            auto typeArg = mc->typeArguments->at(0);
            if (typeArg->is<IR::Type_Name>()) {
                auto origType = refMap->getDeclaration(
                    typeArg->to<IR::Type_Name>()->path, true);
                if (!origType->is<IR::Type_Struct>()) {
                    modelError("%1%: expected a struct type", origType->getNode());
                    return;
                }
                auto st = origType->to<IR::Type_Struct>();
                listName = st->controlPlaneName();
            }
        }
        int id = createFieldList(mc->arguments->at(1)->expression, "learn_lists",
                                 listName, json->learn_lists);
        auto cst = new IR::Constant(id);
        typeMap->setType(cst, IR::Type_Bits::get(32));
        auto jcst = conv->convert(cst);
        parameters->append(jcst);
    } else if (ef->method->name == v1model.resubmit.name ||
               ef->method->name == v1model.recirculate.name) {
        if (mc->arguments->size() != 1) {
            modelError("Expected 1 argument for %1%", mc);
            return;
        }
        cstring prim = (ef->method->name == v1model.resubmit.name) ?
                "resubmit" : "recirculate";
        auto primitive = mkPrimitive(prim, result);
        auto parameters = mkParameters(primitive);
        // TODO(jafingerhut):
        // primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        cstring listName = prim;
        // If we are supplied a type argument that is a named type use
        // that for the list name.
        if (mc->typeArguments->size() == 1) {
            auto typeArg = mc->typeArguments->at(0);
            if (typeArg->is<IR::Type_Name>()) {
                auto origType = refMap->getDeclaration(
                    typeArg->to<IR::Type_Name>()->path, true);
                if (!origType->is<IR::Type_Struct>()) {
                    modelError("%1%: expected a struct type", origType->getNode());
                    return;
                }
                auto st = origType->to<IR::Type_Struct>();
                listName = st->controlPlaneName();
            }
        }
        int id = createFieldList(mc->arguments->at(0)->expression, "field_lists",
                                 listName, json->field_lists);
        auto cst = new IR::Constant(id);
        typeMap->setType(cst, IR::Type_Bits::get(32));
        auto jcst = conv->convert(cst);
        parameters->append(jcst);
    } else if (ef->method->name == v1model.drop.name) {
        if (mc->arguments->size() != 0) {
            modelError("Expected 0 arguments for %1%", mc);
            return;
        }
        auto primitive = mkPrimitive("drop", result);
        (void)mkParameters(primitive);
        primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
    } else if (ef->method->name == v1model.random.name) {
        if (mc->arguments->size() != 3) {
            modelError("Expected 3 arguments for %1%", mc);
            return;
        }
        auto primitive =
                mkPrimitive(v1model.random.modify_field_rng_uniform.name, result);
        auto params = mkParameters(primitive);
        // TODO(jafingerhut):
        // primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        auto dest = conv->convert(mc->arguments->at(0)->expression);
        auto lo = conv->convert(mc->arguments->at(1)->expression);
        auto hi = conv->convert(mc->arguments->at(2)->expression);
        params->append(dest);
        params->append(lo);
        params->append(hi);
    } else if (ef->method->name == v1model.truncate.name) {
        if (mc->arguments->size() != 1) {
            modelError("Expected 1 arguments for %1%", mc);
            return;
        }
        auto primitive = mkPrimitive(v1model.truncate.name, result);
        auto params = mkParameters(primitive);
        // TODO(jafingerhut):
        // primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        auto len = conv->convert(mc->arguments->at(0)->expression);
        params->append(len);
    }
}

void
SimpleSwitchBackend::convertExternInstances(const IR::Declaration *c,
                                     const IR::ExternBlock* eb,
                                     Util::JsonArray* action_profiles,
                                     BMV2::SharedActionSelectorCheck& selector_check,
                                     const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    if (eb->type->name == v1model.counter.name) {
        auto jctr = new Util::JsonObject();
        jctr->emplace("name", name);
        jctr->emplace("id", nextId("counter_arrays"));
        jctr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
        auto sz = eb->findParameterValue(v1model.counter.sizeParam.name);
        CHECK_NULL(sz);
        if (!sz->is<IR::Constant>()) {
            modelError("%1%: expected a constant", sz->getNode());
            return;
        }
        jctr->emplace("size", sz->to<IR::Constant>()->value);
        jctr->emplace("is_direct", false);
        json->counters->append(jctr);
    } else if (eb->type->name == v1model.meter.name) {
        auto jmtr = new Util::JsonObject();
        jmtr->emplace("name", name);
        jmtr->emplace("id", nextId("meter_arrays"));
        jmtr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
        jmtr->emplace("is_direct", false);
        auto sz = eb->findParameterValue(v1model.meter.sizeParam.name);
        CHECK_NULL(sz);
        if (!sz->is<IR::Constant>()) {
            modelError("%1%: expected a constant", sz->getNode());
            return;
        }
        jmtr->emplace("size", sz->to<IR::Constant>()->value);
        jmtr->emplace("rate_count", 2);
        auto mkind = eb->findParameterValue(v1model.meter.typeParam.name);
        CHECK_NULL(mkind);
        if (!mkind->is<IR::Declaration_ID>()) {
            modelError("%1%: expected a member", mkind->getNode());
            return;
        }
        cstring name = mkind->to<IR::Declaration_ID>()->name;
        cstring type = "?";
        if (name == v1model.meter.meterType.packets.name)
            type = "packets";
        else if (name == v1model.meter.meterType.bytes.name)
            type = "bytes";
        else
            ::error("Unexpected meter type %1%", mkind->getNode());
        jmtr->emplace("type", type);
        json->meter_arrays->append(jmtr);
    } else if (eb->type->name == v1model.registers.name) {
        auto jreg = new Util::JsonObject();
        jreg->emplace("name", name);
        jreg->emplace("id", nextId("register_arrays"));
        jreg->emplace_non_null("source_info", eb->sourceInfoJsonObj());
        auto sz = eb->findParameterValue(v1model.registers.sizeParam.name);
        CHECK_NULL(sz);
        if (!sz->is<IR::Constant>()) {
            modelError("%1%: expected a constant", sz->getNode());
            return;
        }
        if (sz->to<IR::Constant>()->value == 0)
            error("%1%: direct registers are not supported in bmv2", inst);
        jreg->emplace("size", sz->to<IR::Constant>()->value);
        if (!eb->instanceType->is<IR::Type_SpecializedCanonical>()) {
            modelError("%1%: Expected a generic specialized type", eb->instanceType);
            return;
        }
        auto st = eb->instanceType->to<IR::Type_SpecializedCanonical>();
        if (st->arguments->size() != 1) {
            modelError("%1%: expected 1 type argument", st);
            return;
        }
        auto regType = st->arguments->at(0);
        if (!regType->is<IR::Type_Bits>()) {
            ::error("%1%: Only registers with bit or int types are currently supported", eb);
            return;
        }
        unsigned width = regType->width_bits();
        if (width == 0) {
            ::error("%1%: unknown width", st->arguments->at(0));
            return;
        }
        jreg->emplace("bitwidth", width);
        json->register_arrays->append(jreg);
    } else if (eb->type->name == v1model.directCounter.name) {
        auto it = directCounterMap.find(name);
        if (it == directCounterMap.end()) {
            ::warning("%1%: Direct counter not used; ignoring", inst);
        } else {
            auto jctr = new Util::JsonObject();
            jctr->emplace("name", name);
            jctr->emplace("id", nextId("counter_arrays"));
            // TODO(jafingerhut) - add line/col here?
            jctr->emplace("is_direct", true);
            jctr->emplace("binding", it->second->controlPlaneName());
            json->counters->append(jctr);
        }
    } else if (eb->type->name == v1model.directMeter.name) {
        auto info = meterMap.getInfo(c);
        CHECK_NULL(info);
        CHECK_NULL(info->table);
        CHECK_NULL(info->destinationField);

        auto jmtr = new Util::JsonObject();
        jmtr->emplace("name", name);
        jmtr->emplace("id", nextId("meter_arrays"));
        jmtr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
        jmtr->emplace("is_direct", true);
        jmtr->emplace("rate_count", 2);
        auto mkind = eb->findParameterValue(v1model.directMeter.typeParam.name);
        CHECK_NULL(mkind);
        if (!mkind->is<IR::Declaration_ID>()) {
            modelError("%1%: expected a member", mkind->getNode());
            return;
        }
        cstring name = mkind->to<IR::Declaration_ID>()->name;
        cstring type = "?";
        if (name == v1model.meter.meterType.packets.name) {
            type = "packets";
        } else if (name == v1model.meter.meterType.bytes.name) {
            type = "bytes";
        } else {
            modelError("%1%: unexpected meter type", mkind->getNode());
            return;
        }
        jmtr->emplace("type", type);
        jmtr->emplace("size", info->tableSize);
        cstring tblname = info->table->controlPlaneName();
        jmtr->emplace("binding", tblname);
        auto result = conv->convert(info->destinationField);
        jmtr->emplace("result_target", result->to<Util::JsonObject>()->get("value"));
        json->meter_arrays->append(jmtr);
    } else if (eb->type->name == v1model.action_profile.name ||
            eb->type->name == v1model.action_selector.name) {
        // Might call this multiple times if the selector/profile is used more than
        // once in a pipeline, so only add it to the action_profiles once
        if (BMV2::JsonObjects::find_object_by_name(action_profiles, name))
            return;
        auto action_profile = new Util::JsonObject();
        action_profile->emplace("name", name);
        action_profile->emplace("id", nextId("action_profiles"));
        // TODO(jafingerhut) - add line/col here?

        auto add_size = [&action_profile, &eb](const cstring &pname) {
            auto sz = eb->findParameterValue(pname);
            if (!sz->is<IR::Constant>()) {
                ::error("%1%: expected a constant", sz);
                return;
            }
            action_profile->emplace("max_size", sz->to<IR::Constant>()->value);
        };

        if (eb->type->name == v1model.action_profile.name) {
            add_size(v1model.action_profile.sizeParam.name);
        } else {
            add_size(v1model.action_selector.sizeParam.name);
            auto selector = new Util::JsonObject();
            auto hash = eb->findParameterValue(
                    v1model.action_selector.algorithmParam.name);
            if (!hash->is<IR::Declaration_ID>()) {
                modelError("%1%: expected a member", hash->getNode());
                return;
            }
            auto algo = convertHashAlgorithm(hash->to<IR::Declaration_ID>()->name);
            selector->emplace("algo", algo);
            auto input = selector_check.get_selector_input(
                    c->to<IR::Declaration_Instance>());
            if (input == nullptr) {
                // the selector is never used by any table, we cannot figure out its
                // input and therefore cannot include it in the JSON
                ::warning("Action selector '%1%' is never referenced by a table "
                        "and cannot be included in bmv2 JSON", c);
                return;
            }
            auto j_input = mkArrayField(selector, "input");
            for (auto expr : *input) {
                auto jk = conv->convert(expr);
                j_input->append(jk);
            }
            action_profile->emplace("selector", selector);
        }

        action_profiles->append(action_profile);
    } else {
        if (!emitExterns)
            error("Unknown extern instance %1%", eb->type->name);
    }
}

cstring
SimpleSwitchBackend::createCalculation(cstring algo, const IR::Expression* fields,
                                Util::JsonArray* calculations, bool withPayload,
                                const IR::Node* sourcePositionNode = nullptr) {
    cstring calcName = refMap->newName("calc_");
    auto calc = new Util::JsonObject();
    calc->emplace("name", calcName);
    calc->emplace("id", nextId("calculations"));
    if (sourcePositionNode != nullptr)
        calc->emplace_non_null("source_info", sourcePositionNode->sourceInfoJsonObj());
    calc->emplace("algo", algo);
    if (!fields->is<IR::ListExpression>()) {
        // expand it into a list
        auto list = new IR::ListExpression({});
        auto type = typeMap->getType(fields, true);
        if (!type->is<IR::Type_StructLike>()) {
            modelError("%1%: expected a struct", fields);
            return calcName;
        }
        for (auto f : type->to<IR::Type_StructLike>()->fields) {
            auto e = new IR::Member(fields, f->name);
            auto ftype = typeMap->getType(f);
            typeMap->setType(e, ftype);
            list->push_back(e);
        }
        fields = list;
        typeMap->setType(fields, type);
    }
    auto jright = conv->convertWithConstantWidths(fields);
    if (withPayload) {
        auto array = jright->to<Util::JsonArray>();
        BUG_CHECK(array, "expected a JSON array");
        auto payload = new Util::JsonObject();
        payload->emplace("type", "payload");
        payload->emplace("value", (Util::IJson*)nullptr);
        array->append(payload);
    }
    calc->emplace("input", jright);
    calculations->append(calc);
    return calcName;
}

void
SimpleSwitchBackend::convertChecksum(const IR::BlockStatement *block, Util::JsonArray* checksums,
                              Util::JsonArray* calculations, bool verify) {
    if (errorCount() > 0)
        return;
    for (auto stat : block->components) {
        if (auto blk = stat->to<IR::BlockStatement>()) {
            convertChecksum(blk, checksums, calculations, verify);
            continue;
        } else if (auto mc = stat->to<IR::MethodCallStatement>()) {
            auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap);
            if (auto em = mi->to<P4::ExternFunction>()) {
                cstring functionName = em->method->name.name;
                if ((verify && (functionName == v1model.verify_checksum.name ||
                                functionName == v1model.verify_checksum_with_payload.name)) ||
                    (!verify && (functionName == v1model.update_checksum.name ||
                                 functionName == v1model.update_checksum_with_payload.name))) {
                    bool usePayload = functionName.endsWith("_with_payload");
                    if (mi->expr->arguments->size() != 4) {
                        modelError("%1%: Expected 4 arguments", mc);
                        return;
                    }
                    auto cksum = new Util::JsonObject();
                    auto ei = P4::EnumInstance::resolve(
                        mi->expr->arguments->at(3)->expression, typeMap);
                    if (ei->name != "csum16") {
                        ::error("%1%: the only supported algorithm is csum16",
                                mi->expr->arguments->at(3)->expression);
                        return;
                    }
                    cstring calcName = createCalculation(
                        ei->name, mi->expr->arguments->at(1)->expression,
                        calculations, usePayload, mc);
                    cksum->emplace("name", refMap->newName("cksum_"));
                    cksum->emplace("id", nextId("checksums"));
                    // TODO(jafingerhut) - add line/col here?
                    auto jleft = conv->convert(mi->expr->arguments->at(2)->expression);
                    cksum->emplace("target", jleft->to<Util::JsonObject>()->get("value"));
                    cksum->emplace("type", "generic");
                    cksum->emplace("calculation", calcName);
                    auto ifcond = conv->convert(
                        mi->expr->arguments->at(0)->expression, true, false);
                    cksum->emplace("if_cond", ifcond);
                    checksums->append(cksum);
                    continue;
                }
            }
        }
        ::error("%1%: Only calls to %2% or %3% allowed", stat,
                verify ? v1model.verify_checksum.name : v1model.update_checksum.name,
                verify ? v1model.verify_checksum_with_payload.name :
                v1model.update_checksum_with_payload.name);
    }
}

void SimpleSwitchBackend::convertActionBody(const IR::Vector<IR::StatOrDecl>* body,
                                     Util::JsonArray* result) {
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

            auto type = typeMap->getType(l, true);
            cstring operation = BMV2::Backend::jsonAssignment(type, false);
            auto primitive = mkPrimitive(operation, result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", assign->sourceInfoJsonObj());
            auto left = conv->convertLeftValue(l);
            parameters->append(left);
            bool convertBool = type->is<IR::Type_Boolean>();
            auto right = conv->convert(r, true, true, convertBool);
            parameters->append(right);
            continue;
        } else if (s->is<IR::EmptyStatement>()) {
            continue;
        } else if (s->is<IR::MethodCallStatement>()) {
            LOG3("Visit " << dbp(s));
            auto mc = s->to<IR::MethodCallStatement>()->methodCall;
            auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap);
            if (mi->is<P4::ActionCall>()) {
                BUG("%1%: action call should have been inlined", mc);
                continue;
            } else if (mi->is<P4::BuiltInMethod>()) {
                auto builtin = mi->to<P4::BuiltInMethod>();

                cstring prim;
                auto parameters = new Util::JsonArray();
                auto obj = conv->convert(builtin->appliedTo);
                parameters->append(obj);

                if (builtin->name == IR::Type_Header::setValid) {
                    prim = "add_header";
                } else if (builtin->name == IR::Type_Header::setInvalid) {
                    prim = "remove_header";
                } else if (builtin->name == IR::Type_Stack::push_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = conv->convert(mc->arguments->at(0)->expression);
                    prim = "push";
                    parameters->append(arg);
                } else if (builtin->name == IR::Type_Stack::pop_front) {
                    BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
                    auto arg = conv->convert(mc->arguments->at(0)->expression);
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
                convertExternObjects(result, em, mc, s, options.emitExterns);
                continue;
            } else if (mi->is<P4::ExternFunction>()) {
                auto ef = mi->to<P4::ExternFunction>();
                convertExternFunctions(result, ef, mc, s);
                continue;
            }
        }
        ::error("%1%: not yet supported on this target", s);
    }
}

void SimpleSwitchBackend::convertActionParams(const IR::ParameterList *parameters, Util::JsonArray* params) {
    for (auto p : *parameters->getEnumerator()) {
        if (!refMap->isUsed(p))
            ::warning("Unused action parameter %1%", p);

        auto param = new Util::JsonObject();
        param->emplace("name", p->name);
        auto type = typeMap->getType(p, true);
        if (!type->is<IR::Type_Bits>())
            ::error("%1%: Action parameters can only be bit<> or int<> on this target", p);
        param->emplace("bitwidth", type->width_bits());
        params->append(param);
    }
}

void SimpleSwitchBackend::createActions() {
    for (auto it : structure.actions) {
        auto action = it.first;
        cstring name = action->controlPlaneName();
        auto params = new Util::JsonArray();
        convertActionParams(action->parameters, params);
        auto body = new Util::JsonArray();
        convertActionBody(&action->body->components, body);
        auto id = json->add_action(name, params, body);
        structure.ids.emplace(action, id);
    }
}

const IR::P4Parser*
SimpleSwitchBackend::getParser(const IR::ToplevelBlock* blk) {
    auto main = blk->getMain();
    auto ctrl = main->findParameterValue(v1model.sw.parser.name);
    if (ctrl == nullptr)
        return nullptr;
    if (!ctrl->is<IR::ParserBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return nullptr;
    }
    return ctrl->to<IR::ParserBlock>()->container;
}

void
SimpleSwitchBackend::setPipelineControls(const IR::ToplevelBlock* toplevel,
                                  std::set<cstring>* controls,
                                  std::map<cstring, cstring>* map) {
    if (errorCount() != 0)
        return;
    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::error("`%1%' module not found for simple switch", IR::P4Program::main);
        return;
    }
    auto ingress = main->findParameterValue(v1model.sw.ingress.name);
    auto egress = main->findParameterValue(v1model.sw.egress.name);
    if (ingress == nullptr || egress == nullptr ||
        !ingress->is<IR::ControlBlock>() || !egress->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return;
    }
    auto ingress_name = ingress->to<IR::ControlBlock>()->container->name;
    auto egress_name = egress->to<IR::ControlBlock>()->container->name;
    controls->emplace(ingress_name);
    controls->emplace(egress_name);
    map->emplace(ingress_name, "ingress");
    map->emplace(egress_name, "egress");
}

void
SimpleSwitchBackend::setNonPipelineControls(const IR::ToplevelBlock* toplevel,
                                     std::set<cstring>* controls) {
    if (errorCount() != 0)
        return;
    auto main = toplevel->getMain();
    auto verify = getVerify(toplevel);
    auto compute = getCompute(toplevel);
    auto deparser = main->findParameterValue(v1model.sw.deparser.name);
    if (verify == nullptr || compute == nullptr || deparser == nullptr ||
        !deparser->is<IR::ControlBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return;
    }
    controls->emplace(verify->name);
    controls->emplace(compute->name);
    controls->emplace(deparser->to<IR::ControlBlock>()->container->name);
}

const IR::P4Control*
SimpleSwitchBackend::getCompute(const IR::ToplevelBlock* toplevel) {
    if (errorCount() != 0)
        return nullptr;
    auto main = toplevel->getMain();
    auto update = main->findParameterValue(v1model.sw.compute.name);
    if (update == nullptr || !update->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return nullptr;
    }
    return update->to<IR::ControlBlock>()->container;
}

const IR::P4Control*
SimpleSwitchBackend::getVerify(const IR::ToplevelBlock* toplevel) {
    if (errorCount() != 0)
        return nullptr;
    auto main = toplevel->getMain();
    auto verify = main->findParameterValue(v1model.sw.verify.name);
    if (verify == nullptr || !verify->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return nullptr;
    }
    return verify->to<IR::ControlBlock>()->container;
}

void
SimpleSwitchBackend::setComputeChecksumControls(const IR::ToplevelBlock* toplevel,
                                         std::set<cstring>* controls) {
    auto ctrl = getCompute(toplevel);
    if (ctrl == nullptr)
        return;
    controls->emplace(ctrl->name);
}

void
SimpleSwitchBackend::setVerifyChecksumControls(const IR::ToplevelBlock* toplevel,
                                        std::set<cstring>* controls) {
    auto ctrl = getVerify(toplevel);
    if (ctrl == nullptr)
        return;
    controls->emplace(ctrl->name);
}

void
SimpleSwitchBackend::setDeparserControls(const IR::ToplevelBlock* toplevel,
                                  std::set<cstring>* controls) {
    auto main = toplevel->getMain();
    auto deparser = main->findParameterValue(v1model.sw.deparser.name);
    if (deparser == nullptr || !deparser->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return;
    }
    controls->emplace(deparser->to<IR::ControlBlock>()->container->name);
}

void
SimpleSwitchBackend::convert(const IR::ToplevelBlock* tlb, BMV2::BMV2Options& options) {
    CHECK_NULL(tlb);
    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    if (tlb->getMain() == nullptr)
        return;  // no main

    /// Declaration which introduces the user metadata.
    /// We expect this to be a struct type.
    const IR::Type_Struct* userMetaType = nullptr;
    cstring userMetaName = refMap->newName("userMetadata");

    setPipelineControls(tlb, &pipeline_controls, &pipeline_namemap);
    setNonPipelineControls(tlb, &non_pipeline_controls);
    setComputeChecksumControls(tlb, &compute_checksum_controls);
    setVerifyChecksumControls(tlb, &verify_checksum_controls);
    setDeparserControls(tlb, &deparser_controls);
    if (::errorCount() > 0)
        return;

    // Find the user metadata declaration
    auto parser = getParser(tlb);
    auto params = parser->getApplyParameters();
    BUG_CHECK(params->size() == 4, "%1%: expected 4 parameters", parser);
    auto metaParam = params->parameters.at(2);
    auto paramType = metaParam->type;
    if (!paramType->is<IR::Type_Name>()) {
        ::error("%1%: expected the user metadata type to be a struct", paramType);
        return;
    }
    auto decl = refMap->getDeclaration(paramType->to<IR::Type_Name>()->path);
    if (!decl->is<IR::Type_Struct>()) {
        ::error("%1%: expected the user metadata type to be a struct", paramType);
        return;
    }
    userMetaType = decl->to<IR::Type_Struct>();
    LOG2("User metadata type is " << userMetaType);

    // These passes are logically bmv2-specific
    PassManager simplify = {
        new ExtractMatchKind(this),
        new RenameUserMetadata(refMap, userMetaType, userMetaName),
        new P4::ClearTypeMap(typeMap),  // because the user metadata type has changed
        new P4::SynthesizeActions(refMap, typeMap, new SkipControls(&non_pipeline_controls)),
        new P4::MoveActionsToTables(refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new LowerExpressions(typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        new RemoveComplexExpressions(refMap, typeMap, new ProcessControls(&pipeline_controls)),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        new DiscoverStructure(&structure),
        evaluator,
    };
    tlb->getProgram()->apply(simplify);

    toplevel = evaluator->getToplevelBlock();
    toplevel->apply(*new BMV2::BuildResourceMap(this));

    // field list and learn list ids in bmv2 are not consistent with ids for
    // other objects: they need to start at 1 (not 0) since the id is also used
    // as a "flag" to indicate that a certain simple_switch primitive has been
    // called (e.g. resubmit or generate_digest)
    BMV2::nextId("field_lists");
    BMV2::nextId("learn_lists");
    json->add_program_info(options.file);
    json->add_meta_info();

    // convert all enums to json
    for (const auto &pEnum : *enumMap) {
        auto name = pEnum.first->getName();
        for (const auto &pEntry : *pEnum.second) {
            json->add_enum(name, pEntry.first, pEntry.second);
        }
    }
    if (::errorCount() > 0)
        return;

    /// generate error types
    for (const auto &p : structure.errorCodesMap) {
        auto name = p.first->toString();
        auto type = p.second;
        json->add_error(name, type);
    }

    cstring scalarsName = refMap->newName("scalars");

    // This visitor is used in multiple passes to convert expression to json
    conv = new SimpleSwitchExpressionConverter(refMap, typeMap, &structure,
                                                     scalarsName, toplevel);

    PassManager codegen_passes = {
        new HeaderConverter(refMap, typeMap, &structure, json, scalarsName),
        new ParserConverter(refMap, typeMap, json, conv),
        new VisitFunctor([this]() { createActions(); }),
        new ControlConverter(this, refMap, typeMap, json, conv, &structure, options.emitExterns),
        new ChecksumConverter(this, json),
        new DeparserConverter(this),
    };

    codegen_passes.setName("CodeGen");
    CHECK_NULL(toplevel);
    auto program = toplevel->getProgram();
    program->apply(codegen_passes);

    (void)toplevel->apply(ConvertGlobals(this));
}

}  // namespace BMV2
