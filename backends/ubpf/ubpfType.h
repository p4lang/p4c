
#ifndef P4C_UBPFTYPE_H
#define P4C_UBPFTYPE_H

#include "backends/ebpf/ebpfType.h"
#include "lib/sourceCodeBuilder.h"

namespace UBPF {

class UBPFTypeFactory : public EBPF::EBPFTypeFactory {
public:
    UBPFTypeFactory(const P4::TypeMap* typeMap) : EBPF::EBPFTypeFactory(typeMap) {}
    static void createFactory(const P4::TypeMap* typeMap) {
        EBPF::EBPFTypeFactory::instance = new UBPFTypeFactory(typeMap);
    }
    static EBPFTypeFactory* getInstance() {
        return EBPF::EBPFTypeFactory::instance;
    }
    EBPF::EBPFType* create(const IR::Type* type) override;
};

class UBPFScalarType : public EBPF::EBPFScalarType {
public:
    UBPFScalarType(const IR::Type_Bits* bits) : EBPF::EBPFScalarType(bits) {}
    void emit(EBPF::CodeBuilder* builder) override;
//    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
};


}

#endif //P4C_UBPFTYPE_H
