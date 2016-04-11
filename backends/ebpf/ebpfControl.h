#ifndef _BACKENDS_EBPF_EBPFCONTROL_H_
#define _BACKENDS_EBPF_EBPFCONTROL_H_

#include "ebpfObject.h"
#include "ebpfTable.h"

namespace EBPF {

class EBPFControl : public EBPFObject {
 public:
    const EBPFProgram*            program;
    const IR::ControlBlock*       controlBlock;
    const IR::Parameter*          headers;
    const IR::Parameter*          accept;

    std::set<const IR::Parameter*> toDereference;
    std::map<cstring, EBPFTable*>  tables;
    std::map<cstring, EBPFCounterTable*>  counters;

    explicit EBPFControl(const EBPFProgram* program, const IR::ControlBlock* block);
    virtual ~EBPFControl() {}
    void emit(CodeBuilder* builder);
    void emitTables(CodeBuilder* builder);
    bool build();
    EBPFTable* getTable(cstring name) const {
        auto result = get(tables, name);
        BUG_CHECK(result != nullptr, "No table named %1%", name);
        return result; }
    EBPFCounterTable* getCounter(cstring name) const {
        auto result = get(counters, name);
        BUG_CHECK(result != nullptr, "No counter named %1%", name);
        return result; }
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFCONTROL_H_ */
