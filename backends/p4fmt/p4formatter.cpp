#include "p4formatter.h"

#include <deque>
#include <sstream>
#include <string>

#include "frontends/common/options.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/parsers/p4/p4parser.hpp"
#include "ir/dump.h"

namespace P4::P4Fmt {

Visitor::profile_t P4Formatter::init_apply(const IR::Node *node) {
    LOG4("Program dump:" << std::endl << dumpToString(node));
    listTerminators_init_apply_size = listTerminators.size();
    vectorSeparator_init_apply_size = vectorSeparator.size();
    return Inspector::init_apply(node);
}

void P4Formatter::end_apply(const IR::Node *) {
    if (outStream != nullptr) {
        cstring result = builder.toString();
        *outStream << result;
        outStream->flush();
    }
    BUG_CHECK(listTerminators.size() == listTerminators_init_apply_size,
              "inconsistent listTerminators");
    BUG_CHECK(vectorSeparator.size() == vectorSeparator_init_apply_size,
              "inconsistent vectorSeparator");
}

// Try to guess whether a file is a "system" file
bool P4Formatter::isSystemFile(cstring file) {
    if (noIncludes) return false;
    if (file.startsWith(p4includePath)) return true;
    return false;
}

cstring P4Formatter::ifSystemFile(const IR::Node *node) {
    if (!node->srcInfo.isValid()) return nullptr;
    auto sourceFile = node->srcInfo.getSourceFile();
    if (isSystemFile(sourceFile)) return sourceFile;
    return nullptr;
}

bool P4Formatter::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Unhandled IR node type: ", node->node_type_name());
    return false;
}

bool P4Formatter::preorder(const IR::P4Program *program) {
    std::unordered_set<cstring> includesEmitted;

    bool first = true;
    for (auto a : program->objects) {
        // Check where this declaration originates
        cstring sourceFile = ifSystemFile(a);
        if (!a->is<IR::Type_Error>() &&  // errors can come from multiple files
            sourceFile != nullptr) {
            /* FIXME -- when including a user header file (sourceFile !=
             * mainFile), do we want to emit an #include of it or not?  Probably
             * not when translating from P4-14, as that would create a P4-16
             * file that tries to include a P4-14 header.  Unless we want to
             * allow converting headers independently (is that even possible?).
             * For now we ignore mainFile and don't emit #includes for any
             * non-system header */

            if (includesEmitted.count(sourceFile) == 0) {
                if (sourceFile.startsWith(p4includePath)) {
                    const char *p = sourceFile.c_str() + strlen(p4includePath);
                    if (*p == '/') p++;
                    if (P4V1::V1Model::instance.file.name == p) {
                        P4V1::getV1ModelVersion g;
                        program->apply(g);
                        builder.append("#define V1MODEL_VERSION ");
                        builder.append(g.version);
                        builder.newline();
                    }
                    builder.append("#include <");
                    builder.append(p);
                    builder.append(">");
                    builder.newline();
                } else {
                    builder.append("#include \"");
                    builder.append(sourceFile);
                    builder.append("\"");
                    builder.newline();
                }
                includesEmitted.emplace(sourceFile);
            }
            first = false;
            continue;
        }
        if (!first) builder.newline();
        first = false;
        visit(a);
    }
    if (!program->objects.empty()) builder.newline();
    return false;
}

bool P4Formatter::preorder(const IR::Type_Bits *t) {
    if (t->expression) {
        builder.append("bit<(");
        visit(t->expression);
        builder.append(")>");
    } else {
        builder.append(t->toString());
    }
    return false;
}

bool P4Formatter::preorder(const IR::Type_String *t) {
    builder.append(t->toString());
    return false;
}

bool P4Formatter::preorder(const IR::Type_InfInt *t) {
    builder.append(t->toString());
    return false;
}

bool P4Formatter::preorder(const IR::Type_Var *t) {
    builder.append(t->name);
    return false;
}

bool P4Formatter::preorder(const IR::Type_Unknown *) {
    BUG("Cannot emit code for an unknown type");
    // builder.append("*unknown type*");
    return false;
}

bool P4Formatter::preorder(const IR::Type_Dontcare *) {
    builder.append("_");
    return false;
}

bool P4Formatter::preorder(const IR::Type_Void *) {
    builder.append("void");
    return false;
}

bool P4Formatter::preorder(const IR::Type_Name *t) {
    visit(t->path);
    return false;
}

bool P4Formatter::preorder(const IR::Type_Stack *t) {
    visit(t->elementType);
    builder.append("[");
    visit(t->size);
    builder.append("]");
    return false;
}

bool P4Formatter::preorder(const IR::Type_Specialized *t) {
    visit(t->baseType);
    builder.append("<");
    setVecSep(", ");
    visit(t->arguments);
    doneVec();
    builder.append(">");
    return false;
}

bool P4Formatter::preorder(const IR::Argument *arg) {
    if (!arg->name.name.isNullOrEmpty()) {
        builder.append(arg->name.name);
        builder.append(" = ");
    }
    visit(arg->expression);
    return false;
}

bool P4Formatter::preorder(const IR::Type_Typedef *t) {
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("typedef ");
    visit(t->type);
    builder.spc();
    builder.append(t->name);
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Type_Newtype *t) {
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("type ");
    visit(t->type);
    builder.spc();
    builder.append(t->name);
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Type_BaseList *t) {
    builder.append("tuple<");
    visitCollection(t->components, ", ", [&](const auto *a) {
        auto p4type = a->getP4Type();
        CHECK_NULL(p4type);
        visit(p4type);
    });
    builder.append(">");
    return false;
}

bool P4Formatter::preorder(const IR::P4ValueSet *t) {
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("value_set<");
    auto p4type = t->elementType->getP4Type();
    CHECK_NULL(p4type);
    visit(p4type);
    builder.append(">");
    builder.append("(");
    visit(t->size);
    builder.append(")");
    builder.spc();
    builder.append(t->name);
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Type_Enum *t) {
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("enum ");
    builder.append(t->name);
    builder.spc();
    builder.blockStart();
    visitCollection(*t->getDeclarations(), ",\n", [this](const auto &a) {
        builder.emitIndent();
        builder.append(a->getName());
    });
    builder.newline();
    builder.blockEnd(true);
    return false;
}

bool P4Formatter::preorder(const IR::Type_SerEnum *t) {
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("enum ");
    visit(t->type);
    builder.spc();
    builder.append(t->name);
    builder.spc();
    builder.blockStart();
    visitCollection(t->members, ",\n", [&](const auto &a) {
        builder.emitIndent();
        builder.append(a->getName());
        builder.append(" = ");
        visit(a->value);
    });
    builder.newline();
    builder.blockEnd(true);
    return false;
}

bool P4Formatter::preorder(const IR::TypeParameters *t) {
    if (!t->empty()) {
        builder.append("<");
        bool decl = isDeclaration;
        isDeclaration = false;
        visitCollection(t->parameters, ", ", [&](const auto *a) { visit(a); });
        isDeclaration = decl;
        builder.append(">");
    }
    return false;
}

bool P4Formatter::preorder(const IR::Method *m) {
    if (!m->annotations->annotations.empty()) {
        visit(m->annotations);
        builder.spc();
    }
    const Context *ctx = getContext();
    bool standaloneFunction = !ctx || !ctx->node->is<IR::Type_Extern>();
    // standalone function declaration: not in a Vector of methods
    if (standaloneFunction) builder.append("extern ");

    if (m->isAbstract) builder.append("abstract ");
    auto t = m->type;
    BUG_CHECK(t != nullptr, "Method %1% has no type", m);
    if (t->returnType != nullptr) {
        visit(t->returnType);
        builder.spc();
    }
    builder.append(m->name);
    visit(t->typeParameters);
    visit(t->parameters);
    if (standaloneFunction) builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Function *function) {
    if (!function->annotations->annotations.empty()) {
        visit(function->annotations);
        builder.spc();
    }
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

bool P4Formatter::preorder(const IR::Type_Extern *t) {
    if (isDeclaration) {
        if (!t->annotations->annotations.empty()) {
            visit(t->annotations);
            builder.spc();
        }
        builder.append("extern ");
    }
    builder.append(t->name);
    visit(t->typeParameters);
    if (!isDeclaration) return false;
    builder.spc();
    builder.blockStart();

    if (t->attributes.size() != 0)
        warn(ErrorType::WARN_UNSUPPORTED,
             "%1%: extern has attributes, which are not supported "
             "in P4-16, and thus are not emitted as P4-16",
             t);

    setVecSep(";\n", ";\n");
    bool decl = isDeclaration;
    isDeclaration = true;
    preorder(&t->methods);
    isDeclaration = decl;
    doneVec();
    builder.blockEnd(true);
    return false;
}

bool P4Formatter::preorder(const IR::Type_Boolean *) {
    builder.append("bool");
    return false;
}

bool P4Formatter::preorder(const IR::Type_Varbits *t) {
    if (t->expression) {
        builder.append("varbit<(");
        visit(t->expression);
        builder.append(")>");
    } else {
        builder.appendFormat("varbit<%d>", t->size);
    }
    return false;
}

bool P4Formatter::preorder(const IR::Type_Package *package) {
    builder.emitIndent();
    if (!package->annotations->annotations.empty()) {
        visit(package->annotations);
        builder.spc();
    }
    builder.append("package ");
    builder.append(package->name);
    visit(package->typeParameters);
    visit(package->constructorParams);
    if (isDeclaration) builder.endOfStatement();
    return false;
}

bool P4Formatter::process(const IR::Type_StructLike *t, const char *name) {
    if (isDeclaration) {
        builder.emitIndent();
        if (!t->annotations->annotations.empty()) {
            visit(t->annotations);
            builder.spc();
        }
        builder.appendFormat("%s ", name);
    }
    builder.append(t->name);
    visit(t->typeParameters);
    if (!isDeclaration) return false;
    builder.spc();
    builder.blockStart();

    std::unordered_map<const IR::StructField *, cstring> type;
    size_t len = 0;
    for (auto f : t->fields) {
        Util::SourceCodeBuilder builder;
        P4Formatter rec(builder);

        f->type->apply(rec);
        cstring t = builder.toString();
        if (t.size() > len) len = t.size();
        type.emplace(f, t);
    }

    for (auto f : t->fields) {
        if (f->annotations->size() > 0) {
            builder.emitIndent();
            if (!f->annotations->annotations.empty()) {
                visit(f->annotations);
            }
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

bool P4Formatter::preorder(const IR::Type_Parser *t) {
    builder.emitIndent();
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("parser ");
    builder.append(t->name);
    visit(t->typeParameters);
    visit(t->applyParams);
    if (isDeclaration) builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Type_Control *t) {
    builder.emitIndent();
    if (!t->annotations->annotations.empty()) {
        visit(t->annotations);
        builder.spc();
    }
    builder.append("control ");
    builder.append(t->name);
    visit(t->typeParameters);
    visit(t->applyParams);
    if (isDeclaration) builder.endOfStatement();
    return false;
}

///////////////////////

bool P4Formatter::preorder(const IR::Constant *c) {
    const IR::Type_Bits *tb = c->type->to<IR::Type_Bits>();
    unsigned width;
    bool sign;
    if (tb == nullptr) {
        width = 0;
        sign = false;
    } else {
        width = tb->size;
        sign = tb->isSigned;
    }
    cstring s = Util::toString(c->value, width, sign, c->base);
    builder.append(s);
    return false;
}

bool P4Formatter::preorder(const IR::BoolLiteral *b) {
    builder.append(b->toString());
    return false;
}

bool P4Formatter::preorder(const IR::StringLiteral *s) {
    builder.append(s->toString());
    return false;
}

bool P4Formatter::preorder(const IR::Declaration_Constant *cst) {
    if (!cst->annotations->annotations.empty()) {
        visit(cst->annotations);
        builder.spc();
    }
    builder.append("const ");
    auto type = cst->type->getP4Type();
    CHECK_NULL(type);
    visit(type);
    builder.spc();
    builder.append(cst->name);
    builder.append(" = ");

    setListTerm("{ ", " }");
    visit(cst->initializer);
    doneList();

    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Declaration_Instance *i) {
    if (!i->annotations->annotations.empty()) {
        visit(i->annotations);
        builder.spc();
    }
    auto type = i->type->getP4Type();
    CHECK_NULL(type);
    visit(type);
    builder.append("(");
    setVecSep(", ");
    visit(i->arguments);
    doneVec();
    builder.append(")");
    builder.spc();
    builder.append(i->name);
    if (i->initializer != nullptr) {
        builder.append(" = ");
        visit(i->initializer);
    }
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::Declaration_Variable *v) {
    if (!v->annotations->annotations.empty()) {
        visit(v->annotations);
        builder.spc();
    }
    auto type = v->type->getP4Type();
    CHECK_NULL(type);
    visit(type);
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

bool P4Formatter::preorder(const IR::Type_Error *d) {
    bool first = true;
    for (auto a : *d->getDeclarations()) {
        if (ifSystemFile(a->getNode()))
            // only print if not from a system file
            continue;
        if (!first) {
            builder.append(",\n");
        } else {
            builder.append("error ");
            builder.blockStart();
        }
        first = false;
        builder.emitIndent();
        builder.append(a->getName());
    }
    if (!first) {
        builder.newline();
        builder.blockEnd(true);
    }
    return false;
}

bool P4Formatter::preorder(const IR::Declaration_MatchKind *d) {
    builder.append("match_kind ");
    builder.blockStart();
    visitCollection(*d->getDeclarations(), ",\n", [this](const auto &a) {
        builder.emitIndent();
        builder.append(a->getName());
    });
    builder.newline();
    builder.blockEnd(true);
    return false;
}

///////////////////////////////////////////////////

#define VECTOR_VISIT(V, T)                                    \
    bool P4Formatter::preorder(const IR::V<IR::T> *v) {       \
        if (v == nullptr) return false;                       \
        bool first = true;                                    \
        VecPrint sep = getSep();                              \
        for (auto a : *v) {                                   \
            if (!first) {                                     \
                builder.append(sep.separator);                \
            }                                                 \
            if (sep.separator.endsWith("\n")) {               \
                builder.emitIndent();                         \
            }                                                 \
            first = false;                                    \
            visit(a);                                         \
        }                                                     \
        if (!v->empty() && !sep.terminator.isNullOrEmpty()) { \
            builder.append(sep.terminator);                   \
        }                                                     \
        return false;                                         \
    }

VECTOR_VISIT(Vector, ActionListElement)
VECTOR_VISIT(Vector, Annotation)
VECTOR_VISIT(Vector, Entry)
VECTOR_VISIT(Vector, Expression)
VECTOR_VISIT(Vector, Argument)
VECTOR_VISIT(Vector, KeyElement)
VECTOR_VISIT(Vector, Method)
VECTOR_VISIT(Vector, Node)
VECTOR_VISIT(Vector, SelectCase)
VECTOR_VISIT(Vector, SwitchCase)
VECTOR_VISIT(Vector, Type)
VECTOR_VISIT(IndexedVector, Declaration)
VECTOR_VISIT(IndexedVector, Declaration_ID)
VECTOR_VISIT(IndexedVector, Node)
VECTOR_VISIT(IndexedVector, ParserState)
VECTOR_VISIT(IndexedVector, StatOrDecl)

#undef VECTOR_VISIT

///////////////////////////////////////////

bool P4Formatter::preorder(const IR::Slice *slice) {
    int prec = expressionPrecedence;
    bool useParens = prec > slice->getPrecedence();
    if (useParens) builder.append("(");
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

    if (useParens) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::DefaultExpression *) {
    // Within a method call this is rendered as a don't care
    if (withinArgument)
        builder.append("_");
    else
        builder.append("default");
    return false;
}

bool P4Formatter::preorder(const IR::This *) {
    builder.append("this");
    return false;
}

bool P4Formatter::preorder(const IR::PathExpression *p) {
    visit(p->path);
    return false;
}

bool P4Formatter::preorder(const IR::TypeNameExpression *e) {
    visit(e->typeName);
    return false;
}

bool P4Formatter::preorder(const IR::ConstructorCallExpression *e) {
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

bool P4Formatter::preorder(const IR::Member *e) {
    int prec = expressionPrecedence;
    expressionPrecedence = e->getPrecedence();
    visit(e->expr);
    builder.append(".");
    builder.append(e->member);
    expressionPrecedence = prec;
    return false;
}

bool P4Formatter::preorder(const IR::SelectCase *e) {
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

bool P4Formatter::preorder(const IR::SelectExpression *e) {
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
    preorder(&e->selectCases);
    doneVec();
    builder.blockEnd(true);
    expressionPrecedence = prec;
    return false;
}

bool P4Formatter::preorder(const IR::ListExpression *e) {
    using namespace P4::literals;
    auto [start, end] = listTerminators.empty() ? std::make_pair("{ "_cs, " }"_cs)
                                                : std::make_pair(listTerminators.back().start,
                                                                 listTerminators.back().end);
    builder.append(start);
    setVecSep(", ");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    setListTerm("{ ", " }");
    preorder(&e->components);
    doneList();
    expressionPrecedence = prec;
    doneVec();
    builder.append(end);
    return false;
}

bool P4Formatter::preorder(const IR::P4ListExpression *e) {
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append("(");
    if (e->elementType != nullptr) {
        builder.append("(list<");
        visit(e->elementType->getP4Type());
        builder.append(">)");
    }
    builder.append("{");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    visitCollection(e->components, ",", [&](const auto &c) { visit(c); });
    expressionPrecedence = prec;
    builder.append("}");
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::NamedExpression *e) {
    builder.append(e->name.name);
    builder.append(" = ");
    visit(e->expression);
    return false;
}

bool P4Formatter::preorder(const IR::StructExpression *e) {
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append("(");
    if (e->structType != nullptr) {
        builder.append("(");
        visit(e->structType);
        builder.append(")");
    }
    builder.append("{");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    visitCollection(e->components, ",", [&](const auto &c) { visit(c); });
    expressionPrecedence = prec;
    builder.append("}");
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::HeaderStackExpression *e) {
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append("(");
    if (e->headerStackType != nullptr) {
        builder.append("(");
        visit(e->headerStackType);
        builder.append(")");
    }
    builder.append("{");
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    visitCollection(e->components, ",", [&](const auto &c) { visit(c); });
    expressionPrecedence = prec;
    builder.append("}");
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::Invalid *) {
    builder.append("{#}");
    return false;
}

bool P4Formatter::preorder(const IR::Dots *) {
    builder.append("...");
    return false;
}

bool P4Formatter::preorder(const IR::NamedDots *) {
    builder.append("...");
    return false;
}

bool P4Formatter::preorder(const IR::InvalidHeader *e) {
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append("(");
    builder.append("(");
    visit(e->headerType);
    builder.append(")");
    builder.append("{#}");
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::InvalidHeaderUnion *e) {
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append("(");
    builder.append("(");
    visit(e->headerUnionType);
    builder.append(")");
    builder.append("{#}");
    if (expressionPrecedence > DBPrint::Prec_Prefix) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::MethodCallExpression *e) {
    int prec = expressionPrecedence;
    bool useParens = (prec > DBPrint::Prec_Postfix) ||
                     (!e->typeArguments->empty() && prec >= DBPrint::Prec_Cond);
    // FIXME: we use parenthesis more often than necessary
    // because the bison parser has a bug which parses
    // these expressions incorrectly.
    expressionPrecedence = DBPrint::Prec_Postfix;
    if (useParens) builder.append("(");
    visit(e->method);
    if (!e->typeArguments->empty()) {
        bool decl = isDeclaration;
        isDeclaration = false;
        builder.append("<");
        setVecSep(", ");
        visit(e->typeArguments);
        doneVec();
        builder.append(">");
        isDeclaration = decl;
    }
    builder.append("(");
    setVecSep(", ");
    expressionPrecedence = DBPrint::Prec_Low;
    withinArgument = true;
    visit(e->arguments);
    withinArgument = false;
    doneVec();
    builder.append(")");
    if (useParens) builder.append(")");
    expressionPrecedence = prec;
    return false;
}

bool P4Formatter::preorder(const IR::Operation_Binary *b) {
    int prec = expressionPrecedence;
    bool useParens = prec > b->getPrecedence();
    if (useParens) builder.append("(");
    expressionPrecedence = b->getPrecedence();
    visit(b->left);
    builder.spc();
    builder.append(b->getStringOp());
    builder.spc();
    expressionPrecedence = b->getPrecedence() + 1;
    visit(b->right);
    if (useParens) builder.append(")");
    expressionPrecedence = prec;
    return false;
}

bool P4Formatter::preorder(const IR::Mux *b) {
    int prec = expressionPrecedence;
    bool useParens = prec >= b->getPrecedence();
    if (useParens) builder.append("(");
    expressionPrecedence = b->getPrecedence();
    visit(b->e0);
    builder.append(" ? ");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(b->e1);
    builder.append(" : ");
    expressionPrecedence = b->getPrecedence();
    visit(b->e2);
    expressionPrecedence = prec;
    if (useParens) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::Operation_Unary *u) {
    int prec = expressionPrecedence;
    bool useParens = prec > u->getPrecedence();
    if (useParens) builder.append("(");
    builder.append(u->getStringOp());
    expressionPrecedence = u->getPrecedence();
    visit(u->expr);
    expressionPrecedence = prec;
    if (useParens) builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::ArrayIndex *a) {
    int prec = expressionPrecedence;
    bool useParens = prec > a->getPrecedence();
    if (useParens) builder.append("(");
    expressionPrecedence = a->getPrecedence();
    visit(a->left);
    builder.append("[");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(a->right);
    builder.append("]");
    if (useParens) builder.append(")");
    expressionPrecedence = prec;
    return false;
}

bool P4Formatter::preorder(const IR::Cast *c) {
    int prec = expressionPrecedence;
    bool useParens = prec > c->getPrecedence();
    if (useParens) builder.append("(");
    builder.append("(");
    visit(c->destType);
    builder.append(")");
    expressionPrecedence = c->getPrecedence();
    visit(c->expr);
    if (useParens) builder.append(")");
    expressionPrecedence = prec;
    return false;
}

//////////////////////////////////////////////////////////

bool P4Formatter::preorder(const IR::AssignmentStatement *a) {
    visit(a->left);
    builder.append(" = ");
    visit(a->right);
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::BlockStatement *s) {
    if (!s->annotations->annotations.empty()) {
        visit(s->annotations);
        builder.spc();
    }
    builder.blockStart();
    setVecSep("\n", "\n");
    preorder(&s->components);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool P4Formatter::preorder(const IR::BreakStatement *) {
    builder.append("break");
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::ContinueStatement *) {
    builder.append("continue");
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::ExitStatement *) {
    builder.append("exit");
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::ReturnStatement *statement) {
    builder.append("return");
    if (statement->expression != nullptr) {
        builder.spc();
        visit(statement->expression);
    }
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::EmptyStatement *) {
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::IfStatement *s) {
    builder.append("if (");
    visit(s->condition);
    builder.append(") ");
    if (!s->ifTrue->is<IR::BlockStatement>()) {
        builder.append("{");
        builder.increaseIndent();
        builder.newline();
        builder.emitIndent();
    }
    visit(s->ifTrue);
    if (!s->ifTrue->is<IR::BlockStatement>()) {
        builder.newline();
        builder.decreaseIndent();
        builder.emitIndent();
        builder.append("}");
    }
    if (s->ifFalse != nullptr) {
        builder.append(" else ");
        if (!s->ifFalse->is<IR::BlockStatement>() && !s->ifFalse->is<IR::IfStatement>()) {
            builder.append("{");
            builder.increaseIndent();
            builder.newline();
            builder.emitIndent();
        }
        visit(s->ifFalse);
        if (!s->ifFalse->is<IR::BlockStatement>() && !s->ifFalse->is<IR::IfStatement>()) {
            builder.newline();
            builder.decreaseIndent();
            builder.emitIndent();
            builder.append("}");
        }
    }
    return false;
}

bool P4Formatter::preorder(const IR::ForStatement *s) {
    if (!s->annotations->annotations.empty()) {
        visit(s->annotations);
        builder.spc();
    }
    builder.append("for (");
    visitCollection(s->init, ", ", [&](auto *d) {
        builder.supressStatementSemi();
        visit(d, "init");
    });
    builder.append("; ");
    visit(s->condition, "condition");
    builder.append("; ");
    visitCollection(s->updates, ", ", [&](auto *e) {
        if (!e->template is<IR::EmptyStatement>()) {
            builder.supressStatementSemi();
            visit(e, "updates");
        }
    });
    builder.append(") ");
    if (!s->body->is<IR::BlockStatement>()) {
        builder.append("{");
        builder.increaseIndent();
        builder.newline();
        builder.emitIndent();
    }
    visit(s->body, "body");
    if (!s->body->is<IR::BlockStatement>()) {
        builder.newline();
        builder.decreaseIndent();
        builder.emitIndent();
        builder.append("}");
    }
    return false;
}

bool P4Formatter::preorder(const IR::ForInStatement *s) {
    if (!s->annotations->annotations.empty()) {
        visit(s->annotations);
        builder.spc();
    }
    builder.append("for (");
    if (s->decl) {
        builder.supressStatementSemi();
        visit(s->decl, "decl");
    } else {
        auto *decl = resolveUnique(s->ref->path->name, P4::ResolutionType::Any);
        if (auto *di = decl->to<IR::Declaration_Variable>()) {
            builder.supressStatementSemi();
            visit(di, "decl");
        } else {
            visit(s->ref, "ref");
        }
    }
    builder.append(" in ");
    visit(s->collection);
    builder.append(") ");
    if (!s->body->is<IR::BlockStatement>()) {
        builder.append("{");
        builder.increaseIndent();
        builder.newline();
        builder.emitIndent();
    }
    visit(s->body, "body");
    if (!s->body->is<IR::BlockStatement>()) {
        builder.newline();
        builder.decreaseIndent();
        builder.emitIndent();
        builder.append("}");
    }
    return false;
}

bool P4Formatter::preorder(const IR::MethodCallStatement *s) {
    visit(s->methodCall);
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::SwitchCase *s) {
    visit(s->label);
    builder.append(": ");
    visit(s->statement);
    return false;
}

bool P4Formatter::preorder(const IR::SwitchStatement *s) {
    builder.append("switch (");
    visit(s->expression);
    builder.append(") ");
    builder.blockStart();
    setVecSep("\n", "\n");
    preorder(&s->cases);
    doneVec();
    builder.blockEnd(false);
    return false;
}

////////////////////////////////////

bool P4Formatter::preorder(const IR::Annotations *a) {
    visitCollection(a->annotations, " ", [&](const auto *anno) { visit(anno); });
    return false;
}

bool P4Formatter::preorder(const IR::Annotation *a) {
    builder.append("@");
    builder.append(a->name);
    const char *open = a->structured ? "[" : "(";
    const char *close = a->structured ? "]" : ")";
    if (!a->expr.empty()) {
        builder.append(open);
        setVecSep(", ");
        preorder(&a->expr);
        doneVec();
        builder.append(close);
    }
    if (!a->kv.empty()) {
        builder.append(open);
        visitCollection(a->kv, ", ", [&](const auto &kvp) {
            builder.append(kvp->name);
            builder.append("=");
            visit(kvp->expression);
        });
        builder.append(close);
    }
    if (a->expr.empty() && a->kv.empty() && a->structured) {
        builder.append("[]");
    }
    if (!a->body.empty() && a->expr.empty() && a->kv.empty()) {
        // Have an unparsed annotation.
        // We could be prettier here with smarter logic, but let's do the easy
        // thing by separating every token with a space.
        builder.append(open);
        visitCollection(a->body, " ", [&](const auto &tok) {
            bool haveStringLiteral =
                tok->token_type == P4::P4Parser::token_type::TOK_STRING_LITERAL;
            if (haveStringLiteral) builder.append("\"");
            builder.append(tok->text);
            if (haveStringLiteral) builder.append("\"");
        });
        builder.append(close);
    }
    return false;
}

bool P4Formatter::preorder(const IR::Parameter *p) {
    if (!p->annotations->annotations.empty()) {
        visit(p->annotations);
        builder.spc();
    }
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
    bool decl = isDeclaration;
    isDeclaration = false;
    visit(p->type);
    isDeclaration = decl;
    builder.spc();
    builder.append(p->name);
    if (p->defaultValue != nullptr) {
        builder.append("=");
        visit(p->defaultValue);
    }
    return false;
}

bool P4Formatter::preorder(const IR::P4Control *c) {
    bool decl = isDeclaration;
    isDeclaration = false;
    visit(c->type);
    isDeclaration = decl;
    if (c->constructorParams->size() != 0) visit(c->constructorParams);
    builder.spc();
    builder.blockStart();
    for (auto s : c->controlLocals) {
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

bool P4Formatter::preorder(const IR::ParameterList *p) {
    builder.append("(");
    visitCollection(*p->getEnumerator(), ", ", [this](const auto &param) { visit(param); });
    builder.append(")");
    return false;
}

bool P4Formatter::preorder(const IR::P4Action *c) {
    if (!c->annotations->annotations.empty()) {
        visit(c->annotations);
        builder.spc();
    }
    builder.append("action ");
    builder.append(c->name);
    visit(c->parameters);
    builder.spc();
    visit(c->body);
    return false;
}

bool P4Formatter::preorder(const IR::ParserState *s) {
    if (s->isBuiltin()) return false;

    if (!s->annotations->annotations.empty()) {
        visit(s->annotations);
        builder.spc();
    }
    builder.append("state ");
    builder.append(s->name);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    preorder(&s->components);
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

bool P4Formatter::preorder(const IR::P4Parser *c) {
    bool decl = isDeclaration;
    isDeclaration = false;
    visit(c->type);
    isDeclaration = decl;
    if (c->constructorParams->size() != 0) visit(c->constructorParams);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    preorder(&c->parserLocals);
    doneVec();
    // explicit visit of parser states
    for (auto s : c->states) {
        if (s->isBuiltin()) continue;
        builder.emitIndent();
        visit(s);
        builder.append("\n");
    }
    builder.blockEnd(true);
    return false;
}

bool P4Formatter::preorder(const IR::ExpressionValue *v) {
    visit(v->expression);
    builder.endOfStatement();
    return false;
}

bool P4Formatter::preorder(const IR::ActionListElement *ale) {
    if (!ale->annotations->annotations.empty()) {
        visit(ale->annotations);
        builder.spc();
    }
    visit(ale->expression);
    return false;
}

bool P4Formatter::preorder(const IR::ActionList *v) {
    builder.blockStart();
    setVecSep(";\n", ";\n");
    preorder(&v->actionList);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool P4Formatter::preorder(const IR::Key *v) {
    builder.blockStart();

    std::unordered_map<const IR::KeyElement *, cstring> kf;
    size_t len = 0;
    for (auto f : v->keyElements) {
        Util::SourceCodeBuilder builder;
        P4Formatter rec(builder);

        f->expression->apply(rec);
        cstring s = builder.toString();
        if (s.size() > len) len = s.size();
        kf.emplace(f, s);
    }

    for (auto f : v->keyElements) {
        builder.emitIndent();
        cstring s = get(kf, f);
        builder.append(s);
        size_t spaces = len - s.size();
        builder.append(std::string(spaces, ' '));
        builder.append(": ");
        visit(f->matchType);
        if (!f->annotations->annotations.empty()) {
            builder.append(" ");
            visit(f->annotations);
        }
        builder.endOfStatement(true);
    }
    builder.blockEnd(false);
    return false;
}

bool P4Formatter::preorder(const IR::Property *p) {
    if (!p->annotations->annotations.empty()) {
        visit(p->annotations);
        builder.spc();
    }
    if (p->isConstant) builder.append("const ");
    builder.append(p->name);
    builder.append(" = ");
    visit(p->value);
    return false;
}

bool P4Formatter::preorder(const IR::TableProperties *t) {
    for (auto p : t->properties) {
        builder.emitIndent();
        visit(p);
        builder.newline();
    }
    return false;
}

bool P4Formatter::preorder(const IR::EntriesList *l) {
    builder.append("{");
    builder.newline();
    builder.increaseIndent();
    visit(&l->entries);
    builder.decreaseIndent();
    builder.emitIndent();
    builder.append("}");
    return false;
}

bool P4Formatter::preorder(const IR::Entry *e) {
    builder.emitIndent();
    if (e->isConst) builder.append("const ");
    if (e->priority) {
        builder.append("priority=");
        visit(e->priority);
        builder.append(": ");
    }
    if (e->keys->components.size() == 1)
        setListTerm("", "");
    else
        setListTerm("(", ")");
    visit(e->keys);
    doneList();
    builder.append(" : ");
    visit(e->action);
    if (!e->annotations->annotations.empty()) {
        visit(e->annotations);
    }
    builder.append(";");
    return false;
}

bool P4Formatter::preorder(const IR::P4Table *c) {
    if (!c->annotations->annotations.empty()) {
        visit(c->annotations);
        builder.spc();
    }
    builder.append("table ");
    builder.append(c->name);
    builder.spc();
    builder.blockStart();
    setVecSep("\n", "\n");
    visit(c->properties);
    doneVec();
    builder.blockEnd(false);
    return false;
}

bool P4Formatter::preorder(const IR::Path *p) {
    builder.append(p->asString());
    return false;
}

std::string toP4(const IR::INode *node) {
    std::stringstream stream;
    P4Fmt::P4Formatter toP4(&stream);
    node->getNode()->apply(toP4);
    return stream.str();
}

}  // namespace P4::P4Fmt
