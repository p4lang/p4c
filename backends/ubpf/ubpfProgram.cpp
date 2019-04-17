//#include "backends/ebpf/ebpfType.h"
//#include "backends/ebpf/ebpfControl.h"
#include "ubpfControl.h"
#include "ubpfParser.h"
#include "ubpfProgram.h"
#include "ubpfType.h"

namespace UBPF {

    bool UBPFProgram::build() {
        bool success = true;
        auto pack = toplevel->getMain();
        if (pack->type->name != "Filter")
            ::warning(ErrorType::WARN_INVALID, "%1%: the main ebpf package should be called ebpfFilter"
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

        printf("Parser przeszedł");

        auto cb = pack->getParameterValue(model.filter.filter.name)
                ->to<IR::ControlBlock>();
        BUG_CHECK(cb != nullptr, "No control block found");
        control = new UBPFControl(this, cb, parser->headers);
        success = control->build();

        printf("Control przeszedł");

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
        builder->target->emitMain(builder, "entry", "pkt");
        builder->blockStart();

        emitHeaderInstances(builder);
        builder->append(" = ");
        parser->headerType->emitInitializer(builder);
        builder->endOfStatement(true);

        emitLocalVariables(builder);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("goto %s;", IR::ParserState::start.c_str());
        builder->newline();

        parser->emit(builder);

        emitPipeline(builder);

        builder->emitIndent();
        builder->appendFormat("if (%s)\n", control->accept->name.name.c_str());
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
//
//        builder->emitIndent();
//        builder->appendFormat("return %s;\n", builder->target->forwardReturnCode().c_str());
//        builder->blockEnd(true);
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
    }

    void UBPFProgram::emitH(EBPF::CodeBuilder *builder, cstring headerFile) {
        emitGeneratedComment(builder);
        builder->appendLine("#ifndef _P4_GEN_HEADER_");
        builder->appendLine("#define _P4_GEN_HEADER_");
        builder->target->emitIncludes(builder);
        builder->newline();
        emitTypes(builder);
        builder->newline();
        builder->appendLine("#endif");
    }

    void UBPFProgram::emitPreamble(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        builder->appendLine("#define BPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)");
        builder->appendLine("#define BYTES(w) ((w) / 8)");
        builder->newline();
    }

    void UBPFProgram::emitTypes(EBPF::CodeBuilder *builder) {
        std::cout << "Emitting Types." << std::endl;
        for (auto d : program->objects) {
            if (d->is<IR::Type>() && !d->is<IR::IContainer>() &&
                !d->is<IR::Type_Extern>() && !d->is<IR::Type_Parser>() &&
                !d->is<IR::Type_Control>() && !d->is<IR::Type_Typedef>() &&
                !d->is<IR::Type_Error>()) {
                std::cout << "Creating instance." << std::endl;
                CHECK_NULL(UBPFTypeFactory::instance);
                auto type = UBPFTypeFactory::instance->create(d->to<IR::Type>());
                if (type == nullptr)
                    continue;
                type->emit(builder);
                builder->newline();
            }
        }
    }

    void UBPFProgram::emitHeaderInstances(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        parser->headerType->declare(builder, parser->headers->name.name, false);
    }

    void UBPFProgram::emitLocalVariables(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        builder->appendFormat("int %s = 0;", offsetVar.c_str());
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("u8 %s = 0;", control->accept->name.name.c_str());
        builder->newline();

//        builder->emitIndent();
//        builder->appendFormat("u8 pass = 0;", offsetVar.c_str());
//        builder->newline();
    }

    void UBPFProgram::emitPipeline(EBPF::CodeBuilder *builder) {
        printf("Wszedłem do pipeline");
        builder->emitIndent();
        builder->append(IR::ParserState::accept);
        builder->append(":");
        builder->newline();
        printf("Blok powinien się otworzyć");
        builder->emitIndent();
        builder->blockStart();
        printf("Przed emit");
        control->emit(builder);
        printf("Po emit");
        builder->blockEnd(true);
    }

}