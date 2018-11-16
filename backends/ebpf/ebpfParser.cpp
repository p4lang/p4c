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

#include "ebpfModel.h"
#include "ebpfParser.h"
#include "ebpfType.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace EBPF {

namespace {
class StateTranslationVisitor : public CodeGenInspector {
    bool hasDefault;
    P4::P4CoreLibrary& p4lib;
    const EBPFParserState* state;

    void compileExtractField(const IR::Expression* expr, cstring name,
                             unsigned alignment, EBPFType* type);
    void compileExtract(const IR::Expression* destination);
    void compileLookahead(const IR::Expression* destination);

 public:
    explicit StateTranslationVisitor(const EBPFParserState* state) :
            CodeGenInspector(state->parser->program->refMap, state->parser->program->typeMap),
            hasDefault(false), p4lib(P4::P4CoreLibrary::instance), state(state) {}
    bool preorder(const IR::ParserState* state) override;
    bool preorder(const IR::SelectCase* selectCase) override;
    bool preorder(const IR::SelectExpression* expression) override;
    bool preorder(const IR::Member* expression) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::MethodCallStatement* stat) override
    { visit(stat->methodCall); return false; }
    bool preorder(const IR::AssignmentStatement* stat) override;
};
}  // namespace

void
StateTranslationVisitor::compileLookahead(const IR::Expression* destination) {
    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s_save = %s",
                          state->parser->program->offsetVar.c_str(),
                          state->parser->program->offsetVar.c_str());
    builder->endOfStatement(true);
    compileExtract(destination);
    builder->emitIndent();
    builder->appendFormat("%s = %s_save",
                          state->parser->program->offsetVar.c_str(),
                          state->parser->program->offsetVar.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

bool StateTranslationVisitor::preorder(const IR::AssignmentStatement* statement) {
    if (auto mce = statement->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(mce,
                                              state->parser->program->refMap,
                                              state->parser->program->typeMap);
        auto extMethod = mi->to<P4::ExternMethod>();
        if (extMethod == nullptr)
            BUG("Unhandled method %1%", mce);

        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.lookahead.name) {
                compileLookahead(statement->left);
                return false;
            }
        }
        ::error("Unexpected method call in parser %1%", statement->right);
    }

    CodeGenInspector::visit(statement);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::ParserState* parserState) {
    if (parserState->isBuiltin()) return false;

    builder->emitIndent();
    builder->append(parserState->name.name);
    builder->append(":");
    builder->spc();
    builder->blockStart();

    visit(parserState->components, "components");
    if (parserState->selectExpression == nullptr) {
        builder->emitIndent();
        builder->append("goto ");
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

bool StateTranslationVisitor::preorder(const IR::SelectExpression* expression) {
    hasDefault = false;
    if (expression->select->components.size() != 1) {
        // TODO: this does not handle correctly tuples
        ::error("%1%: only supporting a single argument for select", expression->select);
        return false;
    }
    builder->emitIndent();
    builder->append("switch (");
    visit(expression->select);
    builder->append(") ");
    builder->blockStart();

    for (auto e : expression->selectCases)
        visit(e);

    if (!hasDefault) {
        builder->emitIndent();
        builder->appendFormat("default: goto %s;", IR::ParserState::reject.c_str());
        builder->newline();
    }

    builder->blockEnd(true);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::SelectCase* selectCase) {
    builder->emitIndent();
    if (selectCase->keyset->is<IR::DefaultExpression>()) {
        hasDefault = true;
        builder->append("default: ");
    } else {
        builder->append("case ");
        visit(selectCase->keyset);
        builder->append(": ");
    }
    builder->append("goto ");
    visit(selectCase->state);
    builder->endOfStatement(true);
    return false;
}

void
StateTranslationVisitor::compileExtractField(
    const IR::Expression* expr, cstring field, unsigned alignment, EBPFType* type) {
    unsigned widthToExtract = dynamic_cast<IHasWidth*>(type)->widthInBits();
    auto program = state->parser->program;

    if (widthToExtract <= 64) {
        unsigned lastBitIndex = widthToExtract + alignment - 1;
        unsigned lastWordIndex = lastBitIndex / 8;
        unsigned wordsToRead = lastWordIndex + 1;
        unsigned loadSize;

        const char* helper = nullptr;
        if (wordsToRead <= 1) {
            helper = "load_byte";
            loadSize = 8;
        } else if (widthToExtract <= 16)  {
            helper = "load_half";
            loadSize = 16;
        } else if (widthToExtract <= 32) {
            helper = "load_word";
            loadSize = 32;
        } else {
            if (widthToExtract > 64) BUG("Unexpected width %d", widthToExtract);
            helper = "load_dword";
            loadSize = 64;
        }

        unsigned shift = loadSize - alignment - widthToExtract;
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s = (", field.c_str());
        type->emit(builder);
        builder->appendFormat(")((%s(%s, BYTES(%s))",
                              helper,
                              program->packetStartVar.c_str(),
                              program->offsetVar.c_str());
        if (shift != 0)
            builder->appendFormat(" >> %d", shift);
        builder->append(")");

        if (widthToExtract != loadSize) {
            builder->append(" & EBPF_MASK(");
            type->emit(builder);
            builder->appendFormat(", %d)", widthToExtract);
        }

        builder->append(")");
        builder->endOfStatement(true);
    } else {
        // wide values; read all bytes one by one.
        unsigned shift;
        if (alignment == 0)
            shift = 0;
        else
            shift = 8 - alignment;

        const char* helper;
        if (shift == 0)
            helper = "load_byte";
        else
            helper = "load_half";
        auto bt = EBPFTypeFactory::instance->create(IR::Type_Bits::get(8));
        unsigned bytes = ROUNDUP(widthToExtract, 8);
        for (unsigned i=0; i < bytes; i++) {
            builder->emitIndent();
            visit(expr);
            builder->appendFormat(".%s[%d] = (", field.c_str(), i);
            bt->emit(builder);
            builder->appendFormat(")((%s(%s, BYTES(%s) + %d) >> %d)",
                                  helper,
                                  program->packetStartVar.c_str(),
                                  program->offsetVar.c_str(), i, shift);

            if ((i == bytes - 1) && (widthToExtract % 8 != 0)) {
                builder->append(" & EBPF_MASK(");
                bt->emit(builder);
                builder->appendFormat(", %d)", widthToExtract % 8);
            }

            builder->append(")");
            builder->endOfStatement(true);
        }
    }

    builder->emitIndent();
    builder->appendFormat("%s += %d", program->offsetVar.c_str(), widthToExtract);
    builder->endOfStatement(true);
    builder->newline();
}

void
StateTranslationVisitor::compileExtract(const IR::Expression* destination) {
    auto type = state->parser->typeMap->getType(destination);
    auto ht = type->to<IR::Type_StructLike>();
    if (ht == nullptr) {
        ::error("Cannot extract to a non-struct type %1%", destination);
        return;
    }

    unsigned width = ht->width_bits();
    auto program = state->parser->program;
    builder->emitIndent();
    builder->appendFormat("if (%s < %s + BYTES(%s + %d)) ",
                          program->packetEndVar.c_str(),
                          program->packetStartVar.c_str(),
                          program->offsetVar.c_str(), width);
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("%s = %s;", program->errorVar.c_str(),
                          p4lib.packetTooShort.str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    builder->blockEnd(true);

    unsigned alignment = 0;
    for (auto f : ht->fields) {
        auto ftype = state->parser->typeMap->getType(f);
        auto etype = EBPFTypeFactory::instance->create(ftype);
        auto et = dynamic_cast<IHasWidth*>(etype);
        if (et == nullptr) {
            ::error("Only headers with fixed widths supported %1%", f);
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

bool StateTranslationVisitor::preorder(const IR::MethodCallExpression* expression) {
    builder->append("/* ");
    visit(expression->method);
    builder->append("(");
    bool first = true;
    for (auto a  : *expression->arguments) {
        if (!first)
            builder->append(", ");
        first = false;
        visit(a);
    }
    builder->append(")");
    builder->append("*/");
    builder->newline();

    auto mi = P4::MethodInstance::resolve(expression,
                                          state->parser->program->refMap,
                                          state->parser->program->typeMap);
    auto extMethod = mi->to<P4::ExternMethod>();
    if (extMethod != nullptr) {
        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.extract.name) {
                if (expression->arguments->size() != 1) {
                    ::error("Variable-sized header fields not yet supported %1%", expression);
                    return false;
                }
                compileExtract(expression->arguments->at(0)->expression);
                return false;
            }
            BUG("Unhandled packet method %1%", expression->method);
            return false;
        }
    }

    ::error("Unexpected method call in parser %1%", expression);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::Member* expression) {
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

//////////////////////////////////////////////////////////////////

void EBPFParserState::emit(CodeBuilder* builder) {
    StateTranslationVisitor visitor(this);
    visitor.setBuilder(builder);
    state->apply(visitor);
}

EBPFParser::EBPFParser(const EBPFProgram* program, const IR::ParserBlock* block,
                       const P4::TypeMap* typeMap) :
        program(program), typeMap(typeMap), parserBlock(block),
        packet(nullptr), headers(nullptr), headerType(nullptr) {}

void EBPFParser::emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl) {
    if (decl->is<IR::Declaration_Variable>()) {
        auto vd = decl->to<IR::Declaration_Variable>();
        auto etype = EBPFTypeFactory::instance->create(vd->type);
        builder->emitIndent();
        etype->declare(builder, vd->name, false);
        builder->endOfStatement(true);
        BUG_CHECK(vd->initializer == nullptr,
                  "%1%: declarations with initializers not supported", decl);
        return;
    }
    BUG("%1%: not yet handled", decl);
}


void EBPFParser::emit(CodeBuilder* builder) {
    for (auto l : parserBlock->container->parserLocals)
        emitDeclaration(builder, l);
    for (auto s : states)
        s->emit(builder);
    builder->newline();

    // Create a synthetic reject state
    builder->emitIndent();
    builder->appendFormat("%s: { return %s; }",
                          IR::ParserState::reject.c_str(),
                          builder->target->abortReturnCode().c_str());
    builder->newline();
    builder->newline();
}

bool EBPFParser::build() {
    auto pl = parserBlock->container->type->applyParams;
    if (pl->size() != 2) {
        ::error("Expected parser to have exactly 2 parameters");
        return false;
    }

    auto it = pl->parameters.begin();
    packet = *it; ++it;
    headers = *it;
    for (auto state : parserBlock->container->states) {
        auto ps = new EBPFParserState(state, this);
        states.push_back(ps);
    }

    auto ht = typeMap->getType(headers);
    if (ht == nullptr)
        return false;
    headerType = EBPFTypeFactory::instance->create(ht);
    return true;
}

}  // namespace EBPF
