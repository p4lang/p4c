#include "attach.h"

#include "ir/ir-generated.h"
#include "lib/source_file.h"

namespace P4 {

void Attach::attachCommentsToNode(const IR::Node *node) {
    const auto &comments = node->srcInfo.getAllComments();

    if ((node == nullptr) || !node->srcInfo.isValid() || comments.empty()) {
        return;
    }

    static std::unordered_set<const Util::Comment *> attachedComments;

    const auto &nodePosition = node->srcInfo.getStart();
    unsigned nodeLineNumber = nodePosition.getLineNumber();

    for (auto *comment : comments) {
        // Prevent attachment of comments to multiple nearest nodes
        if (attachedComments.find(comment) != attachedComments.end()) {
            // Comment is already attached to another node
            continue;
        }

        const auto &commentStart = comment->getStartPosition();
        const auto &commentEnd = comment->getEndPosition();

        try {
            std::cout << "Comment: " << *comment << '\n';
            std::cout << "Comment start line: " << commentStart.getLineNumber() << '\n';
            std::cout << "Comment end line: " << commentEnd.getLineNumber() << '\n';
            std::cout << "Node line: " << nodeLineNumber << '\n';
        } catch (const std::exception &e) {
            std::cerr << "Error printing comment: " << e.what() << '\n';
            continue;
        }

        if (commentEnd.getLineNumber() == nodeLineNumber - 1) {
            node->srcInfo.addBefore(comment);
            attachedComments.insert(comment);
        } else if (commentStart.getLineNumber() == nodeLineNumber) {
            node->srcInfo.addAfter(comment);
            attachedComments.insert(comment);
        }
    }
}

Attach::Attach(){};
Attach::~Attach(){};

template <typename T>
bool Attach::preorderImpl(const T *node) {
    attachCommentsToNode(node);
    return false;
}

//////////////////////// TYPES ////////////////////
bool Attach::preorder(const IR::Type_Bits *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_String *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_InfInt *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Var *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Name *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Stack *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Specialized *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Argument *arg) { return preorderImpl(arg); }
bool Attach::preorder(const IR::Type_Typedef *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Newtype *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_BaseList *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::P4ValueSet *c) { return preorderImpl(c); }
bool Attach::preorder(const IR::Type_Enum *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_SerEnum *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::TypeParameters *p) { return preorderImpl(p); }
bool Attach::preorder(const IR::Method *m) { return preorderImpl(m); }
bool Attach::preorder(const IR::Function *function) { return preorderImpl(function); }
bool Attach::preorder(const IR::Type_Extern *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Varbits *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Package *package) { return preorderImpl(package); }
bool Attach::preorder(const IR::Type_Parser *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::Type_Control *t) { return preorderImpl(t); }

/////////////////////// DECLARATIONS /////////////////////////////////
bool Attach::preorder(const IR::Declaration_Constant *cst) { return preorderImpl(cst); }
bool Attach::preorder(const IR::Declaration_Instance *i) { return preorderImpl(i); }
bool Attach::preorder(const IR::Declaration_Variable *v) { return preorderImpl(v); }
bool Attach::preorder(const IR::Declaration_MatchKind *d) { return preorderImpl(d); }

/////////////////////// EXPRESSIONS /////////////////////////////////
bool Attach::preorder(const IR::Constant *c) { return preorderImpl(c); }
bool Attach::preorder(const IR::BoolLiteral *b) { return preorderImpl(b); }
bool Attach::preorder(const IR::StringLiteral *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::Slice *slice) { return preorderImpl(slice); }
bool Attach::preorder(const IR::PathExpression *p) { return preorderImpl(p); }
bool Attach::preorder(const IR::TypeNameExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::ConstructorCallExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::Member *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::SelectCase *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::SelectExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::ListExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::P4ListExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::NamedExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::StructExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::HeaderStackExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::InvalidHeader *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::InvalidHeaderUnion *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::MethodCallExpression *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::Operation_Binary *b) { return preorderImpl(b); }
bool Attach::preorder(const IR::Mux *b) { return preorderImpl(b); }
bool Attach::preorder(const IR::Operation_Unary *u) { return preorderImpl(u); }
bool Attach::preorder(const IR::ArrayIndex *a) { return preorderImpl(a); }
bool Attach::preorder(const IR::Cast *c) { return preorderImpl(c); }

/////////////////////// STATEMENTS /////////////////////////////////

bool Attach::preorder(const IR::AssignmentStatement *a) { return preorderImpl(a); }
bool Attach::preorder(const IR::BlockStatement *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::ReturnStatement *statement) { return preorderImpl(statement); }
bool Attach::preorder(const IR::IfStatement *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::ForStatement *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::ForInStatement *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::MethodCallStatement *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::SwitchCase *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::SwitchStatement *s) { return preorderImpl(s); }

/////////////////////// MISCS /////////////////////////////////
bool Attach::preorder(const IR::Annotations *a) { return preorderImpl(a); }
bool Attach::preorder(const IR::Annotation *a) { return preorderImpl(a); }
bool Attach::preorder(const IR::Parameter *p) { return preorderImpl(p); }
bool Attach::preorder(const IR::P4Control *c) { return preorderImpl(c); }
bool Attach::preorder(const IR::ParameterList *p) { return preorderImpl(p); }
bool Attach::preorder(const IR::P4Action *c) { return preorderImpl(c); }
bool Attach::preorder(const IR::ParserState *s) { return preorderImpl(s); }
bool Attach::preorder(const IR::P4Parser *c) { return preorderImpl(c); }
bool Attach::preorder(const IR::ExpressionValue *v) { return preorderImpl(v); }
bool Attach::preorder(const IR::ActionListElement *ale) { return preorderImpl(ale); }
bool Attach::preorder(const IR::ActionList *v) { return preorderImpl(v); }
bool Attach::preorder(const IR::Key *v) { return preorderImpl(v); }
bool Attach::preorder(const IR::Property *p) { return preorderImpl(p); }
bool Attach::preorder(const IR::TableProperties *t) { return preorderImpl(t); }
bool Attach::preorder(const IR::EntriesList *l) { return preorderImpl(l); }
bool Attach::preorder(const IR::Entry *e) { return preorderImpl(e); }
bool Attach::preorder(const IR::P4Table *c) { return preorderImpl(c); }
bool Attach::preorder(const IR::Path *p) { return preorderImpl(p); }

}  // namespace P4
