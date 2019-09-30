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

#include "ubpfControl.h"
#include "ubpfParser.h"
#include "ubpfProgram.h"
#include "ubpfType.h"

namespace UBPF {

    bool UBPFProgram::build() {
        bool success = true;
        auto pack = toplevel->getMain();
        if (pack->type->name != "ubpfFilter")
            ::warning(ErrorType::WARN_INVALID, "%1%: the main ubpf package should be called ubpfFilter"
                                               "; are you using the wrong architecture?", pack->type->name);

        if (pack->getConstructorParameters()->size() != 2) {
            ::error("Expected toplevel package %1% to have 2 parameters", pack->type);
            return false;
        }

        auto pb = pack->getParameterValue(model.filter.parser.name)
                ->to<IR::ParserBlock>();
        BUG_CHECK(pb != nullptr, "No parser block found");
        parser = new UBPFParser(this, pb, typeMap);
        success = parser->build();
        if (!success)
            return success;

        auto cb = pack->getParameterValue(model.filter.filter.name)
                ->to<IR::ControlBlock>();
        BUG_CHECK(cb != nullptr, "No control block found");
        control = new UBPFControl(this, cb, parser->headers);
        success = control->build();

        if (!success)
            return success;

        return success;
    }

    void UBPFProgram::emitC(EBPF::CodeBuilder *builder, cstring headerFile) {
        emitGeneratedComment(builder);

        builder->appendFormat("#include \"%s\"", headerFile);
        builder->newline();

        builder->target->emitIncludes(builder);
        emitPreamble(builder);
        emitUbpfHelpers(builder);

        builder->emitIndent();
        builder->target->emitMain(builder, "entry", model.CPacketName.str());
        builder->blockStart();

        emitHeaderInstances(builder);
        builder->append(" = ");
        parser->headerType->emitInitializer(builder);
        builder->endOfStatement(true);

        emitLocalVariables(builder);
        emitPacketCheck(builder);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("goto %s;", IR::ParserState::start.c_str());
        builder->newline();

        parser->emit(builder);

        emitPipeline(builder);

        builder->emitIndent();
        builder->appendFormat("if (%s)\n", control->passVariable);
        builder->increaseIndent();
        builder->emitIndent();
        builder->appendFormat("return %s;\n", builder->target->forwardReturnCode().c_str());
        builder->decreaseIndent();
        builder->emitIndent();
        builder->appendLine("else");
        builder->increaseIndent();
        builder->emitIndent();
        builder->appendFormat("return %s;\n", builder->target->dropReturnCode().c_str());
        builder->decreaseIndent();
        builder->blockEnd(true);  // end of function
    }

    void UBPFProgram::emitUbpfHelpers(EBPF::CodeBuilder *builder) const {
        builder->append(
                "static void *(*ubpf_map_lookup)(const void *, const void *) = (void *)1;\n"
                "static int (*ubpf_map_update)(void *, const void *, void *) = (void *)2;\n"
                "static int (*ubpf_map_delete)(void *, const void *) = (void *)3;\n"
                "static int (*ubpf_map_add)(void *, const void *) = (void *)4;\n"
                "static uint32_t (*ubpf_hash)(const void *, uint64_t) = (void *)6;\n"
                "static uint64_t (*ubpf_time_get_ns)() = (void *)5;\n"
                "static void (*ubpf_printf)(const char *fmt, ...) = (void *)7;\n"
                "\n");
        builder->append("static uint32_t\n"
                        "bpf_htonl(uint32_t val) {\n"
                        "    return htonl(val);\n"
                        "}");
        builder->newline();
        builder->append("static uint16_t\n"
                        "bpf_htons(uint16_t val) {\n"
                        "    return htons(val);\n"
                        "}");
        builder->newline();
        builder->append("static uint64_t\n"
                        "bpf_htonll(uint64_t val) {\n"
                        "    return htonll(val);\n"
                        "}\n");
        builder->newline();
    }

    void UBPFProgram::emitH(EBPF::CodeBuilder *builder, cstring headerFile) {
        emitGeneratedComment(builder);
        builder->appendLine("#ifndef _P4_GEN_HEADER_");
        builder->appendLine("#define _P4_GEN_HEADER_");
        builder->target->emitIncludes(builder);
        builder->newline();
        emitTypes(builder);
        builder->newline();
        emitTableDefinition(builder);
        builder->newline();
        control->emitTableTypes(builder);
        builder->newline();
        control->emitTableInstances(builder);
        builder->appendLine("#endif");
    }

    void UBPFProgram::emitPreamble(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        builder->appendLine("#define BPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)");
        builder->appendLine("#define BYTES(w) ((w) / 8)");
        builder->newline();
        builder->appendLine("void* memcpy(void* dest, const void* src, size_t num);");
        builder->newline();
    }

    void UBPFProgram::emitTypes(EBPF::CodeBuilder *builder) {
        for (auto d : program->objects) {
            if (d->is<IR::Type>() && !d->is<IR::IContainer>() &&
                !d->is<IR::Type_Extern>() && !d->is<IR::Type_Parser>() &&
                !d->is<IR::Type_Control>() && !d->is<IR::Type_Typedef>() &&
                !d->is<IR::Type_Error>()) {
                if (d->toString().startsWith("struct tuple")) {
                    continue;
                }
                CHECK_NULL(UBPFTypeFactory::instance);
                auto type = UBPFTypeFactory::instance->create(d->to<IR::Type>());
                if (type == nullptr)
                    continue;
                type->emit(builder);
                builder->newline();
            }
        }
    }

    void UBPFProgram::emitTableDefinition(EBPF::CodeBuilder *builder) const {
        //ubpf maps types
        builder->append("enum ");
        builder->append("ubpf_map_type");
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append("UBPF_MAP_TYPE_HASHMAP = 4,");
        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);

        // definition of ubpf map
        builder->append("struct ");
        builder->append("ubpf_map_def");
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append("enum ubpf_map_type type;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int key_size;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int value_size;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int max_entries;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int nb_hash_functions;");
        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    void UBPFProgram::emitHeaderInstances(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        parser->headerType->declare(builder, parser->headers->name.name, true);
    }

    void UBPFProgram::emitLocalVariables(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        builder->appendFormat("int %s = 0;", offsetVar.c_str());
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("uint8_t %s = 1;", control->passVariable);
        builder->newline();
    }

    void UBPFProgram::emitPacketCheck(EBPF::CodeBuilder* builder) {
        auto header_type = parser->headerType->to<EBPF::EBPFStructType>();
        if (header_type != nullptr) {
            auto header_type_name = header_type->name;
            builder->newline();
            builder->emitIndent();
            builder->appendFormat("if (sizeof(struct %s) < pkt_len) ", header_type_name);
            builder->blockStart();
            builder->emitIndent();
            builder->appendLine("return 0;");
            builder->blockEnd(true);
        }
    }

    void UBPFProgram::emitPipeline(EBPF::CodeBuilder *builder) {
        builder->emitIndent();
        builder->append(IR::ParserState::accept);
        builder->append(":");
        builder->newline();
        builder->emitIndent();
        builder->blockStart();
        control->emit(builder);
        builder->blockEnd(true);
    }

}