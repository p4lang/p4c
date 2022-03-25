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
#include "ebpfPsaParser.h"
#include "backends/ebpf/ebpfType.h"
#include "frontends/p4/enumInstance.h"

namespace EBPF {

bool PsaStateTranslationVisitor::preorder(const IR::Expression* expression) {
    // Allow for friendly error name in comment before verify() call, e.g. error.NoMatch
    if (expression->is<IR::TypeNameExpression>()) {
        auto tne = expression->to<IR::TypeNameExpression>();
        builder->append(tne->typeName->path->name.name);
        return false;
    }

    return CodeGenInspector::preorder(expression);
}

bool PsaStateTranslationVisitor::preorder(const IR::Mask *mask) {
    if (currentSelectExpression == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: Masks outside of select expression are not supported.",
                mask);
        return false;
    }

    BUG_CHECK(currentSelectExpression->select->components.size() == 1,
              "%1%: tuple not eliminated in select",
              currentSelectExpression->select);

    builder->append("(");
    visit(currentSelectExpression->select->components.at(0));
    builder->append(" & ");
    visit(mask->right);
    builder->append(") == (");
    visit(mask->left);
    builder->append(" & ");
    visit(mask->right);
    builder->append(")");
    return false;
}

void PsaStateTranslationVisitor::processFunction(const P4::ExternFunction* function) {
    if (function->method->name.name == "verify") {
        compileVerify(function->expr);
        return;
    }

    StateTranslationVisitor::processFunction(function);
}

void PsaStateTranslationVisitor::processMethod(const P4::ExternMethod* ext) {
    // TODO: placeholder for handling value sets and checksums
    StateTranslationVisitor::processMethod(ext);
}

void PsaStateTranslationVisitor::compileVerify(const IR::MethodCallExpression * expression) {
    BUG_CHECK(expression->arguments->size() == 2, "Expected 2 arguments: %1%", expression);

    builder->emitIndent();
    builder->append("if (!(");
    visit(expression->arguments->at(0));
    builder->append(")) ");

    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("%s = ", parser->program->errorVar.c_str());

    auto errorType = expression->arguments->at(1)->expression;
    auto ei = P4::EnumInstance::resolve(errorType, typeMap);
    if (ei == nullptr) {
        ::error(ErrorType::ERR_MODEL, "%1%: must be a constant on this target", errorType);
        return;
    }

    builder->append(ei->name);

    builder->endOfStatement(true);

    cstring msg = Util::printf_format("Verify: condition failed, parser_error=%%u (%s)",
                                      ei->name.name);
    builder->target->emitTraceMessage(builder, msg.c_str(), 1, parser->program->errorVar.c_str());

    builder->emitIndent();
    builder->appendFormat("goto %s", IR::ParserState::reject.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

EBPFPsaParser::EBPFPsaParser(const EBPFProgram* program, const IR::ParserBlock* block,
                             const P4::TypeMap* typeMap) : EBPFParser(program, block, typeMap) {
    visitor = new PsaStateTranslationVisitor(program->refMap, program->typeMap, this);
}

void EBPFPsaParser::emitRejectState(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("if (%s == 0) ", program->errorVar.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(builder,
        "Parser: Explicit transition to reject state, dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->emitIndent();
    builder->appendFormat("goto %s", IR::ParserState::accept.c_str());
    builder->endOfStatement(true);
}
}  // namespace EBPF
