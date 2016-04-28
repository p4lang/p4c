#ifndef _BACKENDS_EBPF_EBPFTABLE_H_
#define _BACKENDS_EBPF_EBPFTABLE_H_

#include "ebpfObject.h"
#include "../../frontends/p4/methodInstance.h"

namespace EBPF {
// Also used to represent counters
class EBPFTableBase : public EBPFObject {
 public:
    const EBPFProgram* program;

    cstring instanceName;
    cstring keyTypeName;
    cstring valueTypeName;
    cstring dataMapName;

 protected:
    EBPFTableBase(const EBPFProgram* program, cstring instanceName) :
            program(program), instanceName(instanceName) {
        keyTypeName = program->refMap->newName(instanceName + "_key");
        valueTypeName = program->refMap->newName(instanceName + "_value");
        dataMapName = instanceName;
    }
};

class EBPFTable final : public EBPFTableBase {
 protected:
    const IR::Key*            keyGenerator;
    const IR::ActionList*     actionList;

 public:
    const IR::TableBlock*    table;
    cstring               defaultActionMapName;
    cstring               actionEnumName;

    EBPFTable(const EBPFProgram* program, const IR::TableBlock* table);
    void emit(CodeBuilder* builder) override;
    void emitActionArguments(CodeBuilder* builder, const IR::P4Action* action,
                             cstring name);
    void emitKeyType(CodeBuilder* builder);
    void emitValueType(CodeBuilder* builder);
    void createKey(CodeBuilder* builder, cstring keyName);
    void runAction(CodeBuilder* builder, cstring valueName);
};

class EBPFCounterTable final : public EBPFTableBase {
    size_t    size;
    bool      isHash;
 public:
    EBPFCounterTable(const EBPFProgram* program, const IR::ExternBlock* block, cstring name);
    void emit(CodeBuilder* builder) override;
    void emitCounterIncrement(CodeBuilder* builder,
                              const IR::MethodCallExpression* expression);
    void emitMethodInvocation(CodeBuilder* builder, const P4::ExternMethod* method);
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFTABLE_H_ */
