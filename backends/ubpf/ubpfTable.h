/*
Copyright 2019 Orange

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

#ifndef BACKENDS_UBPF_UBPFTABLE_H_
#define BACKENDS_UBPF_UBPFTABLE_H_

#include <stddef.h>

#include <map>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/target.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "ubpfProgram.h"

namespace UBPF {

class UBPFTableBase : public EBPF::EBPFObject {
 public:
    const UBPFProgram *program;

    cstring instanceName;
    cstring keyTypeName;
    cstring valueTypeName;
    const IR::Type *keyType{};
    const IR::Type *valueType{};
    cstring dataMapName;
    size_t size{};
    EBPF::CodeGenInspector *codeGen;

    void emitInstance(EBPF::CodeBuilder *pBuilder, EBPF::TableKind tableKind);

 protected:
    UBPFTableBase(const UBPFProgram *program, cstring instanceName, EBPF::CodeGenInspector *codeGen)
        : program(program), instanceName(instanceName), codeGen(codeGen) {
        CHECK_NULL(codeGen);
        CHECK_NULL(program);
        keyTypeName = program->refMap->newName(instanceName + "_key");
        valueTypeName = program->refMap->newName(instanceName + "_value");
        dataMapName = instanceName;
    }
};

class UBPFTable final : public UBPFTableBase {
 private:
    void setTableSize(const IR::TableBlock *table);
    void setTableKind();

 public:
    const IR::Key *keyGenerator;
    const IR::ActionList *actionList;
    const IR::TableBlock *table;
    EBPF::TableKind tableKind;
    cstring defaultActionMapName;
    cstring actionEnumName;
    cstring noActionName;
    std::map<const IR::KeyElement *, cstring> keyFieldNames;
    std::map<const IR::KeyElement *, EBPF::EBPFType *> keyTypes;

    UBPFTable(const UBPFProgram *program, const IR::TableBlock *table,
              EBPF::CodeGenInspector *codeGen);

    cstring generateActionName(const IR::P4Action *action);
    void emitInstance(EBPF::CodeBuilder *pBuilder);
    void emitTypes(EBPF::CodeBuilder *builder);
    void emitActionArguments(EBPF::CodeBuilder *builder, const IR::P4Action *action, cstring name);
    void emitKeyType(EBPF::CodeBuilder *builder);
    void emitValueType(EBPF::CodeBuilder *builder);
    void emitKey(EBPF::CodeBuilder *builder, cstring keyName);
    void emitAction(EBPF::CodeBuilder *builder, cstring valueName);
    void emitInitializer(EBPF::CodeBuilder *builder);
};

}  // namespace UBPF

#endif  // BACKENDS_UBPF_UBPFTABLE_H_
