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

#include "ebpfPsaCounter.h"

#include <string>
#include <vector>

#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/psa/ebpfPipeline.h"
#include "backends/ebpf/target.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/stringify.h"

namespace EBPF {

EBPFCounterPSA::EBPFCounterPSA(const EBPFProgram *program, const IR::Declaration_Instance *di,
                               cstring name, CodeGenInspector *codeGen)
    : EBPFCounterTable(program, name, codeGen, 1, false) {
    CHECK_NULL(di);
    if (!di->type->is<IR::Type_Specialized>()) {
        ::error(ErrorType::ERR_MODEL, "Missing specialization: %1%", di);
        return;
    }
    auto ts = di->type->to<IR::Type_Specialized>();

    if (ts->baseType->toString() == "Counter") {
        isDirect = false;
    } else if (ts->baseType->toString() == "DirectCounter") {
        isDirect = true;
    } else {
        ::error(ErrorType::ERR_UNKNOWN, "Unknown counter type extern: %1%", di);
        return;
    }

    if (isDirect && di->arguments->size() != 1) {
        ::error(ErrorType::ERR_MODEL, "Expected 1 argument: %1%", di);
        return;
    } else if (!isDirect && di->arguments->size() != 2) {
        ::error(ErrorType::ERR_MODEL, "Expected 2 arguments: %1%", di);
        return;
    }

    if (isDirect && ts->arguments->size() != 1) {
        ::error(ErrorType::ERR_MODEL, "Expected a type specialized with one argument: %1%", ts);
        return;
    } else if (!isDirect && ts->arguments->size() != 2) {
        ::error(ErrorType::ERR_MODEL, "Expected a type specialized with two arguments: %1%", ts);
        return;
    }

    // check dataplane counter width
    auto dpwtype = ts->arguments->at(0);
    if (!dpwtype->is<IR::Type_Bits>()) {
        ::error(ErrorType::ERR_UNSUPPORTED, "Must be bit or int type: %1%", dpwtype);
        return;
    }

    dataplaneWidthType = EBPFTypeFactory::instance->create(dpwtype);
    unsigned dataplaneWidth = dpwtype->width_bits();
    if (dataplaneWidth > 64) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "Counters dataplane width up to 64 bits are supported: %1%", dpwtype);
        return;
    }
    if (dataplaneWidth < 8 || (dataplaneWidth & (dataplaneWidth - 1)) != 0) {
        ::warning(ErrorType::WARN_UNSUPPORTED,
                  "Counter dataplane width will be extended to "
                  "nearest type (8, 16, 32 or 64 bits): %1%",
                  dpwtype);
    }

    // By default, use BPF array map for Counter
    // TODO: add more advance logic to decide whether used map will be HASH_MAP or ARRAY_MAP
    isHash = false;

    // check index type
    indexWidthType = nullptr;
    if (!isDirect) {
        auto istype = ts->arguments->at(1);
        if (!dpwtype->is<IR::Type_Bits>()) {
            ::error(ErrorType::ERR_UNSUPPORTED, "Must be bit or int type: %1%", istype);
            return;
        }

        if (!isHash && istype->width_bits() != 32) {
            // ARRAY_MAP can have only 32 bits key, so assume this and warn user
            ::warning(ErrorType::WARN_UNSUPPORTED,
                      "Up to 32-bit type for index is supported, using 32 bit: %1%", istype);
        }

        if (isHash) {
            indexWidthType = EBPFTypeFactory::instance->create(istype);
        } else {
            keyTypeName = "u32";
        }

        auto declaredSize = di->arguments->at(0)->expression->to<IR::Constant>();
        if (!declaredSize->fitsUint()) {
            ::error(ErrorType::ERR_OVERLIMIT, "%1%: size too large", declaredSize);
            return;
        }
        size = declaredSize->asUnsigned();
    }

    auto typeArg = di->arguments->at(di->arguments->size() - 1)->expression->to<IR::Constant>();
    type = toCounterType(typeArg->asInt());
}

EBPFCounterPSA::CounterType EBPFCounterPSA::toCounterType(const int type) {
    // TODO: make use of something similar to EBPFModel to avoid hardcoded values
    if (type == 0)
        return CounterType::PACKETS;
    else if (type == 1)
        return CounterType::BYTES;
    else if (type == 2)
        return CounterType::PACKETS_AND_BYTES;

    BUG("Unknown counter type %1%", type);
}

void EBPFCounterPSA::emitTypes(CodeBuilder *builder) {
    emitKeyType(builder);
    emitValueType(builder);
}

void EBPFCounterPSA::emitKeyType(CodeBuilder *builder) {
    if (indexWidthType == nullptr) return;

    if (indexWidthType->is<EBPFStructType>()) {
        builder->emitIndent();
        indexWidthType->emit(builder);
        builder->endOfStatement(true);
    }
}

void EBPFCounterPSA::emitValueType(CodeBuilder *builder) {
    builder->emitIndent();
    builder->append("struct ");
    if (!isDirect) builder->appendFormat("%s ", valueTypeName.c_str());
    builder->blockStart();
    if (type == CounterType::BYTES || type == CounterType::PACKETS_AND_BYTES) {
        builder->emitIndent();
        dataplaneWidthType->emit(builder);
        builder->append(" bytes");
        builder->endOfStatement(true);
    }
    if (type == CounterType::PACKETS || type == CounterType::PACKETS_AND_BYTES) {
        builder->emitIndent();
        dataplaneWidthType->emit(builder);
        builder->append(" packets");
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    if (isDirect) builder->appendFormat(" %s", instanceName.c_str());
    builder->endOfStatement(true);
}

void EBPFCounterPSA::emitInstance(CodeBuilder *builder) {
    TableKind kind = isHash ? TableHash : TableArray;
    builder->target->emitTableDecl(builder, dataMapName, kind, keyTypeName,
                                   "struct " + valueTypeName, size);
}

void EBPFCounterPSA::emitMethodInvocation(CodeBuilder *builder, const P4::ExternMethod *method,
                                          CodeGenInspector *codeGen) {
    if (method->method->name.name != "count") {
        ::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1%", method->expr);
        return;
    }
    BUG_CHECK(!isDirect, "DirectCounter used outside of table");
    BUG_CHECK(method->expr->arguments->size() == 1, "Expected just 1 argument for %1%",
              method->expr);

    builder->blockStart();
    this->emitCount(builder, method->expr, codeGen);
    builder->blockEnd(false);
}

void EBPFCounterPSA::emitDirectMethodInvocation(CodeBuilder *builder,
                                                const P4::ExternMethod *method, cstring valuePtr) {
    if (method->method->name.name != "count") {
        ::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1%", method->expr);
        return;
    }
    BUG_CHECK(isDirect, "Bad Counter invocation");
    BUG_CHECK(method->expr->arguments->size() == 0, "Expected no arguments for %1%", method->expr);

    cstring target = valuePtr + "->" + instanceName;
    auto pipeline = dynamic_cast<const EBPFPipeline *>(program);
    CHECK_NULL(pipeline);

    cstring msgStr =
        Util::printf_format("Counter: updating %s, packets=1, bytes=%%u", instanceName.c_str());
    cstring varStr = Util::printf_format("%s", pipeline->lengthVar.c_str());
    builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, varStr.c_str());

    emitCounterUpdate(builder, target, "");

    msgStr = Util::printf_format("Counter: %s updated", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msgStr.c_str());
}

void EBPFCounterPSA::emitCount(CodeBuilder *builder, const IR::MethodCallExpression *expression,
                               CodeGenInspector *codeGen) {
    BUG_CHECK(codeGen != nullptr, "Index codeGen is nullptr!");
    cstring keyName = program->refMap->newName("key");
    cstring valueName = program->refMap->newName("value");
    cstring msgStr, varStr;

    builder->emitIndent();
    builder->appendFormat("struct %s *%s", valueTypeName.c_str(), valueName.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    if (indexWidthType != nullptr)
        indexWidthType->declare(builder, keyName, false);
    else
        builder->appendFormat("%s %s", keyTypeName.c_str(), keyName.c_str());
    builder->append(" = ");
    auto index = expression->arguments->at(0);
    codeGen->visit(index);
    builder->endOfStatement(true);

    msgStr = Util::printf_format("Counter: updating %s, id=%%u, packets=1, bytes=%%u",
                                 instanceName.c_str());
    varStr = Util::printf_format("%s", program->lengthVar.c_str());
    builder->target->emitTraceMessage(builder, msgStr.c_str(), 2, keyName.c_str(), varStr.c_str());

    builder->emitIndent();
    builder->target->emitTableLookup(builder, dataMapName, keyName, valueName);
    builder->endOfStatement(true);

    emitCounterUpdate(builder, valueName, keyName);

    msgStr = Util::printf_format("Counter: %s updated", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msgStr.c_str());
}

void EBPFCounterPSA::emitCounterUpdate(CodeBuilder *builder, const cstring target,
                                       const cstring keyName) {
    cstring varStr;
    cstring targetWAccess = target + (isDirect ? "." : "->");

    if (!isDirect) {
        builder->emitIndent();
        builder->appendFormat("if (%s != NULL) ", target.c_str());
        builder->blockStart();
    }

    if (type == CounterType::BYTES || type == CounterType::PACKETS_AND_BYTES) {
        builder->emitIndent();
        builder->appendFormat("__sync_fetch_and_add(&(%sbytes), %s)", targetWAccess.c_str(),
                              program->lengthVar);
        builder->endOfStatement(true);

        varStr = Util::printf_format("%sbytes", targetWAccess.c_str());
        builder->target->emitTraceMessage(builder, "Counter: now bytes=%u", 1, varStr.c_str());
    }
    if (type == CounterType::PACKETS || type == CounterType::PACKETS_AND_BYTES) {
        builder->emitIndent();
        builder->appendFormat("__sync_fetch_and_add(&(%spackets), 1)", targetWAccess.c_str());
        builder->endOfStatement(true);

        varStr = Util::printf_format("%spackets", targetWAccess.c_str());
        builder->target->emitTraceMessage(builder, "Counter: now packets=%u", 1, varStr.c_str());
    }

    if (!isDirect) {
        builder->blockEnd(false);
        builder->append(" else ");
        builder->blockStart();

        if (isHash) {
            cstring initValueName = program->refMap->newName("init_val");
            builder->target->emitTraceMessage(builder,
                                              "Counter: data not found, adding new instance");
            builder->emitIndent();
            builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), initValueName.c_str());
            emitCounterInitializer(builder);
            builder->endOfStatement(true);

            builder->emitIndent();
            builder->target->emitTableUpdate(builder, dataMapName, keyName, initValueName);
            builder->newline();
        } else {
            builder->target->emitTraceMessage(builder, "Counter: instance not found");
        }

        builder->blockEnd(true);
    }
}

void EBPFCounterPSA::emitCounterInitializer(CodeBuilder *builder) {
    builder->blockStart();
    if (type == CounterType::BYTES || type == CounterType::PACKETS_AND_BYTES) {
        builder->emitIndent();
        builder->appendFormat(".bytes = %s,", program->lengthVar.c_str());
        builder->newline();
    }
    if (type == CounterType::PACKETS || type == CounterType::PACKETS_AND_BYTES) {
        builder->emitIndent();
        builder->appendFormat(".packets = 1,");
        builder->newline();
    }
    builder->blockEnd(false);
}

}  // namespace EBPF
