#ifndef BACKENDS_P4FMT_ATTACH_H_
#define BACKENDS_P4FMT_ATTACH_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

class Attach : public Inspector {
 public:
    Attach();
    ~Attach();

    template <typename T>
    bool preorderImpl(const T *node);

    void attachCommentsToNode(const IR::Node *);

    using Inspector::preorder;

    bool preorder(const IR::Type_Varbits *t) override;
    bool preorder(const IR::Type_Bits *t) override;
    bool preorder(const IR::Type_InfInt *t) override;
    bool preorder(const IR::Type_String *t) override;
    bool preorder(const IR::Type_Var *t) override;
    bool preorder(const IR::Type_Package *package) override;
    bool preorder(const IR::Type_Parser *t) override;
    bool preorder(const IR::Type_Control *t) override;
    bool preorder(const IR::Type_Name *t) override;
    bool preorder(const IR::Type_Stack *t) override;
    bool preorder(const IR::Type_Specialized *t) override;
    bool preorder(const IR::Type_Enum *t) override;
    bool preorder(const IR::Type_SerEnum *t) override;
    bool preorder(const IR::Type_Typedef *t) override;
    bool preorder(const IR::Type_Newtype *t) override;
    bool preorder(const IR::Type_Extern *t) override;
    bool preorder(const IR::Type_BaseList *t) override;

    // declarations
    bool preorder(const IR::Declaration_Constant *cst) override;
    bool preorder(const IR::Declaration_Variable *v) override;
    bool preorder(const IR::Declaration_Instance *i) override;
    bool preorder(const IR::Declaration_MatchKind *d) override;

    // expressions
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
    bool preorder(const IR::InvalidHeader *e) override;
    bool preorder(const IR::InvalidHeaderUnion *e) override;
    bool preorder(const IR::HeaderStackExpression *e) override;
    bool preorder(const IR::MethodCallExpression *e) override;

    // statements
    bool preorder(const IR::AssignmentStatement *s) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::MethodCallStatement *s) override;
    bool preorder(const IR::ReturnStatement *s) override;
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
    bool preorder(const IR::Annotations *a) override;
    bool preorder(const IR::Annotation *a) override;
    bool preorder(const IR::P4Control *c) override;
    bool preorder(const IR::P4Action *c) override;
    bool preorder(const IR::ParserState *s) override;
    bool preorder(const IR::P4Parser *c) override;
    bool preorder(const IR::TypeParameters *p) override;
    bool preorder(const IR::ParameterList *p) override;
    bool preorder(const IR::Method *m) override;
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
};

}  // namespace P4

#endif /* BACKENDS_P4FMT_ATTACH_H_ */
