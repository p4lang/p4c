#ifndef _P4_TOP4_TOP4_H_
#define _P4_TOP4_TOP4_H_

#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/sourceCodeBuilder.h"

namespace P4 {
    // conversion from P4 v1.2 IR back to P4 source

class ToP4 : public Inspector {
    int expressionPrecedence;  // precedence of current IR::Operation
    bool isDeclaration;  // current type is a declaration
    bool showIR;  // if true dump IR as comments

    struct VecPrint {
        cstring separator;
        cstring terminator;

        VecPrint(const char* sep, const char* term) :
                separator(sep), terminator(term) {}
    };

    struct ListPrint {
        cstring start;
        cstring end;

        ListPrint(const char* start, const char* end) :
                start(start), end(end) {}
    };

    // maintained as stacks
    std::vector<VecPrint> vectorSeparator;
    std::vector<ListPrint> listTerminators;

    void setVecSep(const char* sep, const char* term = nullptr) {
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

    void setListTerm(const char* start, const char* end) {
        listTerminators.push_back(ListPrint(start, end));
    }
    void doneList() {
        BUG_CHECK(!listTerminators.empty(), "Empty listTerminators");
        listTerminators.pop_back();
    }
    bool isSystemFile(cstring file);
    // dump node IR tree up to depth - in the form of a comment
    void dump(unsigned depth, const IR::Node* node = nullptr, unsigned adjDepth = 0);
    unsigned curDepth() const;

 public:
    // Output is constructed here
    Util::SourceCodeBuilder& builder;
    std::ostream* outStream;
    // If this is set to non-nullptr, some declarations
    // that come from libraries and models are not
    // emitted.
    cstring mainFile;

    ToP4(Util::SourceCodeBuilder& builder, bool showIR, cstring mainFile = nullptr) :
            expressionPrecedence(DBPrint::Prec_Low),
            isDeclaration(true),
            showIR(showIR),
            builder(builder),
            outStream(nullptr),
            mainFile(mainFile)
    { visitDagOnce = false; }
    ToP4(std::ostream* outStream, bool showIR, cstring mainFile = nullptr) :
            expressionPrecedence(DBPrint::Prec_Low),
            isDeclaration(true),
            showIR(showIR),
            builder(* new Util::SourceCodeBuilder()),
            outStream(outStream),
            mainFile(mainFile)
    { visitDagOnce = false; }

    using Inspector::preorder;

    Visitor::profile_t init_apply(const IR::Node* node) override;
    void end_apply(const IR::Node* node) override;

    bool process(const IR::Type_StructLike* t, const char* name);
    // types
    bool preorder(const IR::Type_Boolean* t) override;
    bool preorder(const IR::Type_Varbits* t) override;
    bool preorder(const IR::Type_Bits* t) override;
    bool preorder(const IR::Type_InfInt* t) override;
    bool preorder(const IR::Type_Var* t) override;
    bool preorder(const IR::Type_Dontcare* t) override;
    bool preorder(const IR::Type_Void* t) override;
    bool preorder(const IR::Type_Error* t) override;
    bool preorder(const IR::Type_Struct* t) override
    { return process(t, "struct"); }
    bool preorder(const IR::Type_Header* t) override
    { return process(t, "header"); }
    bool preorder(const IR::Type_Union* t) override
    { return process(t, "header_union"); }
    bool preorder(const IR::Type_Package* t) override;
    bool preorder(const IR::Type_Parser* t) override;
    bool preorder(const IR::Type_Control* t) override;
    bool preorder(const IR::Type_Name* t) override;
    bool preorder(const IR::Type_Stack* t) override;
    bool preorder(const IR::Type_Specialized* t) override;
    bool preorder(const IR::Type_Enum* t) override;
    bool preorder(const IR::Type_Typedef* t) override;
    bool preorder(const IR::Type_Extern* t) override;
    bool preorder(const IR::Type_Unknown* t) override;

    // declarations
    bool preorder(const IR::Declaration_Constant* cst) override;
    bool preorder(const IR::Declaration_Variable* v) override;
    bool preorder(const IR::Declaration_Instance* t) override;
    bool preorder(const IR::Declaration_Errors* d) override;
    bool preorder(const IR::Declaration_MatchKind* d) override;

    // expressions
    bool preorder(const IR::Constant* c) override;
    bool preorder(const IR::Slice* slice) override;
    bool preorder(const IR::BoolLiteral* b) override;
    bool preorder(const IR::StringLiteral* s) override;
    bool preorder(const IR::PathExpression* p) override;
    bool preorder(const IR::Cast* c) override;
    bool preorder(const IR::Operation_Binary* b) override;
    bool preorder(const IR::Operation_Unary* u) override;
    bool preorder(const IR::ArrayIndex* a) override;
    bool preorder(const IR::TypeNameExpression* e) override;
    bool preorder(const IR::Mux* a) override;
    bool preorder(const IR::ConstructorCallExpression* e) override;
    bool preorder(const IR::Member* e) override;
    bool preorder(const IR::SelectCase* e) override;
    bool preorder(const IR::SelectExpression* e) override;
    bool preorder(const IR::ListExpression* e) override;
    bool preorder(const IR::MethodCallExpression* e) override;
    bool preorder(const IR::DefaultExpression* e) override;
    bool preorder(const IR::This* e) override;

    // vectors
    bool preorder(const IR::Vector<IR::Annotation>* v) override;
    bool preorder(const IR::Vector<IR::Type>* v) override;
    bool preorder(const IR::Vector<IR::Expression>* v) override;
    bool preorder(const IR::Vector<IR::SelectCase>* v) override;
    bool preorder(const IR::Vector<IR::SwitchCase>* v) override;
    bool preorder(const IR::Vector<IR::Node>* v) override;
    bool preorder(const IR::Vector<IR::ActionListElement>* v) override;
    bool preorder(const IR::Vector<IR::Method>* v) override;
    bool preorder(const IR::IndexedVector<IR::Node>* v) override;
    bool preorder(const IR::IndexedVector<IR::StatOrDecl>* v) override;
    bool preorder(const IR::IndexedVector<IR::ParserState>* v) override;
    bool preorder(const IR::IndexedVector<IR::Declaration>* v) override;

    // statements
    bool preorder(const IR::AssignmentStatement* s) override;
    bool preorder(const IR::BlockStatement* s) override;
    bool preorder(const IR::MethodCallStatement* s) override;
    bool preorder(const IR::EmptyStatement* s) override;
    bool preorder(const IR::ReturnStatement* s) override;
    bool preorder(const IR::ExitStatement* s) override;
    bool preorder(const IR::SwitchCase* s) override;
    bool preorder(const IR::SwitchStatement* s) override;
    bool preorder(const IR::IfStatement* s) override;

    // misc
    bool preorder(const IR::Path* p) override;
    bool preorder(const IR::Parameter* p) override;
    bool preorder(const IR::Annotation* a) override;
    bool preorder(const IR::P4Program* program) override;
    bool preorder(const IR::P4Control* c) override;
    bool preorder(const IR::P4Action* c) override;
    bool preorder(const IR::ParserState* s) override;
    bool preorder(const IR::P4Parser* c) override;
    bool preorder(const IR::TypeParameters* p) override;
    bool preorder(const IR::ParameterList* p) override;
    bool preorder(const IR::Method* p) override;
    bool preorder(const IR::Function* function) override;

    bool preorder(const IR::ExpressionValue* v) override;
    bool preorder(const IR::ActionListElement* ale) override;
    bool preorder(const IR::ActionList* v) override;
    bool preorder(const IR::Key* v) override;
    bool preorder(const IR::TableProperty* p) override;
    bool preorder(const IR::TableProperties* t) override;
    bool preorder(const IR::P4Table* c) override;
};

}  // namespace P4

#endif /* _P4_TOP4_TOP4_H_ */
