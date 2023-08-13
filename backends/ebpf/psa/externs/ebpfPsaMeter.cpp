/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation
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
#include "ebpfPsaMeter.h"

#include <initializer_list>
#include <string>
#include <vector>

#include "backends/ebpf/ebpfControl.h"
#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/psa/ebpfPipeline.h"
#include "backends/ebpf/psa/ebpfPsaControl.h"
#include "backends/ebpf/target.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace EBPF {

EBPFMeterPSA::EBPFMeterPSA(const EBPFProgram *program, cstring instanceName,
                           const IR::Declaration_Instance *di, CodeGenInspector *codeGen)
    : EBPFTableBase(program, instanceName, codeGen) {
    CHECK_NULL(di);
    auto typeName = di->type->toString();
    if (typeName == "DirectMeter") {
        isDirect = true;
    } else if (typeName.startsWith("Meter")) {
        isDirect = false;
        auto ts = di->type->to<IR::Type_Specialized>();
        auto keyArg = ts->arguments->at(0);
        this->keyType = EBPFTypeFactory::instance->create(keyArg);

        auto declaredSize = di->arguments->at(0)->expression->to<IR::Constant>();
        if (!declaredSize->fitsUint()) {
            ::error(ErrorType::ERR_OVERLIMIT, "%1%: size too large", declaredSize);
            return;
        }
        size = declaredSize->asUnsigned();
    } else {
        ::error(ErrorType::ERR_INVALID, "Not known Meter type: %1%", di);
        return;
    }

    auto typeExpr = di->arguments->at(isDirect ? 0 : 1)->expression->to<IR::Constant>();
    this->type = toType(typeExpr->asInt());
}

EBPFType *EBPFMeterPSA::getBaseValueType(P4::ReferenceMap *refMap) {
    IR::IndexedVector<IR::StructField> vec = getValueFields();
    auto valueStructType = new IR::Type_Struct(IR::ID(getBaseStructName(refMap)), vec);
    return EBPFTypeFactory::instance->create(valueStructType);
}

EBPFType *EBPFMeterPSA::getIndirectValueType() const {
    auto vec = IR::IndexedVector<IR::StructField>();

    auto baseValue = new IR::Type_Struct(IR::ID(getBaseStructName(program->refMap)));
    vec.push_back(new IR::StructField(IR::ID(indirectValueField), baseValue));

    IR::Type_Struct *spinLock = createSpinlockStruct();
    vec.push_back(new IR::StructField(IR::ID(spinlockField), spinLock));

    auto valueType = new IR::Type_Struct(IR::ID(getIndirectStructName()), vec);
    auto meterType = EBPFTypeFactory::instance->create(valueType);

    return meterType;
}

cstring EBPFMeterPSA::getBaseStructName(P4::ReferenceMap *refMap) {
    static cstring valueBaseStructName;

    if (valueBaseStructName.isNullOrEmpty()) {
        valueBaseStructName = refMap->newName("meter_value");
    }

    return valueBaseStructName;
}

cstring EBPFMeterPSA::getIndirectStructName() const {
    static cstring valueIndirectStructName;

    if (valueIndirectStructName.isNullOrEmpty()) {
        valueIndirectStructName = program->refMap->newName("indirect_meter");
    }

    return valueIndirectStructName;
}

EBPFMeterPSA::MeterType EBPFMeterPSA::toType(const int typeCode) {
    if (typeCode == 0) {
        return PACKETS;
    } else if (typeCode == 1) {
        return BYTES;
    } else {
        BUG("Unknown meter type %1%", typeCode);
    }
}

IR::IndexedVector<IR::StructField> EBPFMeterPSA::getValueFields() {
    auto vec = IR::IndexedVector<IR::StructField>();
    auto bits_64 = IR::Type_Bits::get(64, false);
    const std::initializer_list<cstring> fieldsNames = {"pir_period", "pir_unit_per_period",
                                                        "cir_period", "cir_unit_per_period",
                                                        "pbs",        "cbs",
                                                        "pbs_left",   "cbs_left",
                                                        "time_p",     "time_c"};
    for (auto fieldName : fieldsNames) {
        vec.push_back(new IR::StructField(IR::ID(fieldName), bits_64));
    }

    return vec;
}

IR::Type_Struct *EBPFMeterPSA::createSpinlockStruct() {
    auto spinLock = new IR::Type_Struct(IR::ID("bpf_spin_lock"));
    return spinLock;
}

void EBPFMeterPSA::emitSpinLockField(CodeBuilder *builder) const {
    auto spinlockStruct = createSpinlockStruct();
    auto spinlockType = EBPFTypeFactory::instance->create(spinlockStruct);
    builder->emitIndent();
    spinlockType->declare(builder, spinlockField, false);
    builder->endOfStatement(true);
}

void EBPFMeterPSA::emitKeyType(CodeBuilder *builder) const {
    builder->emitIndent();
    builder->append("typedef ");
    this->keyType->declare(builder, keyTypeName, false);
    builder->endOfStatement(true);
}

void EBPFMeterPSA::emitValueStruct(CodeBuilder *builder, P4::ReferenceMap *refMap) {
    builder->emitIndent();
    getBaseValueType(refMap)->emit(builder);
}
void EBPFMeterPSA::emitValueType(CodeBuilder *builder) const {
    if (isDirect) {
        builder->emitIndent();
        getBaseValueType(program->refMap)->declare(builder, instanceName, false);
        builder->endOfStatement(true);
    } else {
        getIndirectValueType()->emit(builder);
    }
}

void EBPFMeterPSA::emitInstance(CodeBuilder *builder) const {
    if (!isDirect) {
        builder->target->emitTableDeclSpinlock(builder, instanceName, TableHash, this->keyTypeName,
                                               "struct " + getIndirectStructName(), size);
    } else {
        ::error(ErrorType::ERR_UNEXPECTED,
                "Direct meter belongs to table "
                "and cannot have own instance");
    }
}

void EBPFMeterPSA::emitExecute(CodeBuilder *builder, const P4::ExternMethod *method,
                               ControlBodyTranslatorPSA *translator) const {
    auto pipeline = dynamic_cast<const EBPFPipeline *>(program);
    CHECK_NULL(pipeline);

    cstring functionNameSuffix;
    if (method->expr->arguments->size() == 2) {
        functionNameSuffix = "_color_aware";
    } else {
        functionNameSuffix = "";
    }

    if (type == BYTES) {
        builder->appendFormat("meter_execute_bytes%s(&%s, &%s, ", functionNameSuffix, instanceName,
                              pipeline->lengthVar.c_str());
        this->emitIndex(builder, method, translator);
        builder->appendFormat(", &%s", pipeline->timestampVar.c_str());
    } else {
        builder->appendFormat("meter_execute_packets%s(&%s, ", functionNameSuffix, instanceName);
        this->emitIndex(builder, method, translator);
        builder->appendFormat(", &%s", pipeline->timestampVar.c_str());
    }

    if (method->expr->arguments->size() == 2) {
        builder->append(", ");
        translator->visit(method->expr->arguments->at(1));
    }
    builder->append(")");
}

void EBPFMeterPSA::emitIndex(CodeBuilder *builder, const P4::ExternMethod *method,
                             ControlBodyTranslatorPSA *translator) const {
    BUG_CHECK(translator != nullptr, "Index translator is nullptr!");
    auto argument = method->expr->arguments->at(0);
    builder->append("&");
    if (argument->expression->is<IR::Constant>()) {
        builder->append("(u32){");
        this->codeGen->visit(argument->expression);
        builder->append("}");
        return;
    }
    translator->visit(argument);
}

void EBPFMeterPSA::emitDirectExecute(CodeBuilder *builder, const P4::ExternMethod *method,
                                     cstring valuePtr) const {
    auto pipeline = dynamic_cast<const EBPFPipeline *>(program);
    CHECK_NULL(pipeline);

    cstring functionNameSuffix;
    if (method->expr->arguments->size() == 1) {
        functionNameSuffix = "_color_aware";
    } else {
        functionNameSuffix = "";
    }

    cstring lockVar = valuePtr + "->" + spinlockField;
    cstring valueMeter = valuePtr + "->" + instanceName;
    if (type == BYTES) {
        builder->appendFormat("meter_execute_bytes_value%s(&%s, &%s, &%s, &%s", functionNameSuffix,
                              valueMeter, lockVar, pipeline->lengthVar.c_str(),
                              pipeline->timestampVar.c_str());
    } else {
        builder->appendFormat("meter_execute_packets_value%s(&%s, &%s, &%s", functionNameSuffix,
                              valueMeter, lockVar, pipeline->timestampVar.c_str());
    }

    if (method->expr->arguments->size() == 1) {
        builder->append(", ");
        program->control->codeGen->visit(method->expr->arguments->at(0));
    }
    builder->append(")");
}

cstring EBPFMeterPSA::meterExecuteFunc(bool trace, P4::ReferenceMap *refMap) {
    cstring meterExecuteFunc =
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute(%meter_struct% *value, "
        "void *lock, "
        "u32 *packet_len, u64 *time_ns) {\n"
        "    if (value != NULL && value->pir_period != 0) {\n"
        "        u64 delta_p, delta_c;\n"
        "        u64 n_periods_p, n_periods_c, tokens_pbs, tokens_cbs;\n"
        "        bpf_spin_lock(lock);\n"
        "        delta_p = *time_ns - value->time_p;\n"
        "        delta_c = *time_ns - value->time_c;\n"
        "\n"
        "        n_periods_p = delta_p / value->pir_period;\n"
        "        n_periods_c = delta_c / value->cir_period;\n"
        "\n"
        "        value->time_p += n_periods_p * value->pir_period;\n"
        "        value->time_c += n_periods_c * value->cir_period;\n"
        "\n"
        "        tokens_pbs = value->pbs_left + "
        "n_periods_p * value->pir_unit_per_period;\n"
        "        if (tokens_pbs > value->pbs) {\n"
        "            tokens_pbs = value->pbs;\n"
        "        }\n"
        "        tokens_cbs = value->cbs_left + "
        "n_periods_c * value->cir_unit_per_period;\n"
        "        if (tokens_cbs > value->cbs) {\n"
        "            tokens_cbs = value->cbs;\n"
        "        }\n"
        "\n"
        "        if (*packet_len > tokens_pbs) {\n"
        "            value->pbs_left = tokens_pbs;\n"
        "            value->cbs_left = tokens_cbs;\n"
        "            bpf_spin_unlock(lock);\n"
        "%trace_msg_meter_red%"
        "            return RED;\n"
        "        }\n"
        "\n"
        "        if (*packet_len > tokens_cbs) {\n"
        "            value->pbs_left = tokens_pbs - *packet_len;\n"
        "            value->cbs_left = tokens_cbs;\n"
        "            bpf_spin_unlock(lock);\n"
        "%trace_msg_meter_yellow%"
        "            return YELLOW;\n"
        "        }\n"
        "\n"
        "        value->pbs_left = tokens_pbs - *packet_len;\n"
        "        value->cbs_left = tokens_cbs - *packet_len;\n"
        "        bpf_spin_unlock(lock);\n"
        "%trace_msg_meter_green%"
        "        return GREEN;\n"
        "    } else {\n"
        "        // From P4Runtime spec. No value - return default GREEN.\n"
        "%trace_msg_meter_no_value%"
        "        return GREEN;\n"
        "    }\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_color_aware(%meter_struct% *value, "
        "void *lock, "
        "u32 *packet_len, u64 *time_ns, enum PSA_MeterColor_t color) {\n"
        "    if (value != NULL && value->pir_period != 0) {\n"
        "        u64 delta_p, delta_c;\n"
        "        u64 n_periods_p, n_periods_c, tokens_pbs, tokens_cbs;\n"
        "        bpf_spin_lock(lock);\n"
        "        delta_p = *time_ns - value->time_p;\n"
        "        delta_c = *time_ns - value->time_c;\n"
        "\n"
        "        n_periods_p = delta_p / value->pir_period;\n"
        "        n_periods_c = delta_c / value->cir_period;\n"
        "\n"
        "        value->time_p += n_periods_p * value->pir_period;\n"
        "        value->time_c += n_periods_c * value->cir_period;\n"
        "\n"
        "        tokens_pbs = value->pbs_left + "
        "n_periods_p * value->pir_unit_per_period;\n"
        "        if (tokens_pbs > value->pbs) {\n"
        "            tokens_pbs = value->pbs;\n"
        "        }\n"
        "        tokens_cbs = value->cbs_left + "
        "n_periods_c * value->cir_unit_per_period;\n"
        "        if (tokens_cbs > value->cbs) {\n"
        "            tokens_cbs = value->cbs;\n"
        "        }\n"
        "\n"
        "        if ((color == RED) || (*packet_len > tokens_pbs)) {\n"
        "            value->pbs_left = tokens_pbs;\n"
        "            value->cbs_left = tokens_cbs;\n"
        "            bpf_spin_unlock(lock);\n"
        "%trace_msg_meter_red%"
        "            return RED;\n"
        "        }\n"
        "\n"
        "        if ((color == YELLOW) || (*packet_len > tokens_cbs)) {\n"
        "            value->pbs_left = tokens_pbs - *packet_len;\n"
        "            value->cbs_left = tokens_cbs;\n"
        "            bpf_spin_unlock(lock);\n"
        "%trace_msg_meter_yellow%"
        "            return YELLOW;\n"
        "        }\n"
        "\n"
        "        value->pbs_left = tokens_pbs - *packet_len;\n"
        "        value->cbs_left = tokens_cbs - *packet_len;\n"
        "        bpf_spin_unlock(lock);\n"
        "%trace_msg_meter_green%"
        "        return GREEN;\n"
        "    } else {\n"
        "        // From P4Runtime spec. No value - return default GREEN.\n"
        "%trace_msg_meter_no_value%"
        "        return GREEN;\n"
        "    }\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_bytes_value("
        "void *value, void *lock, u32 *packet_len, "
        "u64 *time_ns) {\n"
        "%trace_msg_meter_execute_bytes%"
        "    return meter_execute(value, lock, packet_len, time_ns);\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_bytes("
        "void *map, u32 *packet_len, void *key, u64 *time_ns) {\n"
        "    %meter_struct% *value = BPF_MAP_LOOKUP_ELEM(*map, key);\n"
        "    return meter_execute_bytes_value(value, ((void *)value) + "
        "sizeof(%meter_struct%), "
        "packet_len, time_ns);\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_packets_value("
        "void *value, void *lock, u64 *time_ns) {\n"
        "%trace_msg_meter_execute_packets%"
        "    u32 len = 1;\n"
        "    return meter_execute(value, lock, &len, time_ns);\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_packets(void *map, "
        "void *key, u64 *time_ns) {\n"
        "    %meter_struct% *value = BPF_MAP_LOOKUP_ELEM(*map, key);\n"
        "    return meter_execute_packets_value(value, ((void *)value) + "
        "sizeof(%meter_struct%), "
        "time_ns);\n"
        "}\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_bytes_value_color_aware("
        "void *value, void *lock, u32 *packet_len, "
        "u64 *time_ns, enum PSA_MeterColor_t color) {\n"
        "%trace_msg_meter_execute_bytes%"
        "    return meter_execute_color_aware(value, lock, packet_len, time_ns, color);\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_bytes_color_aware("
        "void *map, u32 *packet_len, void *key, u64 *time_ns, enum PSA_MeterColor_t color) {\n"
        "    %meter_struct% *value = BPF_MAP_LOOKUP_ELEM(*map, key);\n"
        "    return meter_execute_bytes_value_color_aware(value, ((void *)value) + "
        "sizeof(%meter_struct%), "
        "packet_len, time_ns, color);\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_packets_value_color_aware("
        "void *value, void *lock, u64 *time_ns, enum PSA_MeterColor_t color) {\n"
        "%trace_msg_meter_execute_packets%"
        "    u32 len = 1;\n"
        "    return meter_execute_color_aware(value, lock, &len, time_ns, color);\n"
        "}\n"
        "\n"
        "static __always_inline\n"
        "enum PSA_MeterColor_t meter_execute_packets_color_aware(void *map, "
        "void *key, u64 *time_ns, enum PSA_MeterColor_t color) {\n"
        "    %meter_struct% *value = BPF_MAP_LOOKUP_ELEM(*map, key);\n"
        "    return meter_execute_packets_value_color_aware(value, ((void *)value) + "
        "sizeof(%meter_struct%), "
        "time_ns, color);\n"
        "}\n";

    if (trace) {
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_green%"),
                                                    "        bpf_trace_message(\""
                                                    "Meter: GREEN\\n\");\n");
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_yellow%"),
                                                    "            bpf_trace_message(\""
                                                    "Meter: YELLOW\\n\");\n");
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_red%"),
                                                    "            bpf_trace_message(\""
                                                    "Meter: RED\\n\");\n");
        meterExecuteFunc =
            meterExecuteFunc.replace(cstring("%trace_msg_meter_no_value%"),
                                     "        bpf_trace_message(\"Meter: No meter value! "
                                     "Returning default GREEN\\n\");\n");
        meterExecuteFunc =
            meterExecuteFunc.replace(cstring("%trace_msg_meter_execute_bytes%"),
                                     "    bpf_trace_message(\"Meter: execute BYTES\\n\");\n");
        meterExecuteFunc =
            meterExecuteFunc.replace(cstring("%trace_msg_meter_execute_packets%"),
                                     "    bpf_trace_message(\"Meter: execute PACKETS\\n\");\n");
    } else {
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_green%"), "");
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_yellow%"), "");
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_red%"), "");
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_no_value%"), "");
        meterExecuteFunc = meterExecuteFunc.replace(cstring("%trace_msg_meter_execute_bytes%"), "");
        meterExecuteFunc =
            meterExecuteFunc.replace(cstring("%trace_msg_meter_execute_packets%"), "");
    }

    meterExecuteFunc = meterExecuteFunc.replace(cstring("%meter_struct%"),
                                                cstring("struct ") + getBaseStructName(refMap));

    return meterExecuteFunc;
}

}  // namespace EBPF
