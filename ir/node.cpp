#include "ir.h"

void IR::Node::traceVisit(const char* visitor) const
{ LOG3("Visiting " << visitor << " " << id << ":" << node_type_name()); }

void IR::Node::traceCreation() const { LOG5("Created node " << id); }

int IR::Node::currentId = 0;

cstring IR::dbp(const IR::INode* node) {
    std::stringstream str;
    if (node == nullptr) {
        str << "<nullptr>";
    } else {
        if (node->is<IR::IDeclaration>()) {
            node->getNode()->Node::dbprint(str);
            str << " " << node->to<IR::IDeclaration>()->getName();
        } else if (node->is<IR::Type_MethodBase>()) {
            str << node;
        } else if (node->is<IR::PathExpression>() ||
                 node->is<IR::Path>() ||
                 node->is<IR::TypeNameExpression>() ||
                 node->is<IR::Constant>() ||
                   node->is<IR::Type_Base>()) {
            node->getNode()->Node::dbprint(str);
            str << " " << node->toString();
        } else {
            node->getNode()->Node::dbprint(str);
        }
    }
    return str.str();
}

IRNODE_DEFINE_APPLY_OVERLOAD(Node, , )
