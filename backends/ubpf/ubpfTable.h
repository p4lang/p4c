//
// Created by mateusz on 19.04.19.
//

#ifndef P4C_UBPFTABLE_H
#define P4C_UBPFTABLE_H

#endif //P4C_UBPFTABLE_H

#include "backends/ebpf/ebpfObject.h"
#include "ubpfProgram.h"
#include "frontends/p4/methodInstance.h"


namespace UBPF {

    class UBPFTableBase : public EBPF::EBPFObject {
    public:
        const UBPFProgram *program;

        cstring instanceName;
        cstring keyTypeName;
        cstring valueTypeName;
        cstring dataMapName;
        EBPF::CodeGenInspector *codeGen;

    protected:
        UBPFTableBase(const UBPFProgram *program, cstring instanceName,
                      EBPF::CodeGenInspector *codeGen) :
                program(program), instanceName(instanceName), codeGen(codeGen) {
            CHECK_NULL(codeGen);
            CHECK_NULL(program);
            keyTypeName = program->refMap->newName(instanceName + "_key");
            valueTypeName = program->refMap->newName(instanceName + "_value");
            dataMapName = instanceName;
        }
    };

    class UBPFTable final : public UBPFTableBase {
    public:
        const IR::Key *keyGenerator;
        const IR::ActionList *actionList;
        const IR::TableBlock *table;
        cstring defaultActionMapName;
        cstring actionEnumName;
        std::map<const IR::KeyElement *, cstring> keyFieldNames;
        std::map<const IR::KeyElement *, EBPF::EBPFType *> keyTypes;

        UBPFTable(const UBPFProgram *program, const IR::TableBlock *table, EBPF::CodeGenInspector *codeGen);

        void emitTypes(EBPF::CodeBuilder *builder);

        void emitInstance(EBPF::CodeBuilder *builder);

        void emitActionArguments(EBPF::CodeBuilder *builder, const IR::P4Action *action, cstring name);

        void emitKeyType(EBPF::CodeBuilder *builder);

        void emitValueType(EBPF::CodeBuilder *builder);

        void emitKey(EBPF::CodeBuilder *builder, cstring keyName);

        void emitAction(EBPF::CodeBuilder *builder, cstring valueName);

        void emitInitializer(EBPF::CodeBuilder *builder);
    };

}