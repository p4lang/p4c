/*
Copyright 2022-present Orange

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

#include "ebpfPsaDigest.h"

#include <string>
#include <vector>

#include "backends/ebpf/psa/ebpfPsaDeparser.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/stringify.h"

namespace EBPF {

class EBPFDigestPSAValueVisitor : public CodeGenInspector {
 protected:
    EBPFDigestPSA *digest;
    CodeGenInspector *codegen;
    EBPFType *valueType;

 public:
    EBPFDigestPSAValueVisitor(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, EBPFDigestPSA *digest,
                              CodeGenInspector *codegen, EBPFType *valueType)
        : CodeGenInspector(refMap, typeMap),
          digest(digest),
          codegen(codegen),
          valueType(valueType) {}

    // handle expression like: "digest.pack(msg)", where "msg" is an existing variable
    bool preorder(const IR::PathExpression *pe) override {
        digest->emitPushElement(builder, pe, codegen);
        return false;
    }

    // handle expression like: "digest.pack(metadata.msg)", where "metadata.msg" is a member from an
    // instance of struct or header
    bool preorder(const IR::Member *member) override {
        digest->emitPushElement(builder, member, codegen);
        return false;
    }

    // handle expression like: "digest.pack(value)", where "value" is compile time known constant
    // value
    bool preorder(const IR::Constant *c) override {
        cstring tmpVar = refMap->newName("digest_entry");

        builder->emitIndent();
        builder->blockStart();

        builder->emitIndent();
        valueType->declare(builder, tmpVar, false);
        builder->append(" = ");
        codegen->visit(c);
        builder->endOfStatement(true);

        digest->emitPushElement(builder, tmpVar);

        builder->blockEnd(true);
        return false;
    }

    // handle expression like: "digest.pack({ expr1, expr2, ... })", where "exprN" is an any valid
    // expression at this context
    bool preorder(const IR::StructExpression *se) override {
        cstring tmpVar = refMap->newName("digest_entry");

        builder->emitIndent();
        builder->blockStart();

        builder->emitIndent();
        valueType->declare(builder, tmpVar, false);
        builder->endOfStatement(true);

        for (const auto *f : se->components) {
            auto type = typeMap->getType(f->expression);
            cstring path = Util::printf_format("%s.%s", tmpVar, f->name.name);
            codegen->emitAssignStatement(type, nullptr, path, f->expression);
            builder->newline();
        }

        digest->emitPushElement(builder, tmpVar);

        builder->blockEnd(true);
        return false;
    }
};

EBPFDigestPSA::EBPFDigestPSA(const EBPFProgram *program, const IR::Declaration_Instance *di)
    : program(program), declaration(di) {
    instanceName = EBPFObject::externalName(di);

    auto typeSpec = declaration->type->to<IR::Type_Specialized>();
    auto messageArg = typeSpec->arguments->front();
    auto messageType = program->typeMap->getType(messageArg);
    valueType = EBPFTypeFactory::instance->create(messageType->to<IR::Type_Type>()->type);

    // if message digest type is an array underlay, it's name (like "u8 [16]") can't be simply
    // passed to map instance definition because there is defined as a pointer. This cause
    // compilation error, so in this case typedef must be used.
    if (valueType->is<EBPFScalarType>()) {
        unsigned width = valueType->to<EBPFScalarType>()->implementationWidthInBits();
        if (!EBPFScalarType::generatesScalar(width)) {
            valueTypeName = instanceName + "_value";
        }
    }
}

void EBPFDigestPSA::emitTypes(CodeBuilder *builder) {
    if (!valueTypeName.isNullOrEmpty()) {
        builder->append("typedef ");
        valueType->declare(builder, valueTypeName, false);
        builder->endOfStatement();
    }
}

void EBPFDigestPSA::emitInstance(CodeBuilder *builder) const {
    builder->appendFormat("REGISTER_TABLE_NO_KEY_TYPE(%s, BPF_MAP_TYPE_QUEUE, 0, ", instanceName);

    if (valueTypeName.isNullOrEmpty()) {
        valueType->declare(builder, "", false);
    } else {
        builder->append(valueTypeName);
    }
    builder->appendFormat(", %d)", maxDigestQueueSize);
    builder->newline();
}

void EBPFDigestPSA::processMethod(CodeBuilder *builder, cstring method,
                                  const IR::MethodCallExpression *expr,
                                  DeparserBodyTranslatorPSA *visitor) {
    if (method == "pack") {
        auto arg = expr->arguments->front();
        EBPFDigestPSAValueVisitor dg(program->refMap, program->typeMap, this, visitor, valueType);
        dg.setBuilder(builder);
        arg->apply(dg);
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: unsupported method call for Digest",
                expr->method);
    }
}

void EBPFDigestPSA::emitPushElement(CodeBuilder *builder, const IR::Expression *elem,
                                    Inspector *codegen) const {
    builder->emitIndent();
    builder->appendFormat("bpf_map_push_elem(&%s, &", instanceName);
    codegen->visit(elem);
    builder->append(", BPF_EXIST)");
    builder->endOfStatement(true);
}

void EBPFDigestPSA::emitPushElement(CodeBuilder *builder, cstring elem) const {
    builder->emitIndent();
    builder->appendFormat("bpf_map_push_elem(&%s, &%s, BPF_EXIST)", instanceName, elem);
    builder->endOfStatement(true);
}

}  // namespace EBPF
