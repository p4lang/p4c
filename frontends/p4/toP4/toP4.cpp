#include <sstream>
#include <string>
#include "toP4.h"

namespace P4 {

Visitor::profile_t ToP4::init_apply(const IR::Node* node) {
    LOG4("Program dump:" << std::endl << dumpToString(node));
    return Inspector::init_apply(node);
}

void ToP4::end_apply(const IR::Node*) {
    if (outStream != nullptr) {
        cstring result = builder.toString();
        *outStream << result.c_str();
        outStream->flush();
    }
}

// Try to guess whether a file is a "system" file
bool ToP4::isSystemFile(cstring file) {
    if (file.endsWith("core.p4")) return true;
    if (file.endsWith("model.p4")) return true;
    return false;
}

namespace {
class DumpIR : public Inspector {
    unsigned depth;
    std::stringstream str;

    DumpIR(unsigned depth, unsigned startDepth) :
            depth(depth) {
        for (unsigned i = 0; i < startDepth; i++)
            str << IndentCtl::indent;
    }
    void display(const IR::Node* node) {
        str << IndentCtl::endl;
        cstring tn = node->node_type_name();
        if (node->is<IR::Member>()) {
            node->Node::dbprint(str);
            str << node->to<IR::Member>()->member;
        } else if (node->is<IR::Constant>()) {
            node->Node::dbprint(str);
            str << " " << node;
        } else if (node->is<IR::Expression>() ||
                   node->is<IR::AssignmentStatement>() ||
                   node->is<IR::P4Action>() ||
                   tn.startsWith("Vector") ||
                   tn.startsWith("IndexedVector")) {
            node->Node::dbprint(str);
        } else {
            str << node;
        }
    }
    bool preorder(const IR::Node* node) override {
        if (depth == 0)
            return false;
        display(node);
        if (node->is<IR::Expression>())
            // increase depth limit for expressions.
            depth++;
        else
            depth--;
        str << IndentCtl::indent;
        return true;
    }
    void postorder(const IR::Node* node) override {
        if (node->is<IR::Expression>())
            depth--;
        else
            depth++;
        str << IndentCtl::unindent;
    }

 public:
    static std::string dump(const IR::Node* node, unsigned depth, unsigned startDepth) {
        DumpIR dumper(depth, startDepth);
        node->apply(dumper);
        return dumper.str.str();
    }
};
}  // namespace

unsigned ToP4::curDepth() const {
    unsigned result = 0;
    auto ctx = getContext();
    while (ctx != nullptr) {
        ctx = ctx->parent;
        result++;
    }
    return result;
}

void ToP4::dump(unsigned depth, const IR::Node* node, unsigned adjDepth) {
    if (!showIR)
        return;
    if (node == nullptr)
        node = getOriginal();

    auto str = DumpIR::dump(node, depth, adjDepth + curDepth());
    bool spc = builder.lastIsSpace();
    builder.commentStart();
    builder.append(str);
    builder.commentEnd();
    builder.newline();
    if (spc)
        // rather heuristic, but the output is very ugly anyway
        builder.emitIndent();
}

bool ToP4::preorder(const IR::P4Program* program) {
    std::set<cstring> includesEmitted;

    bool first = true;
    dump(2);
    for (auto a : *program->declarations) {
        // Check where this declaration originates
        if (!mainFile.isNullOrEmpty()) {
            if (a->srcInfo.isValid()) {
                unsigned line = a->srcInfo.getStart().getLineNumber();
                auto sfl = Util::InputSources::instance->getSourceLine(line);
                cstring sourceFile = sfl.fileName;
                if (sourceFile != mainFile && isSystemFile(sourceFile)) {
                    if (includesEmitted.find(sourceFile) == includesEmitted.end()) {
                        builder.append("#include \"");
                        builder.append(sourceFile);
                        builder.appendLine("\"");
                        includesEmitted.emplace(sourceFile);
                    }
                    first = false;
                    continue;
                }
            }
        }
        if (!first)
            builder.newline();
        first = false;
        visit(a);
    }
    if (!program->declarations->empty())
        builder.newline();
    return false;
}

bool ToP4::preorder(const IR::Type_Bits* t) {
    builder.appendFormat(t->toString());
    return false;
}

bool ToP4::preorder(const IR::Type_InfInt* t) {
    builder.appendFormat(t->toString());
    return false;
}

bool ToP4::preorder(const IR::Type_Var* t) {
    builder.append(t->name);
    return false;
}

bool ToP4::preorder(const IR::Type_Unknown*) {
    BUG("Cannot emit code for an unknown type");
    return false;
}

bool ToP4::preorder(const IR::Type_Dontcare*) {
    builder.append("_");
    return false;
}

bool ToP4::preorder(const IR::Type_Void*) {
    builder.append("void");
    return false;
}

bool ToP4::preorder(const IR::Type_Name* t) {
    visit(t->path);
    return false;
}

bool ToP4::preorder(const IR::Type_Stack* t) {
    dump(2);
    visit(t->baseType);
    builder.append("[");
    visit(t->size);
    builder.append("]");
    return false;
}

bool ToP4::preorder(const IR::Type_Specialized* t) {
    dump(3);
    visit(t->baseType);
    builder.append("<");
    setVecSep(", ");
    visit(t->arguments);
    doneVec();
    builder.append(">");
    return false;
}

bool ToP4::preorder(const IR::Type_Typedef* t) {
    dump(2);
    builder.append("typedef ");
    visit(t->type);
    builder.spc();
    visit(t->annotations);
    builder.append(t->name);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Type_Enum* t) {
    dump(1);
    builder.append("enum ");
    builder.append(t->name);
    builder.spc();
    builder.blockStart();
    bool first = true;
    for (auto a : *t->getDeclarations()) {
        dump(2, a->getNode(), 1);
        if (!first)
            builder.append(",\n");
        first = false;
        builder.emitIndent();
        builder.append(a->getName());
    }
    builder.newline();
    builder.blockEnd(true);
    return false;
}

bool ToP4::preorder(const IR::TypeParameters* t) {
    if (!t->empty()) {
        builder.append("<");
        bool first = true;
        for (auto a : *t->getEnumerator()) {
            if (!first)
                builder.append(", ");
            first = false;
            visit(a);
        }
        builder.append(">");
    }
    return false;
}

bool ToP4::preorder(const IR::Method* m) {
    dump(1);
    const Context* ctx = getContext();
    bool standaloneFunction = ctx && (!ctx->parent ||
                                      !ctx->parent->node->is<IR::Type_Extern>());
    // standalone function declaration: not in a Vector of methods
    if (standaloneFunction)
        builder.append("extern ");

    if (m->isAbstract)
        builder.append("abstract ");
    auto t = m->type;
    BUG_CHECK(t != nullptr, "Method %1% has no type", m);
    if (t->returnType != nullptr) {
        visit(t->returnType);
        builder.spc();
    }
    builder.append(m->name);
    visit(t->typeParameters);
    visit(t->parameters);
    if (standaloneFunction)
        builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Function* function) {
    dump(1);
    auto t = function->type;
    BUG_CHECK(t != nullptr, "Function %1% has no type", function);
    if (t->returnType != nullptr) {
        visit(t->returnType);
        builder.spc();
    }
    builder.append(function->name);
    visit(t->typeParameters);
    visit(t->parameters);
    builder.spc();
    visit(function->body);
    return false;
}

bool ToP4::preorder(const IR::Type_Extern* t) {
    dump(2);
    builder.append("extern ");
    builder.append(t->name);
    visit(t->typeParameters);
    builder.spc();
    builder.blockStart();

    setVecSep(";\n", ";\n");
    bool decl = isDeclaration;
    isDeclaration = true;
    visit(t->methods);
    isDeclaration = decl;
    doneVec();
    builder.blockEnd(true);
    return false;
}

bool ToP4::preorder(const IR::Type_Error*) {
    builder.append("error");
    return false;
}

bool ToP4::preorder(const IR::Type_Boolean*) {
    builder.append("bool");
    return false;
}

bool ToP4::preorder(const IR::Type_Varbits* t) {
    builder.appendFormat("varbit<%d>", t->size);
    return false;
}

bool ToP4::preorder(const IR::Type_Package* package) {
    dump(2);
    builder.emitIndent();
    visit(package->annotations);
    builder.append("package ");
    builder.append(package->name);
    visit(package->typeParams);
    visit(package->constructorParams);
    if (isDeclaration) builder.endOfStatement();
    return false;
}

bool ToP4::process(const IR::Type_StructLike* t, const char* name) {
    dump(1);
    builder.emitIndent();
    visit(t->annotations);
    builder.appendFormat("%s ", name);
    builder.append(t->name);
    builder.spc();
    builder.blockStart();

    std::map<const IR::StructField*, cstring> type;
    size_t len = 0;
    for (auto f : *t->getEnumerator()) {
        Util::SourceCodeBuilder builder;
        ToP4 rec(builder, showIR);

        f->type->apply(rec);
        cstring t = builder.toString();
        if (t.size() > len)
            len = t.size();
        type.emplace(f, t);
    }

    for (auto f : *t->getEnumerator()) {
        dump(2, f, 1);
        if (f->annotations->size() > 0) {
            builder.emitIndent();
            visit(f->annotations);
            builder.newline();
        }
        builder.emitIndent();
        cstring t = get(type, f);
        builder.append(t);
        size_t spaces = len + 1 - t.size();
        builder.append(std::string(spaces, ' '));
        builder.append(f->name);
        builder.endOfStatement(true);
    }

    builder.blockEnd(true);
    return false;
}

bool ToP4::preorder(const IR::Type_Parser* t) {
    dump(2);
    builder.emitIndent();
    visit(t->annotations);
    builder.append("parser ");
    builder.append(t->name);
    visit(t->typeParams);
    visit(t->applyParams);
    if (isDeclaration) builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Type_Control* t) {
    dump(2);
    builder.emitIndent();
    visit(t->annotations);
    builder.append("control ");
    builder.append(t->name);
    visit(t->typeParams);
    visit(t->applyParams);
    if (isDeclaration) builder.endOfStatement();
    return false;
}

///////////////////////

bool ToP4::preorder(const IR::Constant* c) {
    mpz_class value = c->value;
    const IR::Type_Bits* tb = dynamic_cast<const IR::Type_Bits*>(c->type);
    if (tb != nullptr) {
        mpz_class zero = 0;
        if (value < zero) {
            builder.append("-");
            value = -value;
        }
        builder.appendFormat("%d", tb->size);
        builder.append(tb->isSigned ? "s" : "w");
    }
    const char* repr = mpz_get_str(nullptr, c->base, value.get_mpz_t());
    switch (c->base) {
        case 2:
            builder.append("0b");
            break;
        case 8:
            builder.append("0o");
            break;
        case 16:
            builder.append("0x");
            break;
        case 10:
            break;
        default:
            BUG("%1%: Unexpected base %2%", c, c->base);
    }
    builder.append(repr);
    return false;
}

bool ToP4::preorder(const IR::BoolLiteral* b) {
    dump(1);
    builder.append(b->toString());
    return false;
}

bool ToP4::preorder(const IR::StringLiteral* s) {
    dump(1);
    builder.append(s->toString());
    return false;
}

bool ToP4::preorder(const IR::Declaration_Constant* cst) {
    dump(1);
    visit(cst->annotations);
    builder.append("const ");
    visit(cst->type);
    builder.spc();
    builder.append(cst->name);
    builder.append(" = ");

    setListTerm("{ ", " }");
    visit(cst->initializer);
    doneList();

    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Declaration_Instance* i) {
    dump(3);
    visit(i->type);
    builder.append("(");
    setVecSep(", ");
    visit(i->arguments);
    doneVec();
    builder.append(")");
    builder.spc();
    visit(i->annotations);
    builder.append(i->name);
    if (i->initializer != nullptr) {
        builder.append(" = ");
        visit(i->initializer);
    }
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Declaration_Variable* v) {
    dump(1);
    visit(v->annotations);
    visit(v->type);
    builder.spc();
    builder.append(v->name);
    if (v->initializer != nullptr) {
        builder.append(" = ");
        setListTerm("{ ", " }");
        visit(v->initializer);
        doneList();
    }
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Declaration_Errors* d) {
    dump(1);
    builder.append("error ");
    builder.blockStart();
    bool first = true;
    for (auto a : *d->getDeclarations()) {
        if (!first)
            builder.append(",\n");
        dump(1, a->getNode(), 1);
        first = false;
        builder.emitIndent();
        builder.append(a->getName());
    }
    builder.newline();
    builder.blockEnd(true);
    return false;
}

bool ToP4::preorder(const IR::Declaration_MatchKind* d) {
    dump(1);
    builder.append("match_kind ");
    builder.blockStart();
    bool first = true;
    for (auto a : *d->getDeclarations()) {
        if (!first)
            builder.append(",\n");
        dump(1, a->getNode(), 1);
        first = false;
        builder.emitIndent();
        builder.append(a->getName());
    }
    builder.newline();
    builder.blockEnd(true);
    return false;
}

///////////////////////////////////////////////////

#define VECTOR_VISIT(V, T)                                \
    bool ToP4::preorder(const IR:: V <IR::T> *v) {        \
    if (v == nullptr) return false;                       \
    bool first = true;                                    \
    VecPrint sep = getSep();                              \
    for (auto a : *v) {                                   \
        if (!first) {                                     \
            builder.append(sep.separator); }              \
        if (sep.separator.endsWith("\n")) {               \
            builder.emitIndent(); }                       \
        first = false;                                    \
        visit(a); }                                       \
    if (!v->empty() && !sep.terminator.isNullOrEmpty()) { \
        builder.append(sep.terminator); }                 \
return false; }

VECTOR_VISIT(Vector, Type)
VECTOR_VISIT(Vector, Expression)
VECTOR_VISIT(Vector, Method)
VECTOR_VISIT(Vector, SelectCase)
VECTOR_VISIT(IndexedVector, StatOrDecl)
VECTOR_VISIT(Vector, SwitchCase)
VECTOR_VISIT(Vector, Declaration)
VECTOR_VISIT(Vector, Node)
VECTOR_VISIT(Vector, ParserState)
VECTOR_VISIT(Vector, ActionListElement)
#undef VECTOR_VISIT

bool ToP4::preorder(const IR::Vector<IR::Annotation> *v) {
    if (v == nullptr) return false;
    for (auto a : *v) {
        visit(a);
        builder.spc();
    }
    return false;
}

///////////////////////////////////////////

bool ToP4::preorder(const IR::Slice* slice) {
    int prec = expressionPrecedence;
    bool useParens = prec > slice->getPrecedence();
    if (useParens)
        builder.append("(");
    expressionPrecedence = slice->getPrecedence();

    visit(slice->e0);
    builder.append("[");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(slice->e1);
    builder.append(":");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(slice->e2);
    builder.append("]");
    expressionPrecedence = prec;

    if (useParens)
        builder.append(")");
    return false;
}

bool ToP4::preorder(const IR::DefaultExpression*) {
    builder.append("default");
    return false;
}

bool ToP4::preorder(const IR::This*) {
    builder.append("this");
    return false;
}

bool ToP4::preorder(const IR::PathExpression* p) {
    visit(p->path);
    return false;
}

bool ToP4::preorder(const IR::TypeNameExpression* e) {
    visit(e->typeName);
    return false;
}

bool ToP4::preorder(const IR::ConstructorCallExpression* e) {
    visit(e->constructedType);
    builder.append("(");
    setVecSep(", ");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    visit(e->arguments);
    expressionPrecedence = prec;
    doneVec();
    builder.append(")");
    return false;
}

bool ToP4::preorder(const IR::Member* e) {
    int prec = expressionPrecedence;
    expressionPrecedence = e->getPrecedence();
    visit(e->expr);
    builder.append(".");
    builder.append(e->member);
    expressionPrecedence = prec;
    return false;
}

bool ToP4::preorder(const IR::SelectCase* e) {
    dump(2);
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    setListTerm("(", ")");
    visit(e->keyset);
    expressionPrecedence = prec;
    doneList();
    builder.append(": ");
    visit(e->state);
    return false;
}

bool ToP4::preorder(const IR::SelectExpression* e) {
    builder.append("select(");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    setListTerm("", "");
    visit(e->select);
    doneList();
    builder.append(") ");
    builder.blockStart();
    setVecSep(";\n", ";\n");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(e->selectCases);
    doneVec();
    builder.blockEnd(true);
    expressionPrecedence = prec;
    return false;
}

bool ToP4::preorder(const IR::ListExpression* e) {
    ListPrint *lp;
    if (listTerminators.empty())
        lp = new ListPrint("{ ", " }");
    else
        lp = &listTerminators.back();
    builder.append(lp->start);
    setVecSep(", ");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    visit(e->components);
    expressionPrecedence = prec;
    doneVec();
    builder.append(lp->end);
    return false;
}

bool ToP4::preorder(const IR::MethodCallExpression* e) {
    int prec = expressionPrecedence;
    bool useParens = (prec > DBPrint::Prec_Postfix) ||
    (!e->typeArguments->empty() &&
     getContext()->parent->node->is<IR::Expression>());
    // FIXME: we use parenthesis more often than necessary
    // because the bison parser has a bug which parses
    // these expressions incorrectly.
    expressionPrecedence = DBPrint::Prec_Postfix;
    if (useParens)
        builder.append("(");
    visit(e->method);
    if (!e->typeArguments->empty()) {
        builder.append("<");
        setVecSep(", ");
        visit(e->typeArguments);
        doneVec();
        builder.append(">");
    }
    builder.append("(");
    setVecSep(", ");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(e->arguments);
    doneVec();
    builder.append(")");
    if (useParens)
        builder.append(")");
    expressionPrecedence = prec;
    return false;
}

bool ToP4::preorder(const IR::Operation_Binary* b) {
    int prec = expressionPrecedence;
    bool useParens = prec > b->getPrecedence();
    if (useParens)
        builder.append("(");
    expressionPrecedence = b->getPrecedence();
    visit(b->left);
    builder.spc();
    builder.append(b->getStringOp());
    builder.spc();
    expressionPrecedence = b->getPrecedence() + 1;
    visit(b->right);
    if (useParens)
        builder.append(")");
    expressionPrecedence = prec;
    return false;
}

bool ToP4::preorder(const IR::Mux* b) {
    int prec = expressionPrecedence;
    bool useParens = prec >= b->getPrecedence();
    if (useParens)
        builder.append("(");
    expressionPrecedence = b->getPrecedence();
    visit(b->e0);
    builder.append(" ? ");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(b->e1);
    builder.append(" : ");
    expressionPrecedence = b->getPrecedence();
    visit(b->e2);
    expressionPrecedence = prec;
    if (useParens)
        builder.append(")");
    return false;
}

bool ToP4::preorder(const IR::Operation_Unary* u) {
    int prec = expressionPrecedence;
    bool useParens = prec > u->getPrecedence();
    if (useParens)
        builder.append("(");
    builder.append(u->getStringOp());
    expressionPrecedence = u->getPrecedence();
    visit(u->expr);
    expressionPrecedence = prec;
    if (useParens)
        builder.append(")");
    return false;
}

bool ToP4::preorder(const IR::ArrayIndex* a) {
    int prec = expressionPrecedence;
    bool useParens = prec > a->getPrecedence();
    if (useParens)
        builder.append("(");
    expressionPrecedence = a->getPrecedence();
    visit(a->left);
    builder.append("[");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(a->right);
    builder.append("]");
    if (useParens)
        builder.append(")");
    expressionPrecedence = prec;
    return false;
}

bool ToP4::preorder(const IR::Cast* c) {
    int prec = expressionPrecedence;
    bool useParens = prec > c->getPrecedence();
    if (useParens)
        builder.append("(");
    builder.append("(");
    visit(c->destType);
    builder.append(")");
    expressionPrecedence = c->getPrecedence();
    visit(c->expr);
    if (useParens)
        builder.append(")");
    expressionPrecedence = prec;
    return false;
}

//////////////////////////////////////////////////////////

bool ToP4::preorder(const IR::AssignmentStatement* a) {
    dump(2);
    visit(a->left);
    builder.append(" = ");
    visit(a->right);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::BlockStatement* s) {
    dump(1);
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(s->components);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::ExitStatement*) {
    dump(1);
    builder.append("exit");
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::ReturnStatement* statement) {
    dump(2);
    builder.append("return");
    if (statement->expression != nullptr) {
        builder.spc();
        visit(statement->expression);
    }
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::EmptyStatement*) {
    dump(1);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::IfStatement* s) {
    dump(1);
    builder.append("if (");
    visit(s->condition);
    builder.append(") ");
    if (!s->ifTrue->is<IR::BlockStatement>()) {
        builder.increaseIndent();
        builder.newline();
        builder.emitIndent();
    }
    visit(s->ifTrue);
    if (!s->ifTrue->is<IR::BlockStatement>())
        builder.decreaseIndent();
    if (s->ifFalse != nullptr) {
        builder.newline();
        builder.emitIndent();
        builder.append("else ");
        if (!s->ifFalse->is<IR::BlockStatement>()) {
            builder.increaseIndent();
            builder.newline();
            builder.emitIndent();
        }
        visit(s->ifFalse);
        if (!s->ifFalse->is<IR::BlockStatement>())
            builder.decreaseIndent();
    }
    return false;
}

bool ToP4::preorder(const IR::MethodCallStatement* s) {
    dump(3);
    visit(s->methodCall);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::SwitchCase* s) {
    visit(s->label);
    builder.append(": ");
    visit(s->statement);
    return false;
}

bool ToP4::preorder(const IR::SwitchStatement* s) {
    dump(4);
    builder.append("switch (");
    visit(s->expression);
    builder.append(") ");
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(s->cases);
    doneVec();
    builder.blockEnd(true);
    return false;
}

////////////////////////////////////

bool ToP4::preorder(const IR::Annotation * a) {
    builder.append("@");
    builder.append(a->name);
    if (a->expr != nullptr) {
        builder.append("(");
        visit(a->expr);
        builder.append(")");
    }
    return false;
}

bool ToP4::preorder(const IR::Parameter* p) {
    dump(1);
    visit(p->annotations);
    switch (p->direction) {
        case IR::Direction::None:
            break;
        case IR::Direction::In:
            builder.append("in ");
            break;
        case IR::Direction::Out:
            builder.append("out ");
            break;
        case IR::Direction::InOut:
            builder.append("inout ");
            break;
        default:
            BUG("Unexpected case");
    }
    visit(p->type);
    builder.spc();
    builder.append(p->name);
    return false;
}

bool ToP4::preorder(const IR::P4Control * c) {
    dump(1);
    bool decl = isDeclaration;
    isDeclaration = false;
    visit(c->type);
    isDeclaration = decl;
    if (c->constructorParams->size() != 0)
        visit(c->constructorParams);
    builder.spc();
    builder.blockStart();
    for (auto s : *c->statefulEnumerator()) {
        builder.emitIndent();
        visit(s);
        builder.newline();
    }

    builder.emitIndent();
    builder.append("apply ");
    visit(c->body);
    builder.newline();
    builder.blockEnd(true);
    return false;
}

bool ToP4::preorder(const IR::ParameterList* p) {
    builder.append("(");
    bool first = true;
    for (auto param : *p->getEnumerator()) {
        if (!first)
            builder.append(", ");
        first = false;
        visit(param);
    }
    builder.append(")");
    return false;
}

bool ToP4::preorder(const IR::P4Action * c) {
    dump(2);
    visit(c->annotations);
    builder.append("action ");
    builder.append(c->name);
    visit(c->parameters);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(c->body);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::ParserState* s) {
    dump(1);
    if (s->isBuiltin()) return false;

    visit(s->annotations);
    builder.append("state ");
    builder.append(s->name);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(s->components);
    doneVec();

    if (s->selectExpression != nullptr) {
        dump(2, s->selectExpression, 1);
        builder.emitIndent();
        builder.append("transition ");
        visit(s->selectExpression);
        if (!s->selectExpression->is<IR::SelectExpression>()) {
            builder.endOfStatement();
            builder.newline();
        }
    }
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::P4Parser * c) {
    dump(1);
    bool decl = isDeclaration;
    isDeclaration = false;
    visit(c->type);
    isDeclaration = decl;
    if (c->constructorParams->size() != 0)
        visit(c->constructorParams);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(c->stateful);
    doneVec();
    // explicit visit of parser states
    for (auto s : *c->states) {
        if (s->isBuiltin()) continue;
        builder.emitIndent();
        visit(s);
        builder.append("\n");
    }
    builder.blockEnd(true);
    return false;
}

bool ToP4::preorder(const IR::ExpressionValue* v) {
    dump(2);
    visit(v->expression);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::ActionListElement* ale) {
    dump(3);
    visit(ale->annotations);
    visit(ale->name);
    if (ale->arguments != nullptr) {
        builder.append("(");
        setVecSep(", ");
        visit(ale->arguments);
        doneVec();
        builder.append(")");
    }
    return false;
}

bool ToP4::preorder(const IR::ActionList* v) {
    dump(2);
    builder.blockStart();
    setVecSep(";\n", ";\n");
    visit(v->actionList);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::Key* v) {
    dump(2);
    builder.blockStart();

    std::map<const IR::KeyElement*, cstring> kf;
    size_t len = 0;
    for (auto f : *v->keyElements) {
        Util::SourceCodeBuilder builder;
        ToP4 rec(builder, showIR);

        f->expression->apply(rec);
        cstring s = builder.toString();
        if (s.size() > len)
            len = s.size();
        kf.emplace(f, s);
    }

    for (auto f : *v->keyElements) {
        dump(2, f, 2);
        builder.emitIndent();
        cstring s = get(kf, f);
        builder.append(s);
        size_t spaces = len - s.size();
        builder.append(std::string(spaces, ' '));
        builder.append(": ");
        visit(f->matchType);
        builder.endOfStatement(true);
    }
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::TableProperty* p) {
    dump(1);
    visit(p->annotations);
    if (p->isConstant)
        builder.append("const ");
    builder.append(p->name);
    builder.append(" = ");
    visit(p->value);
    return false;
}

bool ToP4::preorder(const IR::TableProperties* t) {
    for (auto p : *t->getEnumerator()) {
        builder.emitIndent();
        visit(p);
        builder.newline();
    }
    return false;
}

bool ToP4::preorder(const IR::P4Table* c) {
    dump(2);
    visit(c->annotations);
    builder.append("table ");
    builder.append(c->name);
    visit(c->parameters);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(c->properties);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::Path* p) {
    if (p->prefix != nullptr)
        builder.append(p->prefix->toString());
    builder.append(p->name);
    return false;
}
}  // namespace P4
