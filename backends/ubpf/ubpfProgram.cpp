#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/ebpfControl.h"
#include "backends/ebpf/ebpfParser.h"
#include "ubpfProgram.h"


namespace UBPF {

    bool UbpfProgram::build() {
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

        //TODO: Implement UBPF parser
        parser = new EBPF::EBPFParser(this, pb, typeMap);
        success = parser->build();
        if (!success)
            return success;

        auto cb = pack->getParameterValue(model.filter.filter.name)
                ->to<IR::ControlBlock>();
        BUG_CHECK(cb != nullptr, "No control block found");
        control = new EBPF::EBPFControl(this, cb, parser->headers);
        success = control->build();
        if (!success)
            return success;

        return success;
    }

    void UbpfProgram::emitC(EBPF::CodeBuilder *builder, cstring headerFile) {
        emitGeneratedComment(builder);

        builder->appendFormat("#include \"%s\"", headerFile);
        builder->newline();

        builder->target->emitIncludes(builder);


    }

    void UbpfProgram::emitH(EBPF::CodeBuilder *builder, cstring headerFile) {
        emitGeneratedComment(builder);
        builder->appendLine("#ifndef _P4_GEN_HEADER_");
        builder->appendLine("#define _P4_GEN_HEADER_");
        builder->target->emitIncludes(builder);
        builder->newline();
        emitTypes(builder);
        builder->newline();
        builder->appendLine("#endif");
        builder->appendLine("#endif");
    }

    void UbpfProgram::emitTypes(CodeBuilder *builder) {

    }

}