#include "tcExterns.h"

namespace TC {

void EBPFRegisterPNA::emitInitializer(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                      ControlBodyTranslatorPNA *translator) {
    auto externName = method->originalExternType->name.name;
    auto index = method->expr->arguments->at(0)->expression;
    builder->emitIndent();
    builder->appendLine("__builtin_memset(ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params->pipe_id = p4tc_filter_fields.pipeid;");
    builder->emitIndent();
    auto extId = translator->tcIR->getExternId(externName);
    BUG_CHECK(extId != 0, "Extern ID not found");
    builder->appendFormat("ext_params->ext_id = %d;", extId);
    builder->newline();
    builder->emitIndent();
    auto instId = translator->tcIR->getExternInstanceId(externName, this->instanceName);
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params->inst_id = %d;", instId);
    builder->newline();
    builder->emitIndent();
    builder->append("ext_params->index = ");
    translator->visit(index);
    builder->endOfStatement(true);
}

void EBPFRegisterPNA::emitRegisterRead(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                       ControlBodyTranslatorPNA *translator,
                                       const IR::Expression *leftExpression) {
    emitInitializer(builder, method, translator);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("ext_val = bpf_p4tc_extern_md_read(skb, ext_params, sizeof(ext_params));");
    builder->emitIndent();
    builder->appendLine("if (!ext_val) ");
    builder->emitIndent();
    builder->appendFormat("     return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    if (leftExpression != nullptr) {
        builder->emitIndent();
        builder->append("__builtin_memcpy(&");
        translator->visit(leftExpression);
        builder->append(", ext_val->out_params, sizeof(");
        this->valueType->declare(builder, cstring::empty, false);
        builder->append("));");
    }
}

void EBPFRegisterPNA::emitRegisterWrite(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                        ControlBodyTranslatorPNA *translator) {
    auto value = method->expr->arguments->at(1)->expression;
    emitInitializer(builder, method, translator);

    builder->newline();
    builder->emitIndent();
    builder->append("__builtin_memcpy(ext_val->out_params, &");
    translator->visit(value);
    builder->append(", sizeof(");
    this->valueType->declare(builder, cstring::empty, false);
    builder->append("));");
    builder->newline();
    builder->emitIndent();
    builder->appendLine(
        "bpf_p4tc_extern_md_write(skb, ext_params, sizeof(ext_params), ext_val, sizeof(ext_val));");
}

}  // namespace TC
