#ifndef BACKENDS_EBPF_PSA_EBPFPSATABLE_H_
#define BACKENDS_EBPF_PSA_EBPFPSATABLE_H_

#include "frontends/p4/methodInstance.h"
#include "backends/ebpf/ebpfTable.h"
#include "ebpfPsaControl.h"

namespace EBPF {

class EBPFTablePSA : public EBPFTable {
 private:
    void emitTableDecl(CodeBuilder *builder,
                       cstring tblName,
                       TableKind kind,
                       cstring keyTypeName,
                       cstring valueTypeName,
                       size_t size) const;

 protected:
    void emitTableValue(CodeBuilder* builder, const IR::MethodCallExpression* actionMce,
                        cstring valueName);
    void emitDefaultActionInitializer(CodeBuilder *builder);
    void emitConstEntriesInitializer(CodeBuilder *builder);
    void emitMapUpdateTraceMsg(CodeBuilder *builder, cstring mapName,
                               cstring returnCode) const;

 public:
    cstring name;
    size_t size;

    EBPFTablePSA(const EBPFProgram* program, const IR::TableBlock* table,
                 CodeGenInspector* codeGen, cstring name, size_t size);
    void emitInstance(CodeBuilder* builder) override;
    void emitTypes(CodeBuilder* builder) override;
    void emitValueActionIDNames(CodeBuilder* builder) override;
    void emitValueStructStructure(CodeBuilder* builder) override;
    void emitAction(CodeBuilder* builder, cstring valueName, cstring actionRunVariable) override;
    void emitInitializer(CodeBuilder* builder) override;
    void emitLookup(CodeBuilder* builder, cstring key, cstring value) override;
    void emitLookupDefault(CodeBuilder* builder, cstring key, cstring value) override;
    bool dropOnNoMatchingEntryFound() const override;
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSATABLE_H_ */
