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

// If you want to use 'raise' below to trace a node creation uncomment the next line
// #include <signal.h>

#include "node.h"

#include <ostream>
// use in combination with "raise" below
// #include <csignal>

#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"
#include "ir/visitor.h"
#include "lib/indent.h"
#include "lib/json.h"
#include "lib/log.h"

void IR::Node::traceVisit(const char *visitor) const {
    LOG3("Visiting " << visitor << " " << id << ":" << node_type_name());
}

void IR::Node::traceCreation() const {
    /*
      You can use this to trigger a breakpoint in the debugger when a
      specific node is created
    if (id == 279493)
        raise(SIGINT);
    */
    LOG5("Created node " << id);
}

int IR::Node::currentId = 0;

void IR::Node::toJSON(JSONGenerator &json) const {
    json << json.indent << "\"Node_ID\" : " << id << "," << std::endl
         << json.indent << "\"Node_Type\" : " << node_type_name();
}

IR::Node::Node(JSONLoader &json) : id(-1) {
    json.load("Node_ID", id);
    if (id < 0)
        id = currentId++;
    else if (id >= currentId)
        currentId = id + 1;
    clone_id = id;
}

// Abbreviated debug print
cstring IR::dbp(const IR::INode *node) {
    std::stringstream str;
    if (node == nullptr) {
        str << "<nullptr>";
    } else {
        if (auto idecl = node->to<IR::IDeclaration>()) {
            node->getNode()->Node::dbprint(str);
            str << " " << idecl->getName();
            if (auto decl = idecl->to<IR::Declaration>())
                str << "/" << decl->declid;
            else if (auto decl = idecl->to<IR::Type_Declaration>())
                str << "/" << decl->declid;
        } else if (node->is<IR::Member>()) {
            node->getNode()->Node::dbprint(str);
            str << " ." << node->to<IR::Member>()->member;
        } else if (node->is<IR::Type_Type>()) {
            node->getNode()->Node::dbprint(str);
            str << "(" << dbp(node->to<IR::Type_Type>()->type) << ")";
        } else if (node->is<IR::PathExpression>() || node->is<IR::Path>() ||
                   node->is<IR::TypeNameExpression>() || node->is<IR::Constant>() ||
                   node->is<IR::Type_Name>() || node->is<IR::Type_Base>() ||
                   node->is<IR::Type_Specialized>()) {
            node->getNode()->Node::dbprint(str);
            str << " " << node->toString();
        } else {
            node->getNode()->Node::dbprint(str);
        }
    }
    return str.str();
}

cstring IR::Node::prepareSourceInfoForJSON(Util::SourceInfo &si, unsigned *lineNumber,
                                           unsigned *columnNumber) const {
    if (!si.isValid()) {
        return nullptr;
    }
    if (is<IR::AssignmentStatement>()) {
        auto assign = to<IR::AssignmentStatement>();
        si = (assign->left->srcInfo + si) + assign->right->srcInfo;
    }
    return si.toSourcePositionData(lineNumber, columnNumber);
}

// TODO: Find a way to eliminate the duplication below.

Util::JsonObject *IR::Node::sourceInfoJsonObj() const {
    Util::SourceInfo si = srcInfo;
    unsigned lineNumber, columnNumber;
    cstring fName = prepareSourceInfoForJSON(si, &lineNumber, &columnNumber);
    if (fName == nullptr) {
        if (si.line == -1) {
            // -1 is default value for objects when SourceInfo
            // was not read from jsonFile using "--fromJSON" flag
            return nullptr;
        } else {
            // Added source_info for jsonObject when "--fromJSON" flag is used
            // which parameters are saved in srcInfo fileds(filename, line, column and srcBrief)
            auto json1 = new Util::JsonObject();
            json1->emplace("filename", srcInfo.filename);
            json1->emplace("line", srcInfo.line);
            json1->emplace("column", srcInfo.column);
            json1->emplace("source_fragment", srcInfo.srcBrief);
            return json1;
        }
    } else {
        auto json = new Util::JsonObject();
        json->emplace("filename", fName);
        json->emplace("line", lineNumber);
        json->emplace("column", columnNumber);
        json->emplace("source_fragment", si.toBriefSourceFragment().escapeJson());
        return json;
    }
}

void IR::Node::sourceInfoToJSON(JSONGenerator &json) const {
    Util::SourceInfo si = srcInfo;
    unsigned lineNumber, columnNumber;
    cstring fName = prepareSourceInfoForJSON(si, &lineNumber, &columnNumber);
    if (fName == nullptr) {
        // Same reasoning as above.
        return;
    }

    json << "," << std::endl << json.indent++ << "\"Source_Info\" : {" << std::endl;

    json << json.indent << "\"filename\" : " << fName << "," << std::endl;
    json << json.indent << "\"line\" : " << lineNumber << "," << std::endl;
    json << json.indent << "\"column\" : " << columnNumber << "," << std::endl;
    json << json.indent << "\"source_fragment\" : " << si.toBriefSourceFragment().escapeJson()
         << std::endl;

    json << --json.indent << "}";
}

IRNODE_DEFINE_APPLY_OVERLOAD(Node, , )
