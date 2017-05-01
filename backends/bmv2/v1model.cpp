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

#include <p4includes/v1model.h>
#include <backends/bmv2/backend.h>

namespace P4V1 {

void V1Model::convertExternObject(Util::JsonObject *o, BMV2::Backend *bmv2,
                                  P4::ExternMethod *em, IR::MethodCallStatement *mc)
{
    if (em->originalExternType->name == v1model.counter.name) {
        if (em->method->name == v1model.counter.increment.name) {
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
    } else if (em->originalExternType->name == v1model.meter.name) {
        if (em->method->name == v1model.meter.executeMeter.name) {
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
    } else if (em->originalExternType->name == v1model.registers.name) {
        BUG_CHECK(mc->arguments->size() == 2, "Expected 2 arguments for %1%", mc);
        auto reg = new Util::JsonObject();
        reg->emplace("type", "register_array");
        cstring name = extVisibleName(em->object);
        reg->emplace("value", name);
        if (em->method->name == v1model.registers.read.name) {
            auto primitive = mkPrimitive("register_read", result);
            auto parameters = mkParameters(primitive);
            auto dest = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
            parameters->append(dest);
            parameters->append(reg);
            auto index = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
            parameters->append(index);
        } else if (em->method->name == v1model.registers.write.name) {
            auto primitive = mkPrimitive("register_write", result);
            auto parameters = mkParameters(primitive);
            parameters->append(reg);
            auto index = bmv2->getExpressionConverter()->convert(mc->arguments->at(0));
            parameters->append(index);
            auto value = bmv2->getExpressionConverter()->convert(mc->arguments->at(1));
            parameters->append(value);
        }
    } else if (em->originalExternType->name == v1model.directMeter.name) {
        if (em->method->name == v1model.directMeter.read.name) {
            BUG_CHECK(mc->arguments->size() == 1, "Expected 1 argument for %1%", mc);
            auto dest = mc->arguments->at(0);
            bmv2->getMeterMap().setDestination(em->object, dest);
            // Do not generate any code for this operation
        }
    } else if (em->originalExternType->name == v1model.directCounter.name) {
        if (em->method->name == v1model.directCounter.count.name) {
            BUG_CHECK(mc->arguments->size() == 0, "Expected 0 argument for %1%", mc);
            // Do not generate any code for this operation
        }
    }

}

void V1Model::convertExternFunctions(Util::JsonObject *o, BMV2::Backend *bmv2,
                                     P4::ExternMethod *em, IR::MethodCallStatement *mc){

}

}