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

void IR::Node::toJSON(JSONGenerator &json) const {
    json << json.indent << "\"Node_ID\" : " << id << ", " << std::endl
         << json.indent << "\"Node_Type\" : " << node_type_name();
}

IR::Node::Node(JSONLoader &json) : id(-1) {
    json.load("Node_ID", id);
    if (id < 0)
        id = currentId++;
    else if (id >= currentId)
        currentId = id+1;
}

// Abbreviated debug print
cstring IR::dbp(const IR::INode* node) {
    std::stringstream str;
    if (node == nullptr) {
        str << "<nullptr>";
    } else {
        if (node->is<IR::IDeclaration>()) {
            node->getNode()->Node::dbprint(str);
            str << " " << node->to<IR::IDeclaration>()->getName();
        } else if (node->is<IR::Member>()) {
            node->getNode()->Node::dbprint(str);
            str << " ." << node->to<IR::Member>()->member;
        } else if (node->is<IR::Type_Type>()) {
            node->getNode()->Node::dbprint(str);
            str << "(" << dbp(node->to<IR::Type_Type>()->type) << ")";
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

Util::JsonObject* IR::Node::sourceInfoJsonObj() const {
    unsigned lineNumber, columnNumber;
    const IR::Expression *lhs, *rhs;
    cstring fName = srcInfo.toSourcePositionData(&lineNumber,
                                                 &columnNumber);
    if (fName == nullptr) {
        // Do not add anything to the bmv2 JSON file for this, as this
        // is likely a statement synthesized by the compiler, and
        // either not easy, or it is impossible, to correlate it
        // directly with anything in the user's P4 source code.
        return nullptr;
    }
    bool isAssignment = is<IR::AssignmentStatement>();
    if (isAssignment) {
        auto assign = to<IR::AssignmentStatement>();
        lhs = assign->left;
        rhs = assign->right;
        lhs->srcInfo.toSourcePositionData(&lineNumber, &columnNumber);
    }
    auto json = new Util::JsonObject();
    cstring sourceFrag = srcInfo.toBriefSourceFragment();
    json->emplace("filename", fName);
    json->emplace("line", lineNumber);
    json->emplace("column", columnNumber);
    cstring fullSourceFrag;
    if (isAssignment) {
        cstring lhsFrag = lhs->srcInfo.toBriefSourceFragment();
        cstring rhsFrag = rhs->srcInfo.toBriefSourceFragment();
        fullSourceFrag = lhsFrag + " " + sourceFrag + " " + rhsFrag;
    } else {
        fullSourceFrag = sourceFrag;
    }
    json->emplace("source_fragment", fullSourceFrag);
    return json;
}

IRNODE_DEFINE_APPLY_OVERLOAD(Node, , )
