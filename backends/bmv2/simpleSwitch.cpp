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
#include "backends/bmv2/backend.h"
#include "simpleSwitch.h"

using BMV2::mkArrayField;
using BMV2::mkParameters;
using BMV2::mkPrimitive;
using BMV2::nextId;
using BMV2::stringRepr;

namespace P4V1 {

// need a class to generate simpleSwitch

cstring jsonMetadataParameterName = "standard_metadata";

void
SimpleSwitch::modelError(const char* format, const IR::Node* node) const {
    ::error(format, node);
    ::error("Are you using an up-to-date v1model.p4?");
}

void
SimpleSwitch::addToFieldList(const IR::Expression* expr, Util::JsonArray* fl) {
    auto typeMap = backend->getTypeMap();
    auto conv = backend->getExpressionConverter();
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
SimpleSwitch::createFieldList(const IR::Expression* expr, cstring group,
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
SimpleSwitch::convertHashAlgorithm(cstring algorithm) {
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
SimpleSwitch::convertExternObjects(Util::JsonArray *result,
                                   const P4::ExternMethod *em,
                                   const IR::MethodCallExpression *mc,
                                   const IR::StatOrDecl *s) {
    auto conv = backend->getExpressionConverter();
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
            auto index = conv->convert(mc->arguments->at(0));
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
            auto index = conv->convert(mc->arguments->at(0));
            parameters->append(index);
            auto result = conv->convert(mc->arguments->at(1));
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
            auto dest = conv->convert(mc->arguments->at(0));
            parameters->append(dest);
            parameters->append(reg);
            auto index = conv->convert(mc->arguments->at(1));
            parameters->append(index);
        } else if (em->method->name == v1model.registers.write.name) {
            auto primitive = mkPrimitive("register_write", result);
            auto parameters = mkParameters(primitive);
            primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
            parameters->append(reg);
            auto index = conv->convert(mc->arguments->at(0));
            parameters->append(index);
            auto value = conv->convert(mc->arguments->at(1));
            parameters->append(value);
        }
    } else if (em->originalExternType->name == v1model.directMeter.name) {
        if (em->method->name == v1model.directMeter.read.name) {
            if (mc->arguments->size() != 1) {
                modelError("Expected 1 argument for %1%", mc);
                return;
            }
            auto dest = mc->arguments->at(0);
            backend->getMeterMap().setDestination(em->object, dest);
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
        error("Unknown extern type %1%", em->originalExternType->name);
    }
}

void
SimpleSwitch::convertExternFunctions(Util::JsonArray *result,
                                     const P4::ExternFunction *ef,
                                     const IR::MethodCallExpression *mc,
                                     const IR::StatOrDecl* s) {
    auto refMap = backend->getRefMap();
    auto typeMap = backend->getTypeMap();
    auto conv = backend->getExpressionConverter();
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
            id = createFieldList(emptylist, "field_lists", name, backend->field_lists);
        } else {
            if (mc->arguments->size() != 3) {
                modelError("Expected 3 arguments for %1%", mc);
                return;
            }
            cstring name = refMap->newName("fl");
            id = createFieldList(mc->arguments->at(2), "field_lists", name,
                                 backend->field_lists);
        }
        auto cloneType = mc->arguments->at(0);
        auto ei = P4::EnumInstance::resolve(cloneType, typeMap);
        if (ei == nullptr) {
            modelError("%1%: must be a constant on this target", cloneType);
        } else {
            cstring prim = ei->name == "I2E" ? "clone_ingress_pkt_to_egress" :
                    "clone_egress_pkt_to_egress";
            auto session = conv->convert(mc->arguments->at(1));
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
        auto dest = conv->convert(mc->arguments->at(0));
        parameters->append(dest);
        auto base = conv->convert(mc->arguments->at(2));
        parameters->append(base);
        auto calculation = new Util::JsonObject();
        auto ei = P4::EnumInstance::resolve(mc->arguments->at(1), typeMap);
        CHECK_NULL(ei);
        if (supportedHashAlgorithms.find(ei->name) == supportedHashAlgorithms.end()) {
            ::error("%1%: unexpected algorithm", ei->name);
            return;
        }
        auto fields = mc->arguments->at(3);
        auto calcName = createCalculation(ei->name, fields, backend->json->calculations,
                                          false, nullptr);
        calculation->emplace("type", "calculation");
        calculation->emplace("value", calcName);
        parameters->append(calculation);
        auto max = conv->convert(mc->arguments->at(4));
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
        auto dest = conv->convert(mc->arguments->at(0));
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
        int id = createFieldList(mc->arguments->at(1), "learn_lists",
                                 listName, backend->learn_lists);
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
        int id = createFieldList(mc->arguments->at(0), "field_lists",
                                 listName, backend->field_lists);
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
        auto dest = conv->convert(mc->arguments->at(0));
        auto lo = conv->convert(mc->arguments->at(1));
        auto hi = conv->convert(mc->arguments->at(2));
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
        auto len = conv->convert(mc->arguments->at(0));
        params->append(len);
    }
}

void
SimpleSwitch::convertExternInstances(const IR::Declaration *c,
                                     const IR::ExternBlock* eb,
                                     Util::JsonArray* action_profiles,
                                     BMV2::SharedActionSelectorCheck& selector_check) {
    CHECK_NULL(backend);
    auto conv = backend->getExpressionConverter();
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
        backend->counters->append(jctr);
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
        backend->meter_arrays->append(jmtr);
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
        backend->register_arrays->append(jreg);
    } else if (eb->type->name == v1model.directCounter.name) {
        auto it = backend->getDirectCounterMap().find(name);
        if (it == backend->getDirectCounterMap().end()) {
            ::warning("%1%: Direct counter not used; ignoring", inst);
        } else {
            auto jctr = new Util::JsonObject();
            jctr->emplace("name", name);
            jctr->emplace("id", nextId("counter_arrays"));
            // TODO(jafingerhut) - add line/col here?
            jctr->emplace("is_direct", true);
            jctr->emplace("binding", it->second->controlPlaneName());
            backend->counters->append(jctr);
        }
    } else if (eb->type->name == v1model.directMeter.name) {
        auto info = backend->getMeterMap().getInfo(c);
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
        if (name == v1model.meter.meterType.packets.name)
            type = "packets";
        else if (name == v1model.meter.meterType.bytes.name)
            type = "bytes";
        else
            modelError("%1%: unexpected meter type", mkind->getNode());
        jmtr->emplace("type", type);
        jmtr->emplace("size", info->tableSize);
        cstring tblname = info->table->controlPlaneName();
        jmtr->emplace("binding", tblname);
        auto result = conv->convert(info->destinationField);
        jmtr->emplace("result_target", result->to<Util::JsonObject>()->get("value"));
        backend->meter_arrays->append(jmtr);
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
    }
}

cstring
SimpleSwitch::createCalculation(cstring algo, const IR::Expression* fields,
                                Util::JsonArray* calculations, bool withPayload,
                                const IR::Node* sourcePositionNode = nullptr) {
    auto typeMap = backend->getTypeMap();
    auto refMap = backend->getRefMap();
    auto conv = backend->getExpressionConverter();
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
SimpleSwitch::convertChecksum(const IR::BlockStatement *block, Util::JsonArray* checksums,
                              Util::JsonArray* calculations, bool verify) {
    if (errorCount() > 0)
        return;
    auto typeMap = backend->getTypeMap();
    auto refMap = backend->getRefMap();
    auto conv = backend->getExpressionConverter();
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
                    auto ei = P4::EnumInstance::resolve(mi->expr->arguments->at(3), typeMap);
                    if (ei->name != "csum16") {
                        ::error("%1%: the only supported algorithm is csum16",
                                mi->expr->arguments->at(3));
                        return;
                    }
                    cstring calcName = createCalculation(ei->name, mi->expr->arguments->at(1),
                                                         calculations, usePayload, mc);
                    cksum->emplace("name", refMap->newName("cksum_"));
                    cksum->emplace("id", nextId("checksums"));
                    // TODO(jafingerhut) - add line/col here?
                    auto jleft = conv->convert(mi->expr->arguments->at(2));
                    cksum->emplace("target", jleft->to<Util::JsonObject>()->get("value"));
                    cksum->emplace("type", "generic");
                    cksum->emplace("calculation", calcName);
                    auto ifcond = conv->convert(mi->expr->arguments->at(0), true, false);
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

void
SimpleSwitch::setPipelineControls(const IR::ToplevelBlock* toplevel,
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

const IR::P4Control* SimpleSwitch::getIngress(const IR::ToplevelBlock* blk) {
    auto main = blk->getMain();
    auto ctrl = main->findParameterValue(v1model.sw.ingress.name);
    if (ctrl == nullptr)
        return nullptr;
    if (!ctrl->is<IR::ControlBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return nullptr;
    }
    return ctrl->to<IR::ControlBlock>()->container;
}

const IR::P4Control* SimpleSwitch::getEgress(const IR::ToplevelBlock* blk) {
    auto main = blk->getMain();
    auto ctrl = main->findParameterValue(v1model.sw.egress.name);
    if (ctrl == nullptr)
        return nullptr;
    if (!ctrl->is<IR::ControlBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return nullptr;
    }
    return ctrl->to<IR::ControlBlock>()->container;
}

const IR::P4Parser* SimpleSwitch::getParser(const IR::ToplevelBlock* blk) {
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
SimpleSwitch::setNonPipelineControls(const IR::ToplevelBlock* toplevel,
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
SimpleSwitch::getCompute(const IR::ToplevelBlock* toplevel) {
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
SimpleSwitch::getVerify(const IR::ToplevelBlock* toplevel) {
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
SimpleSwitch::setComputeChecksumControls(const IR::ToplevelBlock* toplevel,
                                         std::set<cstring>* controls) {
    auto ctrl = getCompute(toplevel);
    if (ctrl == nullptr)
        return;
    controls->emplace(ctrl->name);
}

void
SimpleSwitch::setVerifyChecksumControls(const IR::ToplevelBlock* toplevel,
                                        std::set<cstring>* controls) {
    auto ctrl = getVerify(toplevel);
    if (ctrl == nullptr)
        return;
    controls->emplace(ctrl->name);
}

void
SimpleSwitch::setDeparserControls(const IR::ToplevelBlock* toplevel,
                                  std::set<cstring>* controls) {
    auto main = toplevel->getMain();
    auto deparser = main->findParameterValue(v1model.sw.deparser.name);
    if (deparser == nullptr || !deparser->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return;
    }
    controls->emplace(deparser->to<IR::ControlBlock>()->container->name);
}

}  // namespace P4V1
