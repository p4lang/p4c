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

#ifndef _BACKENDS_EBPF_EBPFTABLE_H_
#define _BACKENDS_EBPF_EBPFTABLE_H_

#include "ebpfObject.h"
#include "ebpfProgram.h"
#include "frontends/p4/methodInstance.h"

namespace EBPF {

class ActionTranslationVisitor : public virtual CodeGenInspector {
 protected:
    const EBPFProgram *program;
    const IR::P4Action *action;
    cstring valueName;

 public:
    ActionTranslationVisitor(cstring valueName, const EBPFProgram *program)
        : CodeGenInspector(program->refMap, program->typeMap),
          program(program),
          action(nullptr),
          valueName(valueName) {
        CHECK_NULL(program);
    }

    bool preorder(const IR::PathExpression *expression);

    bool preorder(const IR::P4Action *act);
    virtual cstring getParamInstanceName(const IR::Expression *expression) const;
    bool isActionParameter(const IR::PathExpression *expression) const;
};  // ActionTranslationVisitor

// Also used to represent counters
class EBPFTableBase : public EBPFObject {
 public:
    const EBPFProgram *program;

    cstring instanceName;
    cstring keyTypeName;
    cstring valueTypeName;
    cstring dataMapName;
    CodeGenInspector *codeGen;

 protected:
    EBPFTableBase(const EBPFProgram *program, cstring instanceName, CodeGenInspector *codeGen)
        : program(program), instanceName(instanceName), codeGen(codeGen) {
        CHECK_NULL(codeGen);
        CHECK_NULL(program);
        keyTypeName = instanceName + "_key";
        valueTypeName = instanceName + "_value";
        dataMapName = instanceName;
    }
};

class EBPFTable : public EBPFTableBase {
    const int prefixLenFieldWidth = 32;

    void initKey();

 public:
    bool isLPMTable() const;
    bool isTernaryTable() const;

 protected:
    void emitTernaryInstance(CodeBuilder *builder);

    virtual void validateKeys() const;
    virtual ActionTranslationVisitor *createActionTranslationVisitor(
        cstring valueName, const EBPFProgram *program) const {
        return new ActionTranslationVisitor(valueName, program);
    }

 public:
    const IR::Key *keyGenerator;
    const IR::ActionList *actionList;
    const IR::TableBlock *table;
    cstring defaultActionMapName;
    std::map<const IR::KeyElement *, cstring> keyFieldNames;
    std::map<const IR::KeyElement *, EBPFType *> keyTypes;
    // Use 1024 by default.
    // TODO: make it configurable using compiler options.
    size_t size = 1024;
    const cstring prefixFieldName = "prefixlen";

    EBPFTable(const EBPFProgram *program, const IR::TableBlock *table, CodeGenInspector *codeGen);
    EBPFTable(const EBPFProgram *program, CodeGenInspector *codeGen, cstring name);

    cstring p4ActionToActionIDName(const IR::P4Action *action) const;
    void emitActionArguments(CodeBuilder *builder, const IR::P4Action *action, cstring name);
    void emitKey(CodeBuilder *builder, cstring keyName);

    virtual void emitTypes(CodeBuilder *builder);
    virtual void emitInstance(CodeBuilder *builder);
    virtual void emitKeyType(CodeBuilder *builder);
    virtual void emitValueType(CodeBuilder *builder);
    virtual void emitValueActionIDNames(CodeBuilder *builder);
    virtual void emitValueStructStructure(CodeBuilder *builder);
    // Emits value types used by direct externs.
    virtual void emitDirectValueTypes(CodeBuilder *builder) { (void)builder; }
    virtual void emitAction(CodeBuilder *builder, cstring valueName, cstring actionRunVariable);
    virtual void emitInitializer(CodeBuilder *builder);
    virtual void emitLookup(CodeBuilder *builder, cstring key, cstring value);
    virtual void emitLookupDefault(CodeBuilder *builder, cstring key, cstring value,
                                   cstring actionRunVariable) {
        (void)actionRunVariable;
        builder->target->emitTableLookup(builder, defaultActionMapName, key, value);
        builder->endOfStatement(true);
    }
    virtual bool isMatchTypeSupported(const IR::Declaration_ID *matchType) {
        return matchType->name.name == P4::P4CoreLibrary::instance().exactMatch.name ||
               matchType->name.name == P4::P4CoreLibrary::instance().ternaryMatch.name ||
               matchType->name.name == P4::P4CoreLibrary::instance().lpmMatch.name;
    }
    // Whether to drop packet if no match entry found.
    // Some table implementations may want to continue processing.
    virtual bool dropOnNoMatchingEntryFound() const { return true; }

    virtual bool cacheEnabled() { return false; }
    virtual void emitCacheLookup(CodeBuilder *builder, cstring key, cstring value) {
        (void)builder;
        (void)key;
        (void)value;
    }
    virtual void emitCacheUpdate(CodeBuilder *builder, cstring key, cstring value) {
        (void)builder;
        (void)key;
        (void)value;
    }
};

class EBPFCounterTable : public EBPFTableBase {
 protected:
    size_t size;
    bool isHash;

 public:
    EBPFCounterTable(const EBPFProgram *program, const IR::ExternBlock *block, cstring name,
                     CodeGenInspector *codeGen);
    EBPFCounterTable(const EBPFProgram *program, cstring name, CodeGenInspector *codeGen,
                     size_t size, bool isHash)
        : EBPFTableBase(program, name, codeGen), size(size), isHash(isHash) {}
    virtual void emitTypes(CodeBuilder *);
    virtual void emitInstance(CodeBuilder *builder);
    virtual void emitCounterIncrement(CodeBuilder *builder,
                                      const IR::MethodCallExpression *expression);
    virtual void emitCounterAdd(CodeBuilder *builder, const IR::MethodCallExpression *expression);
    virtual void emitMethodInvocation(CodeBuilder *builder, const P4::ExternMethod *method);
};

class EBPFValueSet : public EBPFTableBase {
 protected:
    size_t size;
    const IR::P4ValueSet *pvs;
    std::vector<std::pair<cstring, const IR::Type *>> fieldNames;
    cstring keyVarName;

 public:
    EBPFValueSet(const EBPFProgram *program, const IR::P4ValueSet *p4vs, cstring instanceName,
                 CodeGenInspector *codeGen);

    void emitTypes(CodeBuilder *builder);
    void emitInstance(CodeBuilder *builder);
    void emitKeyInitializer(CodeBuilder *builder, const IR::SelectExpression *expression,
                            cstring varName);
    void emitLookup(CodeBuilder *builder);
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFTABLE_H_ */
