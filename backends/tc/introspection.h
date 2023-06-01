/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_INTROSPECTION_H_
#define BACKENDS_TC_INTROSPECTION_H_

#include "frontends/p4/parseAnnotations.h"
#include "frontends/p4/parserCallGraph.h"
#include "ir/ir.h"
#include "lib/json.h"
#include "lib/nullstream.h"
#include "options.h"

namespace TC {

struct introspectionInfo {
    cstring schemaVersion;
    cstring pipelineName;
    unsigned int pipelineId;
    introspectionInfo() {
        schemaVersion = nullptr;
        pipelineName = nullptr;
        pipelineId = 0;
    }
    void initIntrospectionInfo(IR::TCPipeline *tcPipeline) {
        schemaVersion = "1.0.0";
        pipelineName = tcPipeline->pipelineName;
        pipelineId = tcPipeline->pipelineId;
    }
};

struct keyFieldAttributes {
    unsigned int id;
    cstring name;
    cstring type;
    cstring matchType;
    unsigned int bitwidth;
    keyFieldAttributes() {
        id = 0;
        name = nullptr;
        type = nullptr;
        matchType = nullptr;
        bitwidth = 0;
    }
};

struct annotation {
    cstring name;
    annotation() { name = nullptr; }
    explicit annotation(cstring n) { name = n; }
};

struct actionParam {
    unsigned int id;
    cstring name;
    unsigned int dataType;
    unsigned int bitwidth;
    actionParam() {
        id = 0;
        name = nullptr;
        bitwidth = 0;
    }
};

enum actionScope { TableOnly, DefaultOnly, TableAndDefault };

struct actionAttributes {
    unsigned int id;
    cstring name;
    actionScope scope;
    bool defaultHit;
    bool defaultMiss;
    safe_vector<struct annotation *> annotations;
    safe_vector<struct actionParam *> actionParams;
    actionAttributes() {
        id = 0;
        name = nullptr;
        scope = TableAndDefault;
        defaultHit = false;
        defaultMiss = false;
    }
};

struct tableAttributes {
    cstring name;
    unsigned int id;
    unsigned int tentries;
    unsigned int numMask;
    unsigned int keysize;
    unsigned int keyid;
    safe_vector<struct keyFieldAttributes *> keyFields;
    safe_vector<struct actionAttributes *> actions;
    tableAttributes() {
        name = nullptr;
        id = 0;
        tentries = 0;
        numMask = 0;
        keysize = 0;
        keyid = 0;
    }
};

// This pass generates introspection JSON into user specified file
class IntrospectionGenerator : public Inspector {
    IR::TCPipeline *tcPipeline;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    safe_vector<struct tableAttributes *> tablesInfo;
    ordered_map<cstring, const IR::P4Table *> p4tables;

 public:
    IntrospectionGenerator(IR::TCPipeline *tcPipeline, P4::ReferenceMap *refMap,
                           P4::TypeMap *typeMap)
        : tcPipeline(tcPipeline), refMap(refMap), typeMap(typeMap) {}
    void postorder(const IR::P4Table *t);
    const Util::JsonObject *genIntrospectionJson();
    void genTableJson(Util::JsonArray *tablesJson);
    Util::JsonObject *genTableInfo(struct tableAttributes *tbl);
    void collectTableInfo();
    Util::JsonObject *genActionInfo(struct actionAttributes *action);
    Util::JsonObject *genKeyInfo(struct keyFieldAttributes *keyField);
    bool serializeIntrospectionJson(std::ostream &destination);
    cstring checkValidTcType(const IR::StringLiteral *sl);
    cstring externalName(const IR::IDeclaration *declaration);
};

}  // namespace TC

#endif /* BACKENDS_TC_INTROSPECTION_H_ */
