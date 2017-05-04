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
#include <frontends/p4/fromv1.0/v1model.h>
#include <backends/bmv2/backend.h>

using BMV2::extVisibleName;
using BMV2::mkArrayField;
using BMV2::mkParameters;
using BMV2::mkPrimitive;
using BMV2::nextId;

namespace P4V1 {

cstring                jsonMetadataParameterName = "standard_metadata";



static void addToFieldList(BMV2::Backend *bmv2, const IR::Expression* expr,
                           Util::JsonArray* fl)
{
    if (expr->is<IR::ListExpression>()) {
        auto le = expr->to<IR::ListExpression>();
        for (auto e : le->components) {
            addToFieldList(bmv2, e, fl);
        }
        return;
    }

    auto type = bmv2->getTypeMap().getType(expr, true);
    if (type->is<IR::Type_StructLike>()) {
        // recursively add all fields
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields) {
            auto member = new IR::Member(expr, f->name);
            bmv2->getTypeMap().setType(member, bmv2->getTypeMap().getType(f, true));
            addToFieldList(bmv2, member, fl);
        }
        return;
    }

    auto j = bmv2->getExpressionConverter()->convert(expr);
    fl->append(j);
}

// returns id of created field list
static int createFieldList(BMV2::Backend *bmv2, const IR::Expression* expr,
                           cstring group, cstring listName,
                           Util::JsonArray* field_lists)
{
    auto fl = new Util::JsonObject();
    field_lists->append(fl);
    int id = nextId(group);
    fl->emplace("id", id);
    fl->emplace("name", listName);
    auto elements = mkArrayField(fl, "elements");
    addToFieldList(bmv2, expr, elements);
    return id;
}


void V1Model::convertExternObjects(Util::JsonArray *result, BMV2::Backend *bmv2,
                                  const P4::ExternMethod *em,
                                  const IR::MethodCallExpression *mc)
{
    LOG1("...convert extern object " << mc);
    if (em->originalExternType->name == instance.counter.name) {
        if (em->method->name == instance.counter.increment.name) {
            LOG1("create counter ");
            BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
            auto primitive = mkPrimitive("count", result);
            auto parameters = mkParameters(primitive);
            auto ctr = new Util::JsonObject();
            ctr->emplace("type", "counter_array");
            ctr->emplace("value", extVisibleName(em->object));
            parameters->append(ctr);
            auto index = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
            parameters->append(index);
        }
    } else if (em->originalExternType->name == instance.meter.name) {
        if (em->method->name == instance.meter.executeMeter.name) {
            BUG_CHECK(mc->arguments->size() == 2, "Expected 2 arguments for %1%", mc);
            auto primitive = mkPrimitive("execute_meter", result);
            auto parameters = mkParameters(primitive);
            auto mtr = new Util::JsonObject();
            mtr->emplace("type", "meter_array");
            mtr->emplace("value", extVisibleName(em->object));
            parameters->append(mtr);
            auto index = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
            parameters->append(index);
            auto result = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
            parameters->append(result);
        }
    } else if (em->originalExternType->name == instance.registers.name) {
        BUG_CHECK(mc->arguments->size() == 2, "Expected 2 arguments for %1%", mc);
        auto reg = new Util::JsonObject();
        reg->emplace("type", "register_array");
        cstring name = extVisibleName(em->object);
        reg->emplace("value", name);
        if (em->method->name == instance.registers.read.name) {
            auto primitive = mkPrimitive("register_read", result);
            auto parameters = mkParameters(primitive);
            auto dest = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
            parameters->append(dest);
            parameters->append(reg);
            auto index = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
            parameters->append(index);
        } else if (em->method->name == instance.registers.write.name) {
            auto primitive = mkPrimitive("register_write", result);
            auto parameters = mkParameters(primitive);
            parameters->append(reg);
            auto index = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
            parameters->append(index);
            auto value = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
            parameters->append(value);
        }
    } else if (em->originalExternType->name == instance.directMeter.name) {
        if (em->method->name == instance.directMeter.read.name) {
            BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
            auto dest = mc->arguments->at(0);
            bmv2->getMeterMap().setDestination(em->object, dest);
            // Do not generate any code for this operation
        }
    } else if (em->originalExternType->name == instance.directCounter.name) {
        LOG1("create direct counter ");
        if (em->method->name == instance.directCounter.count.name) {
            BUG_CHECK(mc->arguments->size() == 0, "Expected 0 argument for %1%", mc);
            // Do not generate any code for this operation
        }
    }

}

void V1Model::convertExternFunctions(Util::JsonArray *result, BMV2::Backend *bmv2,
                                     const P4::ExternFunction *ef,
                                     const IR::MethodCallExpression *mc)
{
    if (ef->method->name == instance.clone.name ||
        ef->method->name == instance.clone.clone3.name) {
        int id = -1;
        if (ef->method->name == instance.clone.name) {
            BUG_CHECK(mc->arguments->size() == 2, "Expected 2 arguments for %1%", mc);
            cstring name = bmv2->getRefMap().newName("fl");
            auto emptylist = new IR::ListExpression({});
            id = createFieldList(bmv2, emptylist, "field_lists", name, bmv2->field_lists);
        } else {
            BUG_CHECK(mc->arguments->size() == 3, "Expected 3 arguments for %1%", mc);
            cstring name = bmv2->getRefMap().newName("fl");
            id = createFieldList(bmv2, mc->arguments->at(2), "field_lists", name, bmv2->field_lists);
        }
        auto cloneType = mc->arguments->at(0);
        auto ei = P4::EnumInstance::resolve(cloneType, &bmv2->getTypeMap());
        if (ei == nullptr) {
            ::error("%1%: must be a constant on this target", cloneType);
        } else {
            cstring prim = ei->name == "I2E" ? "clone_ingress_pkt_to_egress" :
                    "clone_egress_pkt_to_egress";
            auto session = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
            auto primitive = mkPrimitive(prim, result);
            auto parameters = mkParameters(primitive);
            parameters->append(session);

            if (id >= 0) {
                auto cst = new IR::Constant(id);
                bmv2->getTypeMap().setType(cst, IR::Type_Bits::get(32));
                auto jcst = bmv2->getExpressionConverter()->convert(cst);
                parameters->append(jcst);
            }
        }
    } else if (ef->method->name == instance.hash.name) {

        static std::set<cstring> supportedHashAlgorithms = {
            instance.algorithm.crc32.name, instance.algorithm.crc32_custom.name,
            instance.algorithm.crc16.name, instance.algorithm.crc16_custom.name,
            instance.algorithm.random.name, instance.algorithm.identity.name };

        BUG_CHECK(mc->arguments->size() == 5, "Expected 5 arguments for %1%", mc);
        auto primitive = mkPrimitive("modify_field_with_hash_based_offset", result);
        auto parameters = mkParameters(primitive);
        auto dest = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
        parameters->append(dest);
        auto base = bmv2->getExpressionConverter()->convert(mc->arguments->at(2));
        parameters->append(base);
        auto calculation = new Util::JsonObject();
        auto ei = P4::EnumInstance::resolve(mc->arguments->at(1), &bmv2->getTypeMap());
        CHECK_NULL(ei);
        if (supportedHashAlgorithms.find(ei->name) == supportedHashAlgorithms.end())
            ::error("%1%: unexpected algorithm", ei->name);
        // inlined cstring calcName = createCalculation(ei->name, mc->arguments->at(3), calculations);
        auto fields = mc->arguments->at(3);
        cstring calcName = bmv2->getRefMap().newName("calc_");
        auto calc = new Util::JsonObject();
        calc->emplace("name", calcName);
        calc->emplace("id", nextId("calculations"));
        calc->emplace("algo", ei->name);
        if (!fields->is<IR::ListExpression>()) {
            // expand it into a list
            auto list = new IR::ListExpression({});
            auto type = bmv2->getTypeMap().getType(fields, true);
            BUG_CHECK(type->is<IR::Type_StructLike>(), "%1%: expected a struct", fields);
            for (auto f : type->to<IR::Type_StructLike>()->fields) {
                auto e = new IR::Member(fields, f->name);
                auto ftype = bmv2->getTypeMap().getType(f);
                bmv2->getTypeMap().setType(e, ftype);
                list->push_back(e);
            }
            fields = list;
            bmv2->getTypeMap().setType(fields, type);
        }
        auto jright = bmv2->getExpressionConverter()->convert(fields);
        calc->emplace("input", jright);
        bmv2->calculations->append(calc);
        calculation->emplace("type", "calculation");
        calculation->emplace("value", calcName);
        parameters->append(calculation);
        auto max = bmv2->getExpressionConverter()->convert(mc->arguments->at(4));
        parameters->append(max);
    } else if (ef->method->name == instance.digest_receiver.name) {
        BUG_CHECK(mc->arguments->size() == 2, "Expected 2 arguments for %1%", mc);
        auto primitive = mkPrimitive("generate_digest", result);
        auto parameters = mkParameters(primitive);
        auto dest = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
        parameters->append(dest);
        cstring listName = "digest";
        // If we are supplied a type argument that is a named type use
        // that for the list name.
        if (mc->typeArguments->size() == 1) {
            auto typeArg = mc->typeArguments->at(0);
            if (typeArg->is<IR::Type_Name>()) {
                auto origType = bmv2->getRefMap().getDeclaration(
                    typeArg->to<IR::Type_Name>()->path, true);
                BUG_CHECK(origType->is<IR::Type_Struct>(),
                          "%1%: expected a struct type", origType);
                auto st = origType->to<IR::Type_Struct>();
                listName = extVisibleName(st);
            }
        }
        int id = createFieldList(bmv2, mc->arguments->at(1), "learn_lists",
                                 listName, bmv2->learn_lists);
        auto cst = new IR::Constant(id);
        bmv2->getTypeMap().setType(cst, IR::Type_Bits::get(32));
        auto jcst = bmv2->getExpressionConverter()->convert(cst);
        parameters->append(jcst);
    } else if (ef->method->name == instance.resubmit.name ||
               ef->method->name == instance.recirculate.name) {
        BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
        cstring prim = (ef->method->name == instance.resubmit.name) ?
                "resubmit" : "recirculate";
        auto primitive = mkPrimitive(prim, result);
        auto parameters = mkParameters(primitive);
        cstring listName = prim;
        // If we are supplied a type argument that is a named type use
        // that for the list name.
        if (mc->typeArguments->size() == 1) {
            auto typeArg = mc->typeArguments->at(0);
            if (typeArg->is<IR::Type_Name>()) {
                auto origType = bmv2->getRefMap().getDeclaration(
                    typeArg->to<IR::Type_Name>()->path, true);
                BUG_CHECK(origType->is<IR::Type_Struct>(),
                          "%1%: expected a struct type", origType);
                auto st = origType->to<IR::Type_Struct>();
                listName = extVisibleName(st);
            }
        }
        int id = createFieldList(bmv2, mc->arguments->at(0), "field_lists",
                                 listName, bmv2->field_lists);
        auto cst = new IR::Constant(id);
        bmv2->getTypeMap().setType(cst, IR::Type_Bits::get(32));
        auto jcst = bmv2->getExpressionConverter()->convert(cst);
        parameters->append(jcst);
    } else if (ef->method->name == instance.drop.name) {
        BUG_CHECK(mc->arguments->size() == 0, "Expected 0 arguments for %1%", mc);
        auto primitive = mkPrimitive("drop", result);
        (void)mkParameters(primitive);
    } else if (ef->method->name == instance.random.name) {
        BUG_CHECK(mc->arguments->size() == 3, "Expected 3 arguments for %1%", mc);
        auto primitive =
                mkPrimitive(instance.random.modify_field_rng_uniform.name, result);
        auto params = mkParameters(primitive);
        auto dest = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
        auto lo = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
        auto hi = bmv2->getExpressionConverter()->convert(mc->arguments->at(2));
        params->append(dest);
        params->append(lo);
        params->append(hi);
    } else if (ef->method->name == instance.truncate.name) {
        BUG_CHECK(mc->arguments->size() == 1, "Expected 1 arguments for %1%", mc);
        auto primitive = mkPrimitive(instance.truncate.name, result);
        auto params = mkParameters(primitive);
        auto len = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
        params->append(len);
    }

}

}
