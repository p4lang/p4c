#ifndef BACKENDS_P4FMT_P4FORMATTER_H_
#define BACKENDS_P4FMT_P4FORMATTER_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/sourceCodeBuilder.h"

namespace P4::P4Fmt {
/**
This pass converts a P4-16 IR into a P4 source (text) program.
It can optionally emit as comments a representation of the program IR.
*/
class P4Formatter : public Inspector, ::P4::ResolutionContext {
    int expressionPrecedence;  /// precedence of current IR::Operation
    bool isDeclaration;        /// current type is a declaration
    bool withinArgument;       /// if true we are within a method call argument
    bool noIncludes = false;   /// If true do not generate #include statements.
                               /// Used for debugging.

    struct VecPrint {
        cstring separator;
        cstring terminator;

        VecPrint(const char *sep, const char *term) : separator(sep), terminator(term) {}
    };

    struct ListPrint {
        cstring start;
        cstring end;

        ListPrint(const char *start, const char *end) : start(start), end(end) {}
    };

    // maintained as stacks
    std::vector<VecPrint> vectorSeparator;
    size_t vectorSeparator_init_apply_size = 0;
    std::vector<ListPrint> listTerminators;
    size_t listTerminators_init_apply_size = 0;

    void setVecSep(const char *sep, const char *term = nullptr) {
        vectorSeparator.push_back(VecPrint(sep, term));
    }
    void doneVec() {
        BUG_CHECK(!vectorSeparator.empty(), "Empty vectorSeparator");
        vectorSeparator.pop_back();
    }
    VecPrint getSep() {
        BUG_CHECK(!vectorSeparator.empty(), "Empty vectorSeparator");
        return vectorSeparator.back();
    }

    void doneList() {
        BUG_CHECK(!listTerminators.empty(), "Empty listTerminators");
        listTerminators.pop_back();
    }

    // RAII helper class to manage separators
    class WithSeparator {
        P4Formatter &formatter;

     public:
        WithSeparator(P4Formatter &fmt, const char *sep, const char *term = nullptr)
            : formatter(fmt) {
            formatter.setVecSep(sep, term);
        }

        ~WithSeparator() { formatter.doneVec(); }
    };

    // For collections which provide support for range based for loops
    // e.g. `IndexedVector` or similar collections
    template <typename Collection, typename Func>
    void visitCollection(const Collection &collection, const std::string &separator,
                         Func visitFunc) {
        bool first = true;
        for (const auto &elem : collection) {
            if (!first) {
                builder.append(separator);
            }
            first = false;
            visitFunc(elem);
        }
    }

    // For `Enumerator`
    template <typename Enumerator, typename Func>
    void visitCollection(Enumerator &enumerator, const std::string &separator, Func visitFunc) {
        bool first = true;
        while (enumerator.moveNext()) {
            if (!first) {
                builder.append(separator);
            }
            first = false;
            visitFunc(enumerator.getCurrent());
        }
    }

    bool isSystemFile(std::filesystem::path &file);
    std::filesystem::path ifSystemFile(
        const IR::Node *node);  // return file containing node if system file

 public:
    // Output is constructed here
    Util::SourceCodeBuilder &builder;
    // FIXME  -- simplify this by getting rid of the 'builder' object and just emitting
    // directly to the ostream.  The SourceCodeBuilder object does not appear to add any
    // useful functionality the ostream does not already provide; it just serves to
    // obfuscate the code
    std::ostream *outStream;
    /** If this is set to non-nullptr, some declarations
        that come from libraries and models are not
        emitted. */
    cstring mainFile;

    explicit P4Formatter(Util::SourceCodeBuilder &builder, cstring mainFile = nullptr)
        : expressionPrecedence(DBPrint::Prec_Low),
          isDeclaration(true),
          withinArgument(false),
          builder(builder),
          outStream(nullptr),
          mainFile(mainFile) {
        visitDagOnce = false;
        setName("P4Formatter");
    }
    explicit P4Formatter(std::ostream *outStream, cstring mainFile = nullptr)
        : expressionPrecedence(DBPrint::Prec_Low),
          isDeclaration(true),
          withinArgument(false),
          builder(*new Util::SourceCodeBuilder()),
          outStream(outStream),
          mainFile(mainFile) {
        visitDagOnce = false;
        setName("P4Formatter");
    }
    P4Formatter()
        :  // this is useful for debugging
          expressionPrecedence(DBPrint::Prec_Low),
          isDeclaration(true),
          withinArgument(false),
          builder(*new Util::SourceCodeBuilder()),
          outStream(&std::cout),
          mainFile(nullptr) {
        visitDagOnce = false;
        setName("P4Formatter");
    }

    void setnoIncludesArg(bool condition) { noIncludes = condition; }

    void setListTerm(const char *start, const char *end) {
        listTerminators.push_back(ListPrint(start, end));
    }
    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *node) override;

    bool process(const IR::Type_StructLike *t, const char *name);

    bool preorder(const IR::Node *node) override;
    bool printAnnotations(const IR::IAnnotated *ann);

    // types
    bool preorder(const IR::Type_Boolean *t) override;
    bool preorder(const IR::Type_Varbits *t) override;
    bool preorder(const IR::Type_Bits *t) override;
    bool preorder(const IR::Type_InfInt *t) override;
    bool preorder(const IR::Type_String *t) override;
    bool preorder(const IR::Type_Var *t) override;
    bool preorder(const IR::Type_Dontcare *t) override;
    bool preorder(const IR::Type_Void *t) override;
    bool preorder(const IR::Type_Error *t) override;
    bool preorder(const IR::Type_Struct *t) override { return process(t, "struct"); }
    bool preorder(const IR::Type_Header *t) override { return process(t, "header"); }
    bool preorder(const IR::Type_HeaderUnion *t) override { return process(t, "header_union"); }
    bool preorder(const IR::Type_Package *t) override;
    bool preorder(const IR::Type_Parser *t) override;
    bool preorder(const IR::Type_Control *t) override;
    bool preorder(const IR::Type_Name *t) override;
    bool preorder(const IR::Type_Array *t) override;
    bool preorder(const IR::Type_Specialized *t) override;
    bool preorder(const IR::Type_Enum *t) override;
    bool preorder(const IR::Type_SerEnum *t) override;
    bool preorder(const IR::Type_Typedef *t) override;
    bool preorder(const IR::Type_Newtype *t) override;
    bool preorder(const IR::Type_Extern *t) override;
    bool preorder(const IR::Type_Unknown *t) override;
    bool preorder(const IR::Type_BaseList *t) override;
    bool preorder(const IR::Type *t) override {
        builder.append(t->toString());
        return false;
    }
    bool preorder(const IR::Type_SpecializedCanonical *t) override {
        BUG("%1%: specialized canonical type in IR tree", t);
        return false;
    }

    // declarations
    bool preorder(const IR::Declaration_Constant *cst) override;
    bool preorder(const IR::Declaration_Variable *v) override;
    bool preorder(const IR::Declaration_Instance *t) override;
    bool preorder(const IR::Declaration_MatchKind *d) override;

    // expressions
    bool preorder(const IR::Dots *e) override;
    bool preorder(const IR::NamedDots *e) override;
    bool preorder(const IR::Constant *c) override;
    bool preorder(const IR::Slice *slice) override;
    bool preorder(const IR::BoolLiteral *b) override;
    bool preorder(const IR::StringLiteral *s) override;
    bool preorder(const IR::PathExpression *p) override;
    bool preorder(const IR::Cast *c) override;
    bool preorder(const IR::Operation_Binary *b) override;
    bool preorder(const IR::Operation_Unary *u) override;
    bool preorder(const IR::ArrayIndex *a) override;
    bool preorder(const IR::TypeNameExpression *e) override;
    bool preorder(const IR::Mux *a) override;
    bool preorder(const IR::ConstructorCallExpression *e) override;
    bool preorder(const IR::Member *e) override;
    bool preorder(const IR::SelectCase *e) override;
    bool preorder(const IR::SelectExpression *e) override;
    bool preorder(const IR::ListExpression *e) override;
    bool preorder(const IR::P4ListExpression *e) override;
    bool preorder(const IR::StructExpression *e) override;
    bool preorder(const IR::Invalid *e) override;
    bool preorder(const IR::InvalidHeader *e) override;
    bool preorder(const IR::InvalidHeaderUnion *e) override;
    bool preorder(const IR::HeaderStackExpression *e) override;
    bool preorder(const IR::MethodCallExpression *e) override;
    bool preorder(const IR::DefaultExpression *e) override;
    bool preorder(const IR::This *e) override;

    // vectors
    bool preorder(const IR::Vector<IR::ActionListElement> *v) override;
    bool preorder(const IR::Vector<IR::Annotation> *v) override;
    bool preorder(const IR::Vector<IR::Entry> *v) override;
    bool preorder(const IR::Vector<IR::Expression> *v) override;
    bool preorder(const IR::Vector<IR::Argument> *v) override;
    bool preorder(const IR::Vector<IR::KeyElement> *v) override;
    bool preorder(const IR::Vector<IR::Method> *v) override;
    bool preorder(const IR::Vector<IR::Node> *v) override;
    bool preorder(const IR::Vector<IR::SelectCase> *v) override;
    bool preorder(const IR::Vector<IR::SwitchCase> *v) override;
    bool preorder(const IR::Vector<IR::Type> *v) override;
    bool preorder(const IR::IndexedVector<IR::Declaration_ID> *v) override;
    bool preorder(const IR::IndexedVector<IR::Declaration> *v) override;
    bool preorder(const IR::IndexedVector<IR::Node> *v) override;
    bool preorder(const IR::IndexedVector<IR::ParserState> *v) override;
    bool preorder(const IR::IndexedVector<IR::StatOrDecl> *v) override;

    // statements
    bool preorder(const IR::AssignmentStatement *s) override;
    bool preorder(const IR::OpAssignmentStatement *s) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::MethodCallStatement *s) override;
    bool preorder(const IR::EmptyStatement *s) override;
    bool preorder(const IR::ReturnStatement *s) override;
    bool preorder(const IR::BreakStatement *s) override;
    bool preorder(const IR::ContinueStatement *s) override;
    bool preorder(const IR::ExitStatement *s) override;
    bool preorder(const IR::SwitchCase *s) override;
    bool preorder(const IR::SwitchStatement *s) override;
    bool preorder(const IR::IfStatement *s) override;
    bool preorder(const IR::ForStatement *s) override;
    bool preorder(const IR::ForInStatement *s) override;

    // misc
    bool preorder(const IR::NamedExpression *ne) override;
    bool preorder(const IR::Argument *arg) override;
    bool preorder(const IR::Path *p) override;
    bool preorder(const IR::Parameter *p) override;
    bool preorder(const IR::Annotation *a) override;
    bool preorder(const IR::P4Program *program) override;
    bool preorder(const IR::P4Control *c) override;
    bool preorder(const IR::P4Action *c) override;
    bool preorder(const IR::ParserState *s) override;
    bool preorder(const IR::P4Parser *c) override;
    bool preorder(const IR::TypeParameters *p) override;
    bool preorder(const IR::ParameterList *p) override;
    bool preorder(const IR::Method *p) override;
    bool preorder(const IR::Function *function) override;

    bool preorder(const IR::ExpressionValue *v) override;
    bool preorder(const IR::ActionListElement *ale) override;
    bool preorder(const IR::ActionList *v) override;
    bool preorder(const IR::Key *v) override;
    bool preorder(const IR::Property *p) override;
    bool preorder(const IR::TableProperties *t) override;
    bool preorder(const IR::EntriesList *l) override;
    bool preorder(const IR::Entry *e) override;
    bool preorder(const IR::P4Table *c) override;
    bool preorder(const IR::P4ValueSet *c) override;

    // in case it is accidentally called on a V1Program
    bool preorder(const IR::V1Program *) override { return false; }
};

std::string toP4(const IR::INode *node);

}  // namespace P4::P4Fmt
#endif /* BACKENDS_P4FMT_P4FORMATTER_H_ */
