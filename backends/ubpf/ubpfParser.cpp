#include "ubpfParser.h"
#include "ubpfType.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
//#include "string.h"

namespace UBPF {

    namespace {
        class UBPFStateTranslationVisitor : public EBPF::CodeGenInspector {
            bool hasDefault;
            P4::P4CoreLibrary& p4lib;
            const UBPFParserState* state;

            void compileExtractField(const IR::Expression* expr, cstring name,
                                     unsigned alignment, EBPF::EBPFType* type);
            void compileExtract(const IR::Expression* destination);
            void compileLookahead(const IR::Expression* destination);

        public:
            static cstring currentHeader; // "pkt" is an initial state
            explicit UBPFStateTranslationVisitor(const UBPFParserState* state) :
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
    }

    cstring UBPFStateTranslationVisitor::currentHeader = "pkt";

    bool UBPFStateTranslationVisitor::preorder(const IR::ParserState* parserState) {
        std::cout << "Visitor: ParserState." << std::endl;
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

    bool UBPFStateTranslationVisitor::preorder(const IR::SelectCase* selectCase) {
        std::cout << "Visitor: SelectCase." << std::endl;
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

    bool UBPFStateTranslationVisitor::preorder(const IR::SelectExpression* expression) {
        std::cout << "Visitor: SelectExpression." << std::endl;
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

    bool UBPFStateTranslationVisitor::preorder(const IR::Member* expression) {
        std::cout << "Visitor: Member." << std::endl;
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

    void cstringToLower(const cstring value, char* result) {
        strcpy(result, value);
        for(int i = 0; result[i]; i++){
            result[i] = tolower(result[i]);
        }
        std::cout << result << std::endl;
    }

//    void
//    UBPFStateTranslationVisitor::compileExtractField(
//            const IR::Expression* expr, cstring field, unsigned alignment, EBPFType* type) {
//        unsigned widthToExtract = dynamic_cast<IHasWidth*>(type)->widthInBits();
//        auto program = state->parser->program;
//
//
//    }

    void
    UBPFStateTranslationVisitor::compileExtract(const IR::Expression* destination) {
        std::cout << "Visitor: Expression." << std::endl;
        auto type = state->parser->typeMap->getType(destination);
        auto ht = type->to<IR::Type_StructLike>();

        if (ht == nullptr) {
            ::error("Cannot extract to a non-struct type %1%", destination);
            return;
        }

        unsigned width = ht->width_bits();
        auto program = state->parser->program;
        builder->emitIndent();
        // TODO: Check if packet is not too short
        char headerName[strlen(ht->name.name)];
        cstringToLower(ht->name.name, headerName);
        if (currentHeader == "pkt")
            builder->appendFormat("struct %s *%s = (void *)%s",  ht->name.name, headerName, currentHeader);
        else
            builder->appendFormat("struct %s *%s = (void *)(%s + 1)",  ht->name.name, headerName, currentHeader);

        UBPFStateTranslationVisitor::currentHeader = headerName;
        std::cout << UBPFStateTranslationVisitor::currentHeader << std::endl;

        builder->endOfStatement(true);
        builder->newline();

//        unsigned alignment = 0;
//        for (auto f : ht->fields) {
//            auto ftype = state->parser->typeMap->getType(f);
//            auto etype = UBPFTypeFactory::instance->create(ftype);
//            auto et = dynamic_cast<IHasWidth*>(etype);
//            if (et == nullptr) {
//                ::error("Only headers with fixed widths supported %1%", f);
//                return;
//            }
//            compileExtractField(destination, f->name, alignment, etype);
//            alignment += et->widthInBits();
//            alignment %= 8;
//        }

        if (ht->is<IR::Type_Header>()) {
            builder->emitIndent();
            visit(destination);
            builder->appendLine(".ebpf_valid = 1;");
        }
    }

    bool UBPFStateTranslationVisitor::preorder(const IR::MethodCallExpression* expression) {
        builder->emitIndent();
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

    bool UBPFStateTranslationVisitor::preorder(const IR::AssignmentStatement* stat) {
        return false;
    }

    void UBPFParserState::emit(EBPF::CodeBuilder* builder) {
        std::cout << "Emitting UBPFParserState." << std::endl;
        UBPFStateTranslationVisitor visitor(this);
        visitor.setBuilder(builder);
        state->apply(visitor);
    }

    void UBPF::UBPFParser::emit(EBPF::CodeBuilder *builder) {
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

    bool UBPFParser::build() {
        auto pl = parserBlock->container->type->applyParams;
        if (pl->size() != 2) {
            ::error("Expected parser to have exactly 2 parameters");
            return false;
        }

        auto it = pl->parameters.begin();
        packet = *it; ++it;
        headers = *it;
        for (auto state : parserBlock->container->states) {
            auto ps = new UBPFParserState(state, this);
            states.push_back(ps);
        }

        auto ht = typeMap->getType(headers);
        if (ht == nullptr)
            return false;
        headerType = UBPFTypeFactory::instance->create(ht);
        return true;
    }

}