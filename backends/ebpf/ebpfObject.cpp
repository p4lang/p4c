#include "ebpfObject.h"
#include "ebpfType.h"
#include "ebpfControl.h"
#include "ebpfParser.h"
#include "ebpfTable.h"
#include "frontends/p4/coreLibrary.h"

namespace EBPF {

cstring nameFromAnnotation(const IR::Annotations* annotations,
                           cstring defaultValue) {
    CHECK_NULL(annotations); CHECK_NULL(defaultValue);
    auto anno = annotations->getSingle(IR::Annotation::nameAnnotation);
    if (anno != nullptr) {
        CHECK_NULL(anno->expr);
        auto str = anno->expr->to<IR::StringLiteral>();
        CHECK_NULL(str);
        return str->value;
    }
    return defaultValue;
}

bool EBPFProgram::build() {
    auto pack = blockMap->getMain();
    if (pack->getConstructorParameters()->size() != 2) {
        ::error("Expected toplevel package %1% to have 2 parameters", pack->type);
        return false;
    }

    auto pb = blockMap->getBlockBoundToParameter(pack, model.filter.parser.name)
                      ->to<IR::ParserBlock>();
    BUG_CHECK(pb != nullptr, "No parser block found");
    parser = new EBPFParser(this, pb, blockMap->typeMap);
    bool success = parser->build();
    if (!success)
        return success;

    auto cb = blockMap->getBlockBoundToParameter(pack, model.filter.filter.name)
                      ->to<IR::ControlBlock>();
    BUG_CHECK(cb != nullptr, "No control block found");
    control = new EBPFControl(this, cb);
    success = control->build();
    if (!success)
        return success;

    return true;
}

void EBPFProgram::emit(CodeBuilder *builder) {
    builder->target->emitIncludes(builder);
    emitPreamble(builder);
    emitTypes(builder);
    control->emitTables(builder);

    builder->newline();
    builder->emitIndent();
    builder->target->emitCodeSection(builder);
    builder->emitIndent();
    builder->appendFormat("int %s(struct __sk_buff* %s) ", functionName, model.CPacketName.str());
    builder->blockStart();

    emitHeaderInstances(builder);
    builder->append(" = ");
    parser->headerType->emitInitializer(builder);
    builder->endOfStatement(true);

    createLocalVariables(builder);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::start.name.c_str());
    builder->newline();

    parser->emit(builder);
    emitPipeline(builder);

    builder->emitIndent();
    builder->append(endLabel);
    builder->appendLine(":");
    builder->emitIndent();
    builder->append("return ");
    builder->append(control->accept->name.name);
    builder->appendLine(";");
    builder->blockEnd(true);  // end of function

    builder->target->emitLicense(builder, license);
}

void EBPFProgram::emitTypes(CodeBuilder* builder) {
    for (auto d : *program->declarations) {
        if (d->is<IR::Type>() && !d->is<IR::IContainer>() &&
            !d->is<IR::Type_Extern>() && !d->is<IR::Type_Parser>() &&
            !d->is<IR::Type_Control>() && !d->is<IR::Type_Typedef>()) {
            auto type = EBPFTypeFactory::instance->create(d->to<IR::Type>());
            type->emit(builder);
            builder->newline();
        }
    }
}

namespace {
class ErrorCodesVisitor : public Inspector {
    CodeBuilder* builder;
 public:
    explicit ErrorCodesVisitor(CodeBuilder* builder) : builder(builder) {}
    bool preorder(const IR::Declaration_Errors* errors) override {
        for (auto m : *errors->getDeclarations()) {
            builder->emitIndent();
            builder->appendFormat("%s,\n", m->getName().name.c_str());
        }
        return false;
    }
};
}  // namespace

void EBPFProgram::emitPreamble(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("enum %s ", errorEnum.c_str());
    builder->blockStart();

    ErrorCodesVisitor visitor(builder);
    program->apply(visitor);

    builder->blockEnd(false);
    builder->endOfStatement(true);
    builder->newline();
    builder->appendLine("#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)");
    builder->appendLine("#define BYTES(w) ((w + 7) / 8)");
    builder->newline();
}

void EBPFProgram::createLocalVariables(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("unsigned %s = 0;", offsetVar);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("enum %s %s = %s;", errorEnum, errorVar,
                          P4::P4CoreLibrary::instance.noError.str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("u8 %s = 0;", control->accept->name.name);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("u32 %s = 0;", zeroKey);
    builder->newline();
}

void EBPFProgram::emitHeaderInstances(CodeBuilder* builder) {
    builder->emitIndent();
    parser->headerType->declare(builder, parser->headers->name.name, false);
}

void EBPFProgram::emitPipeline(CodeBuilder* builder) {
    builder->emitIndent();
    builder->append(IR::ParserState::accept);
    builder->append(":");
    builder->newline();
    control->emit(builder);
}

}  // namespace EBPF
