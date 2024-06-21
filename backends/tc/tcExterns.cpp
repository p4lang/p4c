/*
Copyright (C) 2024 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include "tcExterns.h"

namespace TC {

void EBPFRegisterPNA::emitInitializer(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                      ControlBodyTranslatorPNA *translator) {
    auto externName = method->originalExternType->name.name;
    auto index = method->expr->arguments->at(0)->expression;
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    builder->emitIndent();
    auto extId = translator->tcIR->getExternId(externName);
    BUG_CHECK(!extId.isNullOrEmpty(), "Extern ID not found");
    builder->appendFormat("ext_params.ext_id = %s;", extId);
    builder->newline();
    builder->emitIndent();
    auto instId = translator->tcIR->getExternInstanceId(externName, this->instanceName);
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params.inst_id = %d;", instId);
    builder->newline();
    builder->emitIndent();
    builder->append("ext_params.index = ");
    translator->visit(index);
    builder->endOfStatement(true);
}

void EBPFRegisterPNA::emitRegisterRead(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                       ControlBodyTranslatorPNA *translator,
                                       const IR::Expression *leftExpression) {
    emitInitializer(builder, method, translator);
    builder->newline();
    builder->emitIndent();
    builder->appendLine(
        "ext_val_ptr = bpf_p4tc_extern_md_read(skb, &ext_params, sizeof(ext_params));");
    builder->emitIndent();
    builder->appendLine("if (!ext_val_ptr) ");
    builder->emitIndent();
    builder->appendFormat("     return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->appendLine("ext_val = *ext_val_ptr;");
    if (leftExpression != nullptr) {
        builder->emitIndent();
        builder->append("__builtin_memcpy(&");
        translator->visit(leftExpression);
        builder->append(", ext_val.out_params, sizeof(");
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
    builder->append("__builtin_memcpy(ext_val.out_params, &");
    translator->visit(value);
    builder->append(", sizeof(");
    this->valueType->declare(builder, cstring::empty, false);
    builder->append("));");
    builder->newline();
    builder->emitIndent();
    builder->appendLine(
        "bpf_p4tc_extern_md_write(skb, &ext_params, sizeof(ext_params), &ext_val, "
        "sizeof(ext_val));");
}

void EBPFCounterPNA::emitDirectMethodInvocation(EBPF::CodeBuilder *builder,
                                                const P4::ExternMethod *method,
                                                const ConvertToBackendIR *tcIR) {
    if (method->method->name.name != "count") {
        ::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1%", method->expr);
        return;
    }
    BUG_CHECK(isDirect, "Bad Counter invocation");
    BUG_CHECK(method->expr->arguments->size() == 0, "Expected no arguments for %1%", method->expr);
    emitCounterUpdate(builder, tcIR);
}

void EBPFCounterPNA::emitCounterUpdate(EBPF::CodeBuilder *builder, const ConvertToBackendIR *tcIR) {
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    auto tblId = tcIR->getTableId(tblname);
    BUG_CHECK(tblId != 0, "Table ID not found");
    builder->emitIndent();
    builder->appendFormat("ext_params.tbl_id = %d;", tblId);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("ext_params.flags = P4TC_EXT_CNT_DIRECT;");
    builder->emitIndent();

    if (type == CounterType::BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_bytes(skb, &ext_params, sizeof(ext_params), &key, "
            "sizeof(key))");
    } else if (type == CounterType::PACKETS) {
        builder->append(
            "bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), &key, "
            "sizeof(key))");
    } else if (type == CounterType::PACKETS_AND_BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_pktsnbytes(skb, &ext_params, sizeof(ext_params), &key, "
            "sizeof(key))");
    }
}

void EBPFCounterPNA::emitMethodInvocation(EBPF::CodeBuilder *builder,
                                          const P4::ExternMethod *method,
                                          ControlBodyTranslatorPNA *translator) {
    if (method->method->name.name != "count") {
        ::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1%", method->expr);
        return;
    }
    BUG_CHECK(!isDirect, "DirectCounter used outside of table");
    BUG_CHECK(method->expr->arguments->size() == 1, "Expected just 1 argument for %1%",
              method->expr);
    emitCount(builder, method, translator);
}

void EBPFCounterPNA::emitCount(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                               ControlBodyTranslatorPNA *translator) {
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    auto externName = method->originalExternType->name.name;
    auto instanceName = di->toString();
    auto index = method->expr->arguments->at(0)->expression;
    builder->emitIndent();
    auto extId = translator->tcIR->getExternId(externName);
    BUG_CHECK(!extId.isNullOrEmpty(), "Extern ID not found");
    builder->appendFormat("ext_params.ext_id = %s;", extId);
    builder->newline();
    builder->emitIndent();
    auto instId = translator->tcIR->getExternInstanceId(externName, instanceName);
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params.inst_id = %d;", instId);
    builder->newline();
    builder->emitIndent();
    builder->append("ext_params.index = ");
    translator->visit(index);
    builder->endOfStatement(true);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("ext_params.flags = P4TC_EXT_CNT_INDIRECT;");
    builder->emitIndent();

    if (type == CounterType::BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_bytes(skb, &ext_params, sizeof(ext_params), NULL, 0)");
    } else if (type == CounterType::PACKETS) {
        builder->append(
            "bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0)");
    } else if (type == CounterType::PACKETS_AND_BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_pktsnbytes(skb, &ext_params, sizeof(ext_params), NULL, 0)");
    }
}

}  // namespace TC
