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

#include "ir.h"

void IR::Node::traceVisit(const char* visitor) const
{ LOG3("Visiting " << visitor << " " << id << ":" << node_type_name()); }

void IR::Node::traceCreation() const { LOG5("Created node " << id); }

int IR::Node::currentId = 0;

cstring IR::Node::toJSON(JSONGenerator &json) const {
    std::stringstream buf;
    buf << json.indent << "\"Node_ID\" : " << id << ", " << std::endl
        << json.indent << "\"Node_Type\" : \"" << node_type_name() << "\"";
    return buf.str();
}

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
        } else if (node->is<IR::Member>()) {
            node->getNode()->Node::dbprint(str);
            str << " ." << node->to<IR::Member>()->member;
        } else if (node->is<IR::PathExpression>() ||
                   node->is<IR::Path>() ||
                   node->is<IR::TypeNameExpression>() ||
                   node->is<IR::Constant>() ||
                   node->is<IR::Type_Name>() ||
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
