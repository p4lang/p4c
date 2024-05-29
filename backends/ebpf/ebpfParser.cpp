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

#include "ebpfParser.h"

#include "ebpfModel.h"
#include "ebpfType.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace EBPF {

void StateTranslationVisitor::compileLookahead(const IR::Expression *destination) {
    cstring msgStr = Util::printf_format("Parser: lookahead for %s %s",
                                         state->parser->typeMap->getType(destination)->toString(),
                                         destination->toString());
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("u8* %s_save = %s", state->parser->program->headerStartVar.c_str(),
                          state->parser->program->headerStartVar.c_str());
    builder->endOfStatement(true);
    compileExtract(destination);
    builder->emitIndent();
    builder->appendFormat("%s = %s_save", state->parser->program->headerStartVar.c_str(),
                          state->parser->program->headerStartVar.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

void StateTranslationVisitor::compileAdvance(const P4::ExternMethod *extMethod) {
    auto argExpr = extMethod->expr->arguments->at(0)->expression;
    auto cnst = argExpr->to<IR::Constant>();
    if (cnst) {
        cstring argStr = cstring::to_cstring(cnst->asUnsigned());
        cstring offsetStr =
            Util::printf_format("%s - (u8*)%s + BYTES(%s)", state->parser->program->headerStartVar,
                                state->parser->program->packetStartVar, argStr);
        builder->target->emitTraceMessage(builder,
                                          "Parser (advance): check pkt_len=%%d < "
                                          "last_read_byte=%%d",
                                          2, state->parser->program->lengthVar.c_str(),
                                          offsetStr.c_str());
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "packet_in.advance() method with non-constant argument is not supported yet");
        return;
    }

    int advanceVal = cnst->asInt();
    if (advanceVal % 8 != 0) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "packet_in.advance(%1%) must be byte-aligned in eBPF backend", advanceVal);
        return;
    }

    builder->emitIndent();
    builder->appendFormat("%s += BYTES(", state->parser->program->headerStartVar.c_str());
    visit(argExpr);
    builder->append(")");
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if ((u8*)%s < %s) ", state->parser->program->packetEndVar.c_str(),
                          state->parser->program->headerStartVar.c_str());
    builder->blockStart();

    builder->target->emitTraceMessage(builder, "Parser: invalid packet (packet too short)");

    builder->emitIndent();
    builder->appendFormat("%s = %s;", state->parser->program->errorVar.c_str(),
                          p4lib.packetTooShort.str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    builder->blockEnd(true);
}

void StateTranslationVisitor::compileVerify(const IR::MethodCallExpression *expression) {
    BUG_CHECK(expression->arguments->size() == 2, "Expected 2 arguments: %1%", expression);

    auto errorExpr = expression->arguments->at(1)->expression;
    auto errorMember = errorExpr->to<IR::Member>();
    auto type = typeMap->getType(errorExpr, true);
    if (!type->is<IR::Type_Error>() || errorMember == nullptr) {
        ::error(ErrorType::ERR_UNEXPECTED, "%1%: not accessing a member error type", errorExpr);
        return;
    }

    builder->emitIndent();
    builder->append("if (!(");
    visit(expression->arguments->at(0));
    builder->append(")) ");
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("%s = %s", state->parser->program->errorVar.c_str(),
                          errorMember->member.name);
    builder->endOfStatement(true);

    cstring msg = Util::printf_format("Verify: condition failed, parser_error=%%u (%s)",
                                      errorMember->member.name);
    builder->target->emitTraceMessage(builder, msg.c_str(), 1,
                                      state->parser->program->errorVar.c_str());

    builder->emitIndent();
    builder->appendFormat("goto %s", IR::ParserState::reject.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

bool StateTranslationVisitor::preorder(const IR::AssignmentStatement *statement) {
    if (auto mce = statement->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(mce, state->parser->program->refMap,
                                              state->parser->program->typeMap);
        auto extMethod = mi->to<P4::ExternMethod>();
        if (extMethod == nullptr) BUG("Unhandled method %1%", mce);

        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.lookahead.name) {
                compileLookahead(statement->left);
                return false;
            } else if (extMethod->method->name.name == p4lib.packetIn.length.name) {
                return CodeGenInspector::preorder(statement);
            }
        }
    }

    return CodeGenInspector::preorder(statement);
}

bool StateTranslationVisitor::preorder(const IR::ParserState *parserState) {
    if (parserState->isBuiltin()) return false;

    builder->emitIndent();
    builder->append(parserState->name.name);
    builder->append(":");
    builder->spc();
    builder->blockStart();

    cstring msgStr =
        Util::printf_format("Parser: state %s (curr_offset=%%d)", parserState->name.name);
    cstring offsetStr = Util::printf_format("%s - (u8*)%s", state->parser->program->headerStartVar,
                                            state->parser->program->packetStartVar);
    builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, offsetStr.c_str());

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
        builder->append(" goto ");
        visit(parserState->selectExpression);
        builder->endOfStatement(true);
    }

    builder->blockEnd(true);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::SelectExpression *expression) {
    BUG_CHECK(expression->select->components.size() == 1, "%1%: tuple not eliminated in select",
              expression->select);
    selectValue = state->parser->program->refMap->newName("select");
    auto type = state->parser->program->typeMap->getType(expression->select, true);
    if (auto list = type->to<IR::Type_List>()) {
        BUG_CHECK(list->components.size() == 1, "%1% list type with more than 1 element", list);
        type = list->components.at(0);
    }
    auto etype = EBPFTypeFactory::instance->create(type);
    builder->emitIndent();
    etype->declare(builder, selectValue, false);
    builder->endOfStatement(true);

    emitAssignStatement(type, nullptr, selectValue, expression->select->components.at(0));
    builder->newline();

    // Init value_sets
    for (auto e : expression->selectCases) {
        if (e->keyset->is<IR::PathExpression>()) {
            cstring pvsName = e->keyset->to<IR::PathExpression>()->path->name.name;
            cstring pvsKeyVarName = state->parser->program->refMap->newName(pvsName + "_key");
            auto pvs = state->parser->getValueSet(pvsName);
            if (pvs != nullptr)
                pvs->emitKeyInitializer(builder, expression, pvsKeyVarName);
            else
                ::error(ErrorType::ERR_UNKNOWN, "%1%: expected a value_set instance", e->keyset);
        }
    }

    for (auto e : expression->selectCases) visit(e);

    builder->emitIndent();
    builder->appendFormat("else goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    return false;
}

bool StateTranslationVisitor::preorder(const IR::SelectCase *selectCase) {
    unsigned width = EBPFInitializerUtils::ebpfTypeWidth(typeMap, selectCase->keyset);
    bool scalar = EBPFScalarType::generatesScalar(width);

    builder->emitIndent();
    if (auto pe = selectCase->keyset->to<IR::PathExpression>()) {
        builder->append("if (");
        cstring pvsName = pe->path->name.name;
        auto pvs = state->parser->getValueSet(pvsName);
        if (pvs) pvs->emitLookup(builder);
        builder->append(" != NULL)");
    } else if (auto mask = selectCase->keyset->to<IR::Mask>()) {
        if (scalar) {
            builder->appendFormat("if ((%s", selectValue);
            builder->append(" & ");
            visit(mask->right);
            builder->append(") == (");
            visit(mask->left);
            builder->append(" & ");
            visit(mask->right);
            builder->append("))");
        } else {
            unsigned bytes = ROUNDUP(width, 8);
            bool first = true;
            cstring hex = EBPFInitializerUtils::genHexStr(mask->right->to<IR::Constant>()->value,
                                                          width, mask->right);
            cstring value = EBPFInitializerUtils::genHexStr(mask->left->to<IR::Constant>()->value,
                                                            width, mask->left);
            builder->append("if (");
            for (unsigned i = 0; i < bytes; ++i) {
                if (!first) {
                    builder->append(" && ");
                }
                builder->appendFormat("((%s[%u] & 0x%s)", selectValue, i, hex.substr(2 * i, 2));
                builder->append(" == ");
                builder->appendFormat("(%s & %s))", value.substr(2 * i, 2), hex.substr(2 * i, 2));
                first = false;
            }
            builder->append(") ");
        }
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

unsigned int StateTranslationVisitor::compileExtractField(const IR::Expression *expr,
                                                          const IR::StructField *field,
                                                          unsigned hdrOffsetBits, EBPFType *type,
                                                          const char *sizecode) {
    unsigned alignment = hdrOffsetBits % 8;
    unsigned widthToExtract = type->as<IHasWidth>().widthInBits();
    auto program = state->parser->program;
    cstring msgStr;
    cstring fieldName = field->name.name;

    builder->appendFormat("/* EBPF::StateTranslationVisitor::compileExtractField: field %s",
                          fieldName);
    if (type->is<EBPF::EBPFScalarType>()) {
        builder->appendFormat(" (%s scalar)",
                              type->as<EBPF::EBPFScalarType>().isvariable ? "variable" : "fixed");
    }
    if (sizecode) builder->appendFormat("\n%s", sizecode);
    builder->appendFormat("*/");

    msgStr = Util::printf_format("Parser: extracting field %s", fieldName);
    builder->target->emitTraceMessage(builder, msgStr.c_str());

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
            // TODO: this is wrong, since a 60-bit unaligned read may require 9 words.
            if (wordsToRead > 64) BUG("Unexpected width %d", widthToExtract);
            helper = "load_dword";
            loadSize = 64;
        }

        unsigned shift = loadSize - alignment - widthToExtract;
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s = (", fieldName.c_str());
        type->emit(builder);
        builder->appendFormat(")((%s(%s, BYTES(%u))", helper, program->headerStartVar.c_str(),
                              hdrOffsetBits);
        if (shift != 0) builder->appendFormat(" >> %d", shift);
        builder->append(")");

        if (widthToExtract != loadSize) {
            builder->append(" & EBPF_MASK(");
            type->emit(builder);
            builder->appendFormat(", %d)", widthToExtract);
        }

        builder->append(")");
        builder->endOfStatement(true);
    } else {
        if (program->options.arch == "psa" && widthToExtract % 8 != 0) {
            // To explain the problem in error lets assume that we have bit<68> field with value:
            //   0x11223344556677889
            //                     ^ this digit will be parsed into a half of byte
            // Such fields are parsed into a table of bytes in network byte order, so possible
            // values in dataplane are (note the position of additional '0' at the end):
            //   0x112233445566778809
            //   0x112233445566778890
            // To correctly insert that padding, the length of field must be known, but tools like
            // nikss-ctl (and the nikss library) don't consume P4info.txtpb to have such knowledge.
            // There is also a bug in (de)parser causing such fields to be deparsed incorrectly.
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: fields wider than 64 bits must have a size multiple of 8 bits (1 byte) "
                    "due to ambiguous padding in the LSB byte when the condition is not met",
                    field);
        }

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
        auto bt = EBPFTypeFactory::instance->create(IR::Type_Bits::get(8));
        unsigned bytes = ROUNDUP(widthToExtract, 8);
        for (unsigned i = 0; i < bytes; i++) {
            builder->emitIndent();
            visit(expr);
            builder->appendFormat(".%s[%d] = (", fieldName.c_str(), i);
            bt->emit(builder);
            builder->appendFormat(")((%s(%s, BYTES(%u) + %d) >> %d)", helper,
                                  program->headerStartVar.c_str(), hdrOffsetBits, i, shift);

            if ((i == bytes - 1) && (widthToExtract % 8 != 0)) {
                builder->append(" & EBPF_MASK(");
                bt->emit(builder);
                builder->appendFormat(", %d)", widthToExtract % 8);
            }

            builder->append(")");
            builder->endOfStatement(true);
        }
    }

    // eBPF can pass 64 bits of data as one argument passed in 64 bit register,
    // so value of the field is printed only when it fits into that register
    if (widthToExtract <= 64) {
        cstring exprStr = expr->is<IR::PathExpression>()
                              ? expr->to<IR::PathExpression>()->path->name.name
                              : expr->toString();

        if (expr->is<IR::Member>() && expr->to<IR::Member>()->expr->is<IR::PathExpression>() &&
            isPointerVariable(
                expr->to<IR::Member>()->expr->to<IR::PathExpression>()->path->name.name)) {
            exprStr = exprStr.replace(".", "->");
        }
        cstring tmp = Util::printf_format("(unsigned long long) %s.%s", exprStr, fieldName);

        msgStr = Util::printf_format("Parser: extracted %s=0x%%llx (%u bits)", fieldName,
                                     widthToExtract);
        builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, tmp.c_str());
    } else {
        msgStr = Util::printf_format("Parser: extracted %s (%u bits)", fieldName, widthToExtract);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
    }
    return (0);
}

/*
 * There has GOT to be a better way to do this, but, given the
 *  interfaces the rest of the code exports, I don't see anything much
 *  better.  Maybe I've just missed something.
 *
 * My thinking is that ->toString returns a P4(ish) decompilation of
 *  the Expression, so we can't use that.  The only thing I know of
 *  that generates C code is to visit() it.  But that outputs to a
 *  SourceCodeBuilder.  I tried using a CodeBuilder, but this->builder
 *  is typed as an EBPF::CodeBuilder, so we can't swap a
 *  Util::SourceCodeBuilder in instead.  So we create a relatively
 *  transient EBPF::CodeBuilder and output to that (by swapping it into
 *  this->builder for the duration).  This means we need a Target to
 *  pass when creating the EBPF::CodeBuilder, and Target is a virtual
 *  base class, demanding implementations for a bunch of methods we
 *  have no use for.  Annoying, but nothing worse; we just provide
 *  dummies that do nothing.
 *
 * The returned string is on the heap, allocated with strdup().  It
 *  would probably be better to make this a std::string or some such,
 *  but I'm not good enough at C++ to get that right.
 *
 * Fortunately, this->builder is a pointer, or I'd have to worry about
 *  copies and moves and try to get _that_ right.  But pointers are
 *  lightweight and totally copyable without issue.
 */
char *StateTranslationVisitor::visit_to_string(const IR::Expression *expr) {
    class StringTarget : public Target {
     public:
        StringTarget(cstring n) : Target(n) {}
#define VB1(a) \
    { (void)a; }
#define VB2(a, b) \
    {             \
        (void)a;  \
        (void)b;  \
    }
#define VB3(a, b, c) \
    {                \
        (void)a;     \
        (void)b;     \
        (void)c;     \
    }
#define VB4(a, b, c, d) \
    {                   \
        (void)a;        \
        (void)b;        \
        (void)c;        \
        (void)d;        \
    }
#define VB6(a, b, c, d, e, f) \
    {                         \
        (void)a;              \
        (void)b;              \
        (void)c;              \
        (void)d;              \
        (void)e;              \
        (void)f;              \
    }
#define SB0() \
    { return (""); }
#define SB1(a)       \
    {                \
        (void)a;     \
        return (""); \
    }
        virtual void emitLicense(Util::SourceCodeBuilder *b, cstring l) const
            VB2(b, l) virtual void emitCodeSection(Util::SourceCodeBuilder *b, cstring n) const
            VB2(b, n) virtual void emitIncludes(Util::SourceCodeBuilder *b) const
            VB1(b) virtual void emitResizeBuffer(Util::SourceCodeBuilder *b, cstring bf,
                                                 cstring o) const
            VB3(b, bf, o) virtual void emitTableLookup(Util::SourceCodeBuilder *b, cstring n,
                                                       cstring k, cstring v) const
            VB4(b, n, k, v) virtual void emitTableUpdate(Util::SourceCodeBuilder *b, cstring n,
                                                         cstring k, cstring v) const
            VB4(b, n, k, v) virtual void emitUserTableUpdate(Util::SourceCodeBuilder *b, cstring n,
                                                             cstring k, cstring v) const
            VB4(b, n, k, v) virtual void emitTableDecl(Util::SourceCodeBuilder *b, cstring tn,
                                                       TableKind k, cstring kt, cstring vt,
                                                       unsigned int sz) const
            VB6(b, tn, k, kt, vt, sz) virtual void emitMain(Util::SourceCodeBuilder *b, cstring fn,
                                                            cstring an) const
            VB3(b, fn, an) virtual cstring dataOffset(cstring b) const SB1(b) virtual cstring
            dataEnd(cstring b) const SB1(b) virtual cstring dataLength(cstring b) const
            SB1(b) virtual cstring forwardReturnCode() const SB0() virtual cstring
            dropReturnCode() const SB0() virtual cstring abortReturnCode() const
            SB0() virtual cstring sysMapPath() const SB0() virtual cstring
            packetDescriptorType() const SB0()
#undef VB1
#undef VB2
#undef VB3
#undef VB4
#undef VB6
#undef SB0
#undef SB1
    };
    StringTarget t = StringTarget("stringizer");
    auto save = builder;
    builder = new EBPF::CodeBuilder(&t);
    visit(expr);
    auto s = builder->toString();
    delete builder;
    builder = save;
    return (strdup(s.c_str()));
}

void StateTranslationVisitor::compileExtract(const IR::Expression *dest,
                                             const IR::Expression *varsize) {
    builder->appendFormat("//compileExtract\n");
    builder->appendFormat("// compileExtract: dest = %s\n", dest->toString());
    builder->appendFormat("// compileExtract: varsize = %s\n",
                          varsize ? varsize->toString() : "nil");
    cstring msgStr;
    auto type = state->parser->typeMap->getType(dest);
    auto ht = type->to<IR::Type_StructLike>();
    if (ht == nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Cannot extract to a non-struct type %1%",
                dest);
        return;
    }

    // We expect all headers to start on a byte boundary.
    unsigned width = ht->width_bits();
    if ((width % 8) != 0) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "Header %1% size %2% is not a multiple of 8 bits.", dest, width);
        return;
    }

    auto program = state->parser->program;

    cstring offsetStr = Util::printf_format("(%s - (u8*)%s) + BYTES(%s)", program->headerStartVar,
                                            program->packetStartVar, cstring::to_cstring(width));

    builder->target->emitTraceMessage(builder, "Parser: check pkt_len=%d >= last_read_byte=%d", 2,
                                      program->lengthVar.c_str(), offsetStr.c_str());

    // to load some fields the compiler will use larger words
    // than actual width of a field (e.g. 48-bit field loaded using load_dword())
    // we must ensure that the larger word is not outside of packet buffer.
    // FIXME: this can fail if a packet does not contain additional payload after header.
    //  However, we don't have better solution in case of using load_X functions to parse packet.
    // TODO: consider using a collection of smaller widths.
    unsigned curr_padding = 0;
    for (auto f : ht->fields) {
        auto ftype = state->parser->typeMap->getType(f);
        auto etype = EBPFTypeFactory::instance->create(ftype);
        if (etype->is<EBPFScalarType>()) {
            auto scalarType = etype->to<EBPFScalarType>();
            unsigned readWordSize = scalarType->alignment() * 8;
            unsigned unaligned = scalarType->widthInBits() % readWordSize;
            unsigned padding = readWordSize - unaligned;
            if (padding == readWordSize) padding = 0;
            if (scalarType->widthInBits() + padding >= curr_padding) {
                curr_padding = padding;
            }
        }
    }

    builder->emitIndent();
    builder->appendFormat("if ((u8*)%s < %s + BYTES(%d + %u)) ", program->packetEndVar.c_str(),
                          program->headerStartVar.c_str(), width, curr_padding);
    builder->blockStart();

    builder->target->emitTraceMessage(builder, "Parser: invalid packet (packet too short)");

    builder->emitIndent();
    builder->appendFormat("%s = %s;", program->errorVar.c_str(), p4lib.packetTooShort.str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    builder->blockEnd(true);

    msgStr = Util::printf_format("Parser: extracting header %s", dest->toString());
    builder->target->emitTraceMessage(builder, msgStr.c_str());
    builder->newline();

    unsigned hdrOffsetBits = 0;
    /*
     * Some of the tests in this loop appear to be can't-happens.  For
     *  example, when varsize is not nil, there appears to be code
     *  elsewhere which (a) requires at least one varbit field in the
     *  header struct and (b) forbids multiple varbit fields in a header,
     *  so we will have exactly one varbit field.  I'm leaving the tests
     *  in both for the cases which aren't can't-happens and for the sake
     *  of firewalling in case code elsewhere changes such that the
     *  can't-happens actually can happen.
     */
    bool had_varbit;
    had_varbit = false;
    for (auto f : ht->fields) {
        auto ftype = state->parser->typeMap->getType(f);
        char *sizecode = 0;
        if (ftype->is<IR::Type_Varbits>()) {
            if (varsize == nullptr) {
                ::error(
                    ErrorType::ERR_INVALID,
                    "Extract to a header with a varbit<> member requires two-argument extract()");
                return;
            } else if (had_varbit) {
                ::error(ErrorType::ERR_INVALID,
                        "Two-argument extract() target must not have multiple varbit<> members");
                return;
            } else {
                sizecode = visit_to_string(varsize);
                builder->appendFormat("/* compileExtract varbit size = %s */", sizecode);
                builder->newline();
                had_varbit = true;
            }
        }
        auto etype = EBPFTypeFactory::instance->create(ftype);
        auto et = etype->to<IHasWidth>();
        if (et == nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Headers must use defined-widths types: %1%", f);
            return;
        }
        unsigned int advance = compileExtractField(dest, f, hdrOffsetBits, etype, sizecode);
        hdrOffsetBits += advance ? advance : et->widthInBits();
    }
    builder->newline();

    if (ht->is<IR::Type_Header>()) {
        builder->emitIndent();
        visit(dest);
        builder->appendLine(".ebpf_valid = 1;");
    }

    // Increment header pointer
    builder->emitIndent();
    builder->appendFormat("%s += BYTES(%u);", program->headerStartVar.c_str(), width);
    builder->newline();

    msgStr = Util::printf_format("Parser: extracted %s", dest->toString());
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    builder->newline();
}

void StateTranslationVisitor::processFunction(const P4::ExternFunction *function) {
    if (function->method->name.name == IR::ParserState::verify) {
        compileVerify(function->expr);
    } else {
        ::error(ErrorType::ERR_UNEXPECTED, "Unexpected extern function call in parser %1%",
                function->expr);
    }
}

void StateTranslationVisitor::processMethod(const P4::ExternMethod *method) {
    auto expression = method->expr;

    auto decl = method->object;
    if (decl == state->parser->packet) {
        if (method->method->name.name == p4lib.packetIn.extract.name) {
            switch (expression->arguments->size()) {
                case 1:
                    compileExtract(expression->arguments->at(0)->expression);
                    break;
                case 2:
                    compileExtract(expression->arguments->at(0)->expression,
                                   expression->arguments->at(1)->expression);
                    break;
                default:
                    // Can other size() values occur?
                    ::error(ErrorType::ERR_UNEXPECTED, "extract() with unexpected arg count %1%",
                            expression->arguments->size());
                    break;
            }
            return;
        } else if (method->method->name.name == p4lib.packetIn.length.name) {
            builder->append(state->parser->program->lengthVar);
            return;
        } else if (method->method->name.name == p4lib.packetIn.advance.name) {
            compileAdvance(method);
            return;
        }
        BUG("Unhandled packet method %1%", expression->method);
    }

    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected extern method call in parser %1%", expression);
}

bool StateTranslationVisitor::preorder(const IR::MethodCallExpression *expression) {
    if (commentDescriptionDepth == 0) builder->append("/* ");
    commentDescriptionDepth++;
    visit(expression->method);
    builder->append("(");
    bool first = true;
    for (auto a : *expression->arguments) {
        if (!first) builder->append(", ");
        first = false;
        visit(a);
    }
    builder->append(")");
    if (commentDescriptionDepth == 1) {
        builder->append(" */");
        builder->newline();
    }
    commentDescriptionDepth--;

    // do not process extern when comment is generated
    if (commentDescriptionDepth != 0) return false;

    auto mi = P4::MethodInstance::resolve(expression, state->parser->program->refMap,
                                          state->parser->program->typeMap);

    auto ef = mi->to<P4::ExternFunction>();
    if (ef != nullptr) {
        processFunction(ef);
        return false;
    }

    auto extMethod = mi->to<P4::ExternMethod>();
    if (extMethod != nullptr) {
        processMethod(extMethod);
        return false;
    }

    if (auto bim = mi->to<P4::BuiltInMethod>()) {
        builder->emitIndent();
        if (bim->name == IR::Type_Header::isValid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid");
            return false;
        } else if (bim->name == IR::Type_Header::setValid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid = true");
            return false;
        } else if (bim->name == IR::Type_Header::setInvalid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid = false");
            return false;
        }
    }

    ::error(ErrorType::ERR_UNEXPECTED, "Unexpected method call in parser %1%", expression);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::Member *expression) {
    if (expression->expr->is<IR::PathExpression>()) {
        auto pe = expression->expr->to<IR::PathExpression>();
        auto decl = state->parser->program->refMap->getDeclaration(pe->path, true);
        if (decl == state->parser->packet) {
            builder->append(expression->member);
            return false;
        }
    }

    return CodeGenInspector::preorder(expression);
}

//////////////////////////////////////////////////////////////////

EBPFParser::EBPFParser(const EBPFProgram *program, const IR::ParserBlock *block,
                       const P4::TypeMap *typeMap)
    : program(program),
      typeMap(typeMap),
      parserBlock(block),
      packet(nullptr),
      headers(nullptr),
      headerType(nullptr) {
    visitor = new StateTranslationVisitor(program->refMap, program->typeMap);
}

void EBPFParser::emitDeclaration(CodeBuilder *builder, const IR::Declaration *decl) {
    if (decl->is<IR::Declaration_Variable>()) {
        auto vd = decl->to<IR::Declaration_Variable>();
        auto etype = EBPFTypeFactory::instance->create(vd->type);
        builder->emitIndent();
        etype->declare(builder, vd->name, false);
        builder->endOfStatement(true);
        BUG_CHECK(vd->initializer == nullptr, "%1%: declarations with initializers not supported",
                  decl);
        return;
    } else if (decl->is<IR::P4ValueSet>()) {
        return;
    }
    BUG("%1%: not yet handled", decl);
}

void EBPFParser::emit(CodeBuilder *builder) {
    for (auto l : parserBlock->container->parserLocals) emitDeclaration(builder, l);

    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::start.c_str());
    builder->newline();

    visitor->setBuilder(builder);
    for (auto s : states) {
        visitor->setState(s);
        s->state->apply(*visitor);
    }

    builder->newline();

    // Create a synthetic reject state
    builder->emitIndent();
    builder->appendFormat("%s:", IR::ParserState::reject.c_str());
    builder->spc();
    builder->blockStart();

    // This state may be called from deparser, so do not explicitly tell source of this event.
    builder->target->emitTraceMessage(builder, "Packet rejected");

    emitRejectState(builder);

    builder->blockEnd(true);
    builder->newline();
}

bool EBPFParser::build() {
    auto pl = parserBlock->container->type->applyParams;
    if (pl->size() != 2) {
        ::error(ErrorType::ERR_EXPECTED, "Expected parser to have exactly 2 parameters");
        return false;
    }

    auto it = pl->parameters.begin();
    packet = *it;
    ++it;
    headers = *it;
    for (auto state : parserBlock->container->states) {
        auto ps = new EBPFParserState(state, this);
        states.push_back(ps);
    }

    auto ht = typeMap->getType(headers);
    if (ht == nullptr) return false;
    headerType = EBPFTypeFactory::instance->create(ht);

    for (auto decl : parserBlock->container->parserLocals) {
        if (decl->is<IR::P4ValueSet>()) {
            cstring extName = EBPFObject::externalName(decl);
            auto pvs = new EBPFValueSet(program, decl->to<IR::P4ValueSet>(), extName, visitor);
            valueSets.emplace(decl->name.name, pvs);
        }
    }

    return true;
}

void EBPFParser::emitTypes(CodeBuilder *builder) {
    for (auto pvs : valueSets) {
        pvs.second->emitTypes(builder);
    }
}

void EBPFParser::emitValueSetInstances(CodeBuilder *builder) {
    for (auto pvs : valueSets) {
        pvs.second->emitInstance(builder);
    }
}

void EBPFParser::emitRejectState(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("return %s;", builder->target->abortReturnCode().c_str());
    builder->newline();
}

}  // namespace EBPF
