/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "ebpfProgram.h"

#include <chrono>
#include <ctime>

#include "ebpfControl.h"
#include "ebpfDeparser.h"
#include "ebpfParser.h"
#include "ebpfTable.h"
#include "ebpfType.h"
#include "frontends/common/options.h"
#include "frontends/p4/coreLibrary.h"

namespace EBPF {

bool EBPFProgram::build() {
    auto pack = toplevel->getMain();
    if (pack->type->name == "xdp") {
        if (pack->getConstructorParameters()->size() != 3) {
            ::error(ErrorType::ERR_EXPECTED,
                    "Expected toplevel xdp package %1% to have 3 parameters", pack->type);
            return false;
        }
        model.arch = ModelArchitecture::XdpSwitch;
        progTarget = new XdpTarget(options.emitTraceMessages);
    } else {
        if (pack->type->name != "ebpfFilter")
            ::warning(ErrorType::WARN_INVALID,
                      "%1%: the main ebpf package should be called ebpfFilter or xdp"
                      "; are you using the wrong architecture?",
                      pack->type->name);

        if (pack->getConstructorParameters()->size() != 2) {
            ::error(ErrorType::ERR_EXPECTED,
                    "Expected toplevel ebpfFilter package %1% to have 2 parameters", pack->type);
            return false;
        }
        model.arch = ModelArchitecture::EbpfFilter;
    }

    auto prsName = (model.arch == ModelArchitecture::XdpSwitch) ? model.xdp.parser.name
                                                                : model.filter.parser.name;
    auto ctlName = (model.arch == ModelArchitecture::XdpSwitch) ? model.xdp.switch_.name
                                                                : model.filter.filter.name;

    auto pb = pack->getParameterValue(prsName)->to<IR::ParserBlock>();
    BUG_CHECK(pb != nullptr, "No parser block found");
    parser = new EBPFParser(this, pb, typeMap);
    bool success = parser->build();
    if (!success) return success;

    auto cb = pack->getParameterValue(ctlName)->to<IR::ControlBlock>();
    BUG_CHECK(cb != nullptr, "No control block found");
    control = new EBPFControl(this, cb, parser->headers);
    success = control->build();
    if (!success) return success;

    if (model.arch == ModelArchitecture::XdpSwitch) {
        auto db = pack->getParameterValue(model.xdp.deparser.name)->to<IR::ControlBlock>();
        BUG_CHECK(db != nullptr, "No deparser block found");
        deparser = new EBPFDeparser(this, db, parser->headers);
        bool success = deparser->build();
        if (!success) return success;
    }

    return true;
}

void EBPFProgram::emitC(CodeBuilder *builder, cstring header) {
    emitGeneratedComment(builder);

    // Find the last occurrence of a folder slash (Linux only)
    const char *header_stripped = header.findlast('/');
    if (header_stripped)
        // Remove the path from the header
        builder->appendFormat("#include \"%s\"", header_stripped + 1);
    else
        // There is no prepended path, just include the header
        builder->appendFormat("#include \"%s\"", header.c_str());
    builder->newline();

    builder->target->emitIncludes(builder);
    emitPreamble(builder);
    builder->append("REGISTER_START()\n");
    control->emitTableInstances(builder);
    parser->emitValueSetInstances(builder);
    builder->append("REGISTER_END()\n");
    builder->newline();
    builder->emitIndent();
    // Use different section name for XDP - this is used by the runtime test framework.
    if (model.arch == ModelArchitecture::XdpSwitch)
        builder->target->emitCodeSection(builder, "xdp");
    else
        builder->target->emitCodeSection(builder, "prog");
    builder->emitIndent();
    builder->target->emitMain(builder, functionName, model.CPacketName.str());
    builder->blockStart();

    emitHeaderInstances(builder);
    builder->append(" = ");
    parser->headerType->emitInitializer(builder);
    builder->endOfStatement(true);

    emitLocalVariables(builder);
    builder->newline();

    parser->emit(builder);
    emitPipeline(builder);

    builder->emitIndent();
    builder->appendFormat("%s:\n", endLabel.c_str());
    builder->emitIndent();
    if (model.arch == ModelArchitecture::EbpfFilter) {
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
    } else if (model.arch == ModelArchitecture::XdpSwitch) {
        builder->append("return omd.output_action;");
        builder->newline();
    } else {
        BUG("Invalid value for model.arch !");
    }
    builder->blockEnd(true);  // end of function

    builder->target->emitLicense(builder, license);
}

void EBPFProgram::emitGeneratedComment(CodeBuilder *builder) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    builder->append("/* Automatically generated by ");
    builder->append(options.exe_name);
    builder->append(" from ");
    builder->append(options.file);
    builder->append(" on ");
    builder->append(std::ctime(&time));
    builder->append(" */");
    builder->newline();
}

void EBPFProgram::emitH(CodeBuilder *builder, cstring) {
    emitGeneratedComment(builder);
    builder->appendLine("#ifndef _P4_GEN_HEADER_");
    builder->appendLine("#define _P4_GEN_HEADER_");
    builder->target->emitIncludes(builder);
    builder->appendFormat("#define MAP_PATH \"%s\"", builder->target->sysMapPath().c_str());
    builder->newline();
    emitTypes(builder);
    control->emitTableTypes(builder);
    parser->emitTypes(builder);
    builder->appendLine("#if CONTROL_PLANE");
    builder->appendLine("static void init_tables() ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("u32 %s = 0;", zeroKey.c_str());
    builder->newline();
    control->emitTableInitializers(builder);
    builder->blockEnd(true);
    builder->appendLine("#endif");
    builder->appendLine("#endif");
}

void EBPFProgram::emitTypes(CodeBuilder *builder) {
    for (auto d : program->objects) {
        if (d->is<IR::Type>() && !d->is<IR::IContainer>() && !d->is<IR::Type_Extern>() &&
            !d->is<IR::Type_Parser>() && !d->is<IR::Type_Control>() && !d->is<IR::Type_Typedef>() &&
            !d->is<IR::Type_Error>()) {
            auto type = EBPFTypeFactory::instance->create(d->to<IR::Type>());
            if (type == nullptr) continue;
            if (d->is<IR::Type_Enum>() && d->to<IR::Type_Enum>()->name == "xdp_action")
                continue;  // already in linux/bpf.h
            type->emit(builder);
            builder->newline();
        }
    }
}

namespace {
class ErrorCodesVisitor : public Inspector {
    CodeBuilder *builder;

 public:
    explicit ErrorCodesVisitor(CodeBuilder *builder) : builder(builder) {}
    bool preorder(const IR::Type_Error *errors) override {
        for (auto m : *errors->getDeclarations()) {
            builder->emitIndent();
            builder->appendFormat("%s,\n", m->getName().name.c_str());
        }
        return false;
    }
};
}  // namespace

void EBPFProgram::emitCommonPreamble(CodeBuilder *builder) {
    builder->newline();
    builder->appendLine("#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)");
    builder->appendLine("#define BYTES(w) ((w) / 8)");
    builder->appendLine(
        "#define write_partial(a, s, v) do "
        "{ u8 mask = EBPF_MASK(u8, s); "
        "*((u8*)a) = ((*((u8*)a)) & ~mask) | (((v) >> (8 - (s))) & mask); "
        "} while (0)");
    builder->appendLine(
        "#define write_partial_ex(a, w, s, v) do { *((u8*)a) = ((*((u8*)a)) "
        "& ~(EBPF_MASK(u8, w) << s)) | (v << s) ; } while (0)");
    builder->appendLine(
        "#define write_byte(base, offset, v) do { "
        "*(u8*)((base) + (offset)) = (v); "
        "} while (0)");
}

void EBPFProgram::emitPreamble(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("enum %s ", errorEnum.c_str());
    builder->blockStart();

    ErrorCodesVisitor visitor(builder);
    program->apply(visitor);

    builder->blockEnd(false);
    builder->endOfStatement(true);

    emitCommonPreamble(builder);

    builder->newline();
    builder->appendLine("void* memcpy(void* dest, const void* src, size_t num);");
    builder->newline();

    builder->target->emitPreamble(builder);
}

void EBPFProgram::emitLocalVariables(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("unsigned %s = 0;", offsetVar.c_str());
    builder->appendFormat("unsigned %s_save = 0;", offsetVar.c_str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("enum %s %s = %s;", errorEnum.c_str(), errorVar.c_str(),
                          P4::P4CoreLibrary::instance().noError.str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("void* %s = %s;", packetStartVar.c_str(),
                          builder->target->dataOffset(model.CPacketName.str()).c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("void* %s = %s;", packetEndVar.c_str(),
                          builder->target->dataEnd(model.CPacketName.str()).c_str());
    builder->newline();

    if (model.arch == ModelArchitecture::EbpfFilter) {
        builder->emitIndent();
        builder->appendFormat("u8 %s = 0;", control->accept->name.name.c_str());
        builder->newline();
    } else if (model.arch == ModelArchitecture::XdpSwitch) {
        builder->emitIndent();
        builder->append("struct xdp_input imd = { .input_port = skb->ingress_ifindex };");
        builder->newline();
        builder->emitIndent();
        builder->append("struct xdp_output omd = { };");
        builder->newline();
    } else {
        BUG("Invalid value for model.arch !");
    }

    builder->emitIndent();
    builder->appendFormat("u32 %s = 0;", zeroKey.c_str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("unsigned char %s;", byteVar.c_str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("u32 %s = %s", lengthVar.c_str(),
                          builder->target->dataLength(model.CPacketName.str()).c_str());
    builder->endOfStatement(true);
}

void EBPFProgram::emitHeaderInstances(CodeBuilder *builder) {
    builder->emitIndent();
    parser->headerType->declare(builder, parser->headers->name.name, false);
}

void EBPFProgram::emitPipeline(CodeBuilder *builder) {
    builder->emitIndent();
    builder->append(IR::ParserState::accept);
    builder->append(":");
    builder->newline();
    builder->emitIndent();
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "Control: packet processing started");
    control->emit(builder);
    builder->blockEnd(true);

    if (model.arch == ModelArchitecture::XdpSwitch) {
        BUG_CHECK(deparser != nullptr, "XDP program can't be missing deparser");
        builder->emitIndent();
        builder->append("/* deparser */");
        builder->newline();
        builder->emitIndent();
        builder->blockStart();
        deparser->emit(builder);
        builder->blockEnd(true);
    } else {
        builder->target->emitTraceMessage(builder, "Control: packet processing finished, pass=%d",
                                          1, control->accept->name.name.c_str());
    }
}

}  // namespace EBPF
