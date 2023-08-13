/*
Copyright 2019 Orange

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

#include "ubpfDeparser.h"

#include <map>
#include <string>
#include <vector>

#include "backends/ubpf/ubpfProgram.h"
#include "frontends/common/model.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "lib/algorithm.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/map.h"
#include "lib/null.h"
#include "ubpfType.h"

namespace UBPF {

class OutHeaderSize final : public EBPF::CodeGenInspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const UBPFProgram *program;

    std::map<const IR::Parameter *, const IR::Parameter *> substitution;

    bool illegal(const IR::Statement *statement) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: not supported in deparser", statement);
        return false;
    }

 public:
    OutHeaderSize(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, const UBPFProgram *program)
        : EBPF::CodeGenInspector(refMap, typeMap),
          refMap(refMap),
          typeMap(typeMap),
          program(program) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(program);
        setName("OutHeaderSize");
    }
    bool preorder(const IR::PathExpression *expression) override {
        auto decl = refMap->getDeclaration(expression->path, true);
        auto param = decl->getNode()->to<IR::Parameter>();
        if (param != nullptr) {
            auto subst = ::get(substitution, param);
            if (subst != nullptr) {
                builder->append(subst->name);
                return false;
            }
        }
        builder->append(expression->path->name);
        return false;
    }
    bool preorder(const IR::SwitchStatement *statement) override { return illegal(statement); }
    bool preorder(const IR::IfStatement *statement) override { return illegal(statement); }
    bool preorder(const IR::AssignmentStatement *statement) override { return illegal(statement); }
    bool preorder(const IR::ReturnStatement *statement) override { return illegal(statement); }
    bool preorder(const IR::ExitStatement *statement) override { return illegal(statement); }
    bool preorder(const IR::MethodCallStatement *statement) override {
        LOG5("Calculate OutHeaderSize");
        auto &p4lib = P4::P4CoreLibrary::instance();

        auto mi = P4::MethodInstance::resolve(statement->methodCall, refMap, typeMap);
        auto method = mi->to<P4::ExternMethod>();
        if (method == nullptr) return illegal(statement);

        auto declType = method->originalExternType;
        if (declType->name.name != p4lib.packetOut.name ||
            method->method->name.name != p4lib.packetOut.emit.name ||
            method->expr->arguments->size() != 1) {
            return illegal(statement);
        }

        auto h = method->expr->arguments->at(0);
        auto type = typeMap->getType(h);
        auto ht = type->to<IR::Type_Header>();
        if (ht == nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Cannot emit a non-header type %1%", h);
            return false;
        }
        unsigned width = ht->width_bits();

        builder->append("if (");
        visit(h);
        builder->append(".ebpf_valid) ");
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("%s += %d;", program->outerHdrLengthVar.c_str(), width);
        builder->newline();
        return false;
    }

    void substitute(const IR::Parameter *p, const IR::Parameter *with) {
        substitution.emplace(p, with);
    }
};

UBPFDeparserTranslationVisitor::UBPFDeparserTranslationVisitor(const UBPFDeparser *deparser)
    : CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
      deparser(deparser),
      p4lib(P4::P4CoreLibrary::instance()) {
    setName("UBPFDeparserTranslationVisitor");
}

void UBPFDeparserTranslationVisitor::compileEmitField(const IR::Expression *expr, cstring field,
                                                      unsigned alignment, EBPF::EBPFType *type) {
    auto et = dynamic_cast<EBPF::IHasWidth *>(type);
    if (et == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "Only headers with fixed widths supported %1%", expr);
        return;
    }

    unsigned widthToEmit = et->widthInBits();

    unsigned loadSize = 0;
    cstring swap = "";
    if (widthToEmit <= 8) {
        loadSize = 8;
    } else if (widthToEmit <= 16) {
        swap = "bpf_htons";
        loadSize = 16;
    } else if (widthToEmit <= 32) {
        swap = "htonl";
        loadSize = 32;
    } else if (widthToEmit <= 64) {
        swap = "htonll";
        loadSize = 64;
    }
    unsigned bytes = ROUNDUP(widthToEmit, 8);
    unsigned shift =
        widthToEmit < 8 ? (loadSize - alignment - widthToEmit) : (loadSize - widthToEmit);

    if (!swap.isNullOrEmpty()) {
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s = %s(", field.c_str(), swap);
        visit(expr);
        builder->appendFormat(".%s", field.c_str());
        if (shift != 0) builder->appendFormat(" << %d", shift);
        builder->append(")");
        builder->endOfStatement(true);
    }

    auto program = deparser->program;
    unsigned bitsInFirstByte = widthToEmit % 8;
    if (bitsInFirstByte == 0) bitsInFirstByte = 8;
    unsigned bitsInCurrentByte = bitsInFirstByte;
    unsigned left = widthToEmit;

    for (unsigned i = 0; i < (widthToEmit + 7) / 8; i++) {
        builder->emitIndent();
        builder->appendFormat("%s = ((char*)(&", program->byteVar.c_str());
        visit(expr);
        builder->appendFormat(".%s))[%d]", field.c_str(), i);
        builder->endOfStatement(true);

        unsigned freeBits = alignment != 0 ? (8 - alignment) : 8;
        bitsInCurrentByte = left >= 8 ? 8 : left;
        unsigned bitsToWrite = bitsInCurrentByte > freeBits ? freeBits : bitsInCurrentByte;
        BUG_CHECK((bitsToWrite > 0) && (bitsToWrite <= 8), "invalid bitsToWrite %d", bitsToWrite);
        builder->emitIndent();
        if (alignment == 0 && bitsToWrite == 8) {  // write whole byte
            builder->appendFormat(
                "write_byte(%s, BYTES(%s) + %d, (%s))", program->packetStartVar.c_str(),
                program->offsetVar.c_str(),
                widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                program->byteVar.c_str());
        } else {  // write partial
            shift = (8 - alignment - bitsToWrite);
            builder->appendFormat(
                "write_partial(%s + BYTES(%s) + %d, %d, %d, (%s >> %d))",
                program->packetStartVar.c_str(), program->offsetVar.c_str(),
                widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                bitsToWrite, shift, program->byteVar.c_str(),
                widthToEmit > freeBits ? alignment == 0 ? shift : alignment : 0);
        }
        builder->endOfStatement(true);
        left -= bitsToWrite;
        bitsInCurrentByte -= bitsToWrite;
        alignment = (alignment + bitsToWrite) % 8;
        bitsToWrite = (8 - bitsToWrite);

        if (bitsInCurrentByte > 0) {
            builder->emitIndent();
            if (bitsToWrite == 8) {
                builder->appendFormat(
                    "write_byte(%s, BYTES(%s) + %d + 1, (%s << %d))",
                    program->packetStartVar.c_str(), program->offsetVar.c_str(),
                    widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                    program->byteVar.c_str(), 8 - alignment % 8);
            } else {
                builder->appendFormat(
                    "write_partial(%s + BYTES(%s) + %d + 1, %d, %d, (%s))",
                    program->packetStartVar.c_str(), program->offsetVar.c_str(),
                    widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                    bitsToWrite, 8 + alignment - bitsToWrite, program->byteVar.c_str());
            }
            builder->endOfStatement(true);
            left -= bitsToWrite;
        }
        alignment = (alignment + bitsToWrite) % 8;
    }

    builder->emitIndent();
    builder->appendFormat("%s += %d", program->offsetVar.c_str(), widthToEmit);
    builder->endOfStatement(true);
    builder->newline();
}

void UBPFDeparserTranslationVisitor::compileEmit(const IR::Vector<IR::Argument> *args) {
    BUG_CHECK(args->size() == 1, "%1%: expected 1 argument for emit", args);

    auto expr = args->at(0)->expression;
    auto type = typeMap->getType(expr);
    auto ht = type->to<IR::Type_Header>();
    if (ht == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Cannot emit a non-header type %1%", expr);
        return;
    }

    builder->emitIndent();
    builder->append("if (");
    visit(expr);
    builder->append(".ebpf_valid) ");
    builder->blockStart();

    auto program = deparser->program;
    unsigned width = ht->width_bits();
    builder->emitIndent();
    builder->appendFormat("if (%s < BYTES(%s + %d)) ", program->lengthVar.c_str(),
                          program->offsetVar.c_str(), width);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    builder->blockEnd(true);

    builder->emitIndent();
    builder->newline();

    unsigned alignment = 0;
    for (auto f : ht->fields) {
        auto ftype = typeMap->getType(f);
        auto etype = UBPFTypeFactory::instance->create(ftype);
        auto et = dynamic_cast<EBPF::IHasWidth *>(etype);
        if (et == nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Only headers with fixed widths supported %1%", f);
            return;
        }
        compileEmitField(expr, f->name, alignment, etype);
        alignment += et->widthInBits();
        alignment %= 8;
    }

    builder->blockEnd(true);
}

bool UBPFDeparserTranslationVisitor::preorder(const IR::MethodCallExpression *expression) {
    auto mi = P4::MethodInstance::resolve(expression, deparser->program->refMap,
                                          deparser->program->typeMap);
    auto extMethod = mi->to<P4::ExternMethod>();

    if (extMethod != nullptr) {
        auto decl = extMethod->object;
        if (decl == deparser->packet_out) {
            if (extMethod->method->name.name == p4lib.packetOut.emit.name) {
                compileEmit(extMethod->expr->arguments);
                return false;
            }
        }
    }

    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected method call in deparser %1%", expression);
    return false;
}

bool UBPFDeparser::build() {
    auto pl = controlBlock->container->type->applyParams;
    if (pl->size() != 2) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: Expected deparser to have exactly 2 parameters",
                controlBlock->getNode());
        return false;
    }

    auto it = pl->parameters.begin();
    packet_out = *it;
    ++it;
    headers = *it;

    codeGen = new UBPFDeparserTranslationVisitor(this);
    codeGen->substitute(headers, parserHeaders);

    return ::errorCount() == 0;
}

void UBPFDeparser::emit(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("int %s = 0", program->outerHdrLengthVar.c_str());
    builder->endOfStatement(true);

    auto ohs = new OutHeaderSize(program->refMap, program->typeMap,
                                 static_cast<const UBPFProgram *>(program));
    ohs->substitute(headers, parserHeaders);
    ohs->setBuilder(builder);

    builder->emitIndent();
    controlBlock->container->body->apply(*ohs);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("int %s = BYTES(%s) - BYTES(%s)", program->outerHdrOffsetVar.c_str(),
                          program->outerHdrLengthVar.c_str(), program->offsetVar.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("%s = ubpf_adjust_head(%s, %s)", program->packetStartVar.c_str(),
                          program->contextVar.c_str(), program->outerHdrOffsetVar.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("%s += %s", program->lengthVar.c_str(),
                          program->outerHdrOffsetVar.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("%s = 0", program->offsetVar.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s > 0) ", program->packetTruncatedSizeVar.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s -= ubpf_truncate_packet(%s, %s)", program->lengthVar.c_str(),
                          program->contextVar.c_str(), program->packetTruncatedSizeVar.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
    builder->emitIndent();
    builder->newline();

    codeGen->setBuilder(builder);
    controlBlock->container->body->apply(*codeGen);
}
}  // namespace UBPF
