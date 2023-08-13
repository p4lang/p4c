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

#include "ubpfParser.h"

#include <stddef.h>

#include <string>

#include "backends/ebpf/ebpfParser.h"
#include "backends/ebpf/target.h"
#include "frontends/common/model.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/algorithm.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "ubpfModel.h"
#include "ubpfType.h"

namespace UBPF {

namespace {
class UBPFStateTranslationVisitor : public EBPF::CodeGenInspector {
    // stores the result of evaluating the select argument
    cstring selectValue;

    P4::P4CoreLibrary &p4lib;
    const UBPFParserState *state;

    void emitCheckPacketLength(const IR::Expression *expr, const char *varname, unsigned width);
    void emitCheckPacketLength(const IR::Expression *expr) {
        emitCheckPacketLength(expr, nullptr, 0);
    }
    void emitCheckPacketLength(unsigned width) { emitCheckPacketLength(nullptr, nullptr, width); }
    void emitCheckPacketLength(const char *varname) { emitCheckPacketLength(nullptr, varname, 0); }

    void compileExtractField(const IR::Expression *expr, cstring field, unsigned alignment,
                             EBPF::EBPFType *type, bool advanceCursor = true);
    void compileExtract(const IR::Expression *destination);

    void compileLookahead(const IR::Expression *destination);

    void compileAdvance(const IR::Expression *expr);

 public:
    explicit UBPFStateTranslationVisitor(const UBPFParserState *state)
        : CodeGenInspector(state->parser->program->refMap, state->parser->program->typeMap),
          p4lib(P4::P4CoreLibrary::instance()),
          state(state) {}
    bool preorder(const IR::ParserState *state) override;
    bool preorder(const IR::SelectCase *selectCase) override;
    bool preorder(const IR::SelectExpression *expression) override;
    bool preorder(const IR::Member *expression) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
    bool preorder(const IR::MethodCallStatement *stat) override {
        visit(stat->methodCall);
        return false;
    }
    bool preorder(const IR::AssignmentStatement *stat) override;
};
}  // namespace

// if expr is nullprt, width is used instead
void UBPFStateTranslationVisitor::emitCheckPacketLength(const IR::Expression *expr,
                                                        const char *varname, unsigned width) {
    auto program = state->parser->program;

    builder->emitIndent();
    if (expr != nullptr) {
        builder->appendFormat("if (%s < BYTES(%s + ", program->lengthVar.c_str(),
                              program->offsetVar.c_str());
        visit(expr);
        builder->append(")) ");
    } else if (varname != nullptr) {
        builder->appendFormat("if (%s < BYTES(%s + %s)) ", program->lengthVar.c_str(),
                              program->offsetVar.c_str(), varname);
    } else {
        builder->appendFormat("if (%s < BYTES(%s + %d)) ", program->lengthVar.c_str(),
                              program->offsetVar.c_str(), width);
    }

    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    builder->blockEnd(true);

    builder->emitIndent();
    builder->newline();
}

bool UBPFStateTranslationVisitor::preorder(const IR::ParserState *parserState) {
    if (parserState->isBuiltin()) return false;

    builder->emitIndent();
    builder->append(parserState->name.name);
    builder->append(":");
    builder->spc();
    builder->blockStart();

    visit(parserState->components, "components");
    if (parserState->selectExpression == nullptr) {
        builder->emitIndent();
        builder->append(" goto ");
        builder->append(IR::ParserState::reject);
        builder->endOfStatement(true);
    } else if (parserState->selectExpression->is<IR::SelectExpression>()) {
        visit(parserState->selectExpression);
    } else {
        // must be a PathExpression which is a state name
        if (!parserState->selectExpression->is<IR::PathExpression>())
            BUG("Expected a PathExpression, got a %1%", parserState->selectExpression);
        builder->emitIndent();
        builder->append("goto ");
        visit(parserState->selectExpression);
        builder->endOfStatement(true);
    }

    builder->blockEnd(true);
    return false;
}

bool UBPFStateTranslationVisitor::preorder(const IR::SelectCase *selectCase) {
    builder->emitIndent();
    if (auto mask = selectCase->keyset->to<IR::Mask>()) {
        builder->appendFormat("if ((%s", selectValue);
        builder->append(" & ");
        visit(mask->right);
        builder->append(") == (");
        visit(mask->left);
        builder->append(" & ");
        visit(mask->right);
        builder->append("))");
    } else {
        builder->appendFormat("if (%s", selectValue);
        builder->append(" == ");
        visit(selectCase->keyset);
        builder->append(")");
    }
    builder->append("goto ");
    visit(selectCase->state);
    builder->endOfStatement(true);
    return false;
}

bool UBPFStateTranslationVisitor::preorder(const IR::SelectExpression *expression) {
    BUG_CHECK(expression->select->components.size() == 1, "%1%: tuple not eliminated in select",
              expression->select);
    selectValue = state->parser->program->refMap->newName("select");
    auto type = state->parser->program->typeMap->getType(expression->select, true);
    if (auto list = type->to<IR::Type_List>()) {
        BUG_CHECK(list->components.size() == 1, "%1% list type with more than 1 element", list);
        type = list->components.at(0);
    }
    auto etype = UBPFTypeFactory::instance->create(type);
    builder->emitIndent();
    etype->declare(builder, selectValue, false);
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("%s = ", selectValue);
    visit(expression->select);
    builder->endOfStatement(true);
    for (auto e : expression->selectCases) visit(e);

    builder->emitIndent();
    builder->appendFormat("else goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    return false;
}

bool UBPFStateTranslationVisitor::preorder(const IR::Member *expression) {
    if (expression->expr->is<IR::PathExpression>()) {
        auto pe = expression->expr->to<IR::PathExpression>();
        auto decl = state->parser->program->refMap->getDeclaration(pe->path, true);
        if (decl == state->parser->packet) {
            builder->append(expression->member);
            return false;
        }
    }

    visit(expression->expr);
    builder->append(".");
    builder->append(expression->member);
    return false;
}

void UBPFStateTranslationVisitor::compileExtractField(const IR::Expression *expr, cstring field,
                                                      unsigned alignment, EBPF::EBPFType *type,
                                                      bool advanceCursor) {
    unsigned widthToExtract = dynamic_cast<EBPF::IHasWidth *>(type)->widthInBits();
    auto program = state->parser->program;

    if (widthToExtract <= 64) {
        unsigned lastBitIndex = widthToExtract + alignment - 1;
        unsigned lastWordIndex = lastBitIndex / 8;
        unsigned wordsToRead = lastWordIndex + 1;
        unsigned loadSize;

        const char *helper = nullptr;
        if (wordsToRead <= 1) {
            helper = "load_byte";
            loadSize = 8;
        } else if (wordsToRead <= 2) {
            helper = "load_half";
            loadSize = 16;
        } else if (wordsToRead <= 4) {
            helper = "load_word";
            loadSize = 32;
        } else {
            helper = "load_dword";
            loadSize = 64;
        }

        unsigned shift = loadSize - alignment - widthToExtract;
        builder->emitIndent();
        if (expr) {
            visit(expr);
            builder->appendFormat(".%s = (", field.c_str());
        } else {
            builder->append("(");
        }
        type->emit(builder);
        builder->appendFormat(")((%s(%s, BYTES(%s))", helper, program->packetStartVar.c_str(),
                              program->offsetVar.c_str());
        if (shift != 0) builder->appendFormat(" >> %d", shift);
        builder->append(")");

        if (widthToExtract != loadSize) {
            builder->append(" & BPF_MASK(");
            type->emit(builder);
            builder->appendFormat(", %d)", widthToExtract);
        }

        builder->append(")");
        if (advanceCursor) builder->endOfStatement(true);
    } else {
        // wide values; read all bytes one by one.
        unsigned shift;
        if (alignment == 0)
            shift = 0;
        else
            shift = 8 - alignment;

        const char *helper;
        if (shift == 0)
            helper = "load_byte";
        else
            helper = "load_half";
        auto bt = UBPFTypeFactory::instance->create(IR::Type_Bits::get(8));
        unsigned bytes = ROUNDUP(widthToExtract, 8);
        for (unsigned i = 0; i < bytes; i++) {
            builder->emitIndent();
            visit(expr);
            builder->appendFormat(".%s[%d] = (", field.c_str(), i);
            bt->emit(builder);
            builder->appendFormat(")((%s(%s, BYTES(%s) + %d) >> %d)", helper,
                                  program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                  bytes - i - 1, shift);

            if ((i == bytes - 1) && (widthToExtract % 8 != 0)) {
                builder->append(" & BPF_MASK(");
                bt->emit(builder);
                builder->appendFormat(", %d)", widthToExtract % 8);
            }

            builder->append(")");
            builder->endOfStatement(true);
        }
    }

    if (advanceCursor) {
        builder->emitIndent();
        builder->appendFormat("%s += %d", program->offsetVar.c_str(), widthToExtract);
        builder->endOfStatement(true);
        builder->newline();
    }
}

void UBPFStateTranslationVisitor::compileExtract(const IR::Expression *destination) {
    auto type = state->parser->typeMap->getType(destination);
    auto ht = type->to<IR::Type_StructLike>();

    if (ht == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Cannot extract to a non-struct type %1%",
                destination);
        return;
    }

    unsigned width = ht->width_bits();
    emitCheckPacketLength(width);

    unsigned alignment = 0;
    for (auto f : ht->fields) {
        auto ftype = state->parser->typeMap->getType(f);
        auto etype = UBPFTypeFactory::instance->create(ftype);
        auto et = dynamic_cast<EBPF::IHasWidth *>(etype);
        if (et == nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Only headers with fixed widths supported %1%", f);
            return;
        }
        compileExtractField(destination, f->name, alignment, etype);
        alignment += et->widthInBits();
        alignment %= 8;
    }

    if (ht->is<IR::Type_Header>()) {
        builder->emitIndent();
        visit(destination);
        builder->appendLine(".ebpf_valid = 1;");
    }
}

void UBPFStateTranslationVisitor::compileLookahead(const IR::Expression *destination) {
    auto type = state->parser->typeMap->getType(destination);
    auto etype = UBPFTypeFactory::instance->create(type);

    if (type->to<IR::Type_Bits>() == nullptr)
        BUG("lookahead<%1%>(): only bit type is supported yet", type);

    unsigned width = dynamic_cast<EBPF::IHasWidth *>(etype)->widthInBits();
    if (width > 64) BUG("lookahead<%1%>(): more than 64 bits not supported yet", type);

    // check packet's length
    emitCheckPacketLength(width);

    builder->emitIndent();
    etype->emit(builder);
    builder->append(" ");
    visit(destination);
    builder->append(" = ");
    compileExtractField(nullptr, "", 0, etype, false);
    builder->endOfStatement(true);
}

void UBPFStateTranslationVisitor::compileAdvance(const IR::Expression *expr) {
    auto type = state->parser->typeMap->getType(expr);
    auto etype = UBPFTypeFactory::instance->create(type);
    cstring tmpVarName = refMap->newName("tmp");

    builder->emitIndent();
    builder->blockStart();
    // declare temp var
    builder->emitIndent();
    etype->emit(builder);
    builder->appendFormat(" %s = ", tmpVarName.c_str());
    visit(expr);
    builder->endOfStatement(true);

    // check packet's length
    emitCheckPacketLength(tmpVarName.c_str());

    builder->emitIndent();
    builder->appendFormat("%s += %s", state->parser->program->offsetVar.c_str(),
                          tmpVarName.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
    builder->newline();
}

bool UBPFStateTranslationVisitor::preorder(const IR::MethodCallExpression *expression) {
    builder->emitIndent();
    builder->append("/* ");
    visit(expression->method);
    builder->append("(");
    bool first = true;
    for (auto a : *expression->arguments) {
        if (!first) builder->append(", ");
        first = false;
        visit(a);
    }
    builder->append(")");
    builder->append("*/");
    builder->newline();

    auto mi = P4::MethodInstance::resolve(expression, state->parser->program->refMap,
                                          state->parser->program->typeMap);

    auto extMethod = mi->to<P4::ExternMethod>();
    if (extMethod != nullptr) {
        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.extract.name) {
                if (expression->arguments->size() != 1) {
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "Variable-sized header fields not yet supported %1%", expression);
                    return false;
                }
                compileExtract(expression->arguments->at(0)->expression);
                return false;
            }

            if (extMethod->method->name.name == p4lib.packetIn.advance.name) {
                if (expression->arguments->size() != 1) {
                    ::error(ErrorType::ERR_EXPECTED, "%1%: expected 1 argument", expression);
                    return false;
                }
                compileAdvance(expression->arguments->at(0)->expression);
                return false;
            }

            BUG("Unhandled packet method %1%", expression->method);
            return false;
        }
    }

    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected method call in parser %1%", expression);

    return false;
}

bool UBPFStateTranslationVisitor::preorder(const IR::AssignmentStatement *stat) {
    if (auto mce = stat->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(mce, state->parser->program->refMap,
                                              state->parser->program->typeMap);
        auto extMethod = mi->to<P4::ExternMethod>();
        if (extMethod == nullptr) BUG("Unhandled method %1%", mce);

        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.lookahead.name) {
                compileLookahead(stat->left);
                return false;
            }
        }
        ::error(ErrorType::ERR_UNEXPECTED, "Unexpected method call in parser %1%", stat->right);
    } else {
        builder->emitIndent();
        visit(stat->left);
        builder->append(" = ");
        visit(stat->right);
        builder->endOfStatement(true);
    }

    return false;
}

void UBPFParserState::emit(EBPF::CodeBuilder *builder) {
    UBPFStateTranslationVisitor visitor(this);
    visitor.setBuilder(builder);
    state->apply(visitor);
}

void UBPFParser::emit(EBPF::CodeBuilder *builder) {
    for (auto s : states) s->emit(builder);

    builder->newline();

    // Create a synthetic reject state
    builder->emitIndent();
    builder->appendFormat("%s: { return %s; }", IR::ParserState::reject.c_str(),
                          builder->target->abortReturnCode().c_str());
    builder->newline();
    builder->newline();
}

bool UBPFParser::build() {
    auto pl = parserBlock->container->type->applyParams;
    size_t numberOfArgs = UBPFModel::instance.numberOfParserArguments();
    if (pl->size() != numberOfArgs) {
        ::error(ErrorType::ERR_EXPECTED, "Expected parser to have exactly %d parameters",
                numberOfArgs);
        return false;
    }

    auto it = pl->parameters.begin();
    packet = *it;
    ++it;
    headers = *it;
    ++it;
    metadata = *it;
    for (auto state : parserBlock->container->states) {
        auto ps = new UBPFParserState(state, this);
        states.push_back(ps);
    }

    auto ht = typeMap->getType(headers);
    if (ht == nullptr) return false;
    headerType = UBPFTypeFactory::instance->create(ht);

    auto md = typeMap->getType(metadata);
    if (md == nullptr) return false;
    metadataType = UBPFTypeFactory::instance->create(md);
    return true;
}

}  // namespace UBPF
