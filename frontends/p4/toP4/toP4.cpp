#include <sstream>
#include "toP4.h"

namespace P4 {

Visitor::profile_t ToP4::init_apply(const IR::Node* node) {
    LOG4("Program dump:" << std::endl << dumpToString(node));
    return Inspector::init_apply(node);
}

// Try to guess whether a file is a "system" file
bool ToP4::isSystemFile(cstring file) {
    if (file.endsWith("core.p4")) return true;
    if (file.endsWith("model.p4")) return true;
    return false;
}

bool ToP4::preorder(const IR::P4Program* program) {
    std::set<cstring> includesEmitted;

    bool first = true;
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

void ToP4::end_apply(const IR::Node*) {
    if (outStream != nullptr) {
        cstring result = builder.toString();
        *outStream << result.c_str();
        outStream->flush();
    }
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
    std::function<void()> visitor = [this, t]() { visit(t->path); };
    visit_children(t, visitor);
    return false;
}

bool ToP4::preorder(const IR::Type_Stack* t) {
    visit(t->baseType);
    builder.append("[");
    visit(t->size);
    builder.append("]");
    return false;
}

bool ToP4::preorder(const IR::Type_Specialized* t) {
    visit(t->baseType);
    builder.append("<");
    setVecSep(", ");
    visit(t->arguments);
    doneVec();
    builder.append(">");
    return false;
}

bool ToP4::preorder(const IR::Type_Typedef* t) {
    builder.append("typedef ");
    visit(t->type);
    builder.spc();
    visit(t->annotations);
    builder.append(t->name);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Type_Enum* t) {
    builder.append("enum ");
    builder.append(t->name);
    builder.spc();
    builder.blockStart();
    bool first = true;
    for (auto a : *t->getEnumerator()) {
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
    const Context* ctx = getContext();
    bool standaloneFunction = ctx && (!ctx->parent ||
                                      !ctx->parent->node->is<IR::Type_Extern>());
    // standalone function declaration: not in a Vector of methods
    if (standaloneFunction)
        builder.append("extern ");

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

bool ToP4::preorder(const IR::Type_Extern* t) {
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
        ToP4 rec(builder);

        f->type->apply(rec);
        cstring t = builder.toString();
        if (t.size() > len)
            len = t.size();
        type.emplace(f, t);
    }

    for (auto f : *t->getEnumerator()) {
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

#define TYPE_CONSTRUCTED(T, N)                          \
    bool ToP4::preorder(const IR::Type_ ## T* t) {      \
builder.emitIndent();                                   \
visit(t->annotations);                                  \
builder.append(N " ");                                  \
builder.append(t->name);                                \
visit(t->typeParams);                                   \
visit(t->applyParams);                                  \
if (isDeclaration) builder.endOfStatement();            \
return false; }

TYPE_CONSTRUCTED(Parser, "parser")
TYPE_CONSTRUCTED(Control, "control")

#undef TYPE_CONSTRUCTED

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
    builder.append(b->toString());
    return false;
}

bool ToP4::preorder(const IR::StringLiteral* s) {
    builder.append(s->toString());
    return false;
}

bool ToP4::preorder(const IR::Declaration_Constant* cst) {
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
    visit(i->type);
    builder.append("(");
    setVecSep(", ");
    visit(i->arguments);
    doneVec();
    builder.append(")");
    builder.spc();
    visit(i->annotations);
    builder.append(i->name);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::Declaration_Variable* v) {
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
    builder.append("error ");
    builder.blockStart();
    bool first = true;
    for (auto a : *d->getEnumerator()) {
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

bool ToP4::preorder(const IR::Declaration_MatchKind* d) {
    builder.append("match_kind ");
    builder.blockStart();
    bool first = true;
    for (auto a : *d->getEnumerator()) {
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

///////////////////////////////////////////////////

#define VECTOR_VISIT(T)                                   \
    bool ToP4::preorder(const IR::Vector<IR::T> *v) {     \
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

VECTOR_VISIT(Type)
VECTOR_VISIT(Expression)
VECTOR_VISIT(Method)
VECTOR_VISIT(SelectCase)
VECTOR_VISIT(StatOrDecl)
VECTOR_VISIT(SwitchCase)
VECTOR_VISIT(Declaration)
VECTOR_VISIT(Node)
VECTOR_VISIT(ParserState)
VECTOR_VISIT(ActionListElement)
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

bool ToP4::preorder(const IR::PathExpression* p) {
    visit(p->path);
    return false;
}

bool ToP4::preorder(const IR::TypeNameExpression* e) {
    visit(e->typeName);
    return false;
}

bool ToP4::preorder(const IR::ConstructorCallExpression* e) {
    visit(e->type);
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
    visit(c->type);
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
    visit(a->left);
    builder.append(" = ");
    visit(a->right);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::BlockStatement* s) {
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(s->components);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::ExitStatement*) {
    builder.append("exit");
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::ReturnStatement*) {
    builder.append("return");
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::EmptyStatement*) {
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::IfStatement* s) {
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
    visit(v->expression);
    builder.endOfStatement();
    return false;
}

bool ToP4::preorder(const IR::ActionListElement* ale) {
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
    builder.blockStart();
    setVecSep(";\n", ";\n");
    visit(v->actionList);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool ToP4::preorder(const IR::Key* v) {
    builder.blockStart();

    std::map<const IR::KeyElement*, cstring> kf;
    size_t len = 0;
    for (auto f : *v->keyElements) {
        Util::SourceCodeBuilder builder;
        ToP4 rec(builder);

        f->expression->apply(rec);
        cstring s = builder.toString();
        if (s.size() > len)
            len = s.size();
        kf.emplace(f, s);
    }

    for (auto f : *v->keyElements) {
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
