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

#ifndef BACKENDS_UBPF_UBPFTYPE_H_
#define BACKENDS_UBPF_UBPFTYPE_H_

#include <vector>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfType.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace UBPF {

class UBPFTypeFactory : public EBPF::EBPFTypeFactory {
 public:
    explicit UBPFTypeFactory(const P4::TypeMap *typeMap) : EBPF::EBPFTypeFactory(typeMap) {}

    static void createFactory(const P4::TypeMap *typeMap) {
        EBPF::EBPFTypeFactory::instance = new UBPFTypeFactory(typeMap);
    }

    static EBPFTypeFactory *getInstance() { return EBPF::EBPFTypeFactory::instance; }

    EBPF::EBPFType *create(const IR::Type *type) override;
};

class UBPFBoolType : public EBPF::EBPFBoolType {
 public:
    UBPFBoolType() : EBPF::EBPFBoolType() {}

    void emit(EBPF::CodeBuilder *builder) override { builder->append("uint8_t"); }
};

class UBPFScalarType : public EBPF::EBPFScalarType {
 public:
    explicit UBPFScalarType(const IR::Type_Bits *bits) : EBPF::EBPFScalarType(bits) {}

    void emit(EBPF::CodeBuilder *builder) override;

    cstring getAsString();

    void declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
};

class UBPFStructType : public EBPF::EBPFStructType {
 public:
    explicit UBPFStructType(const IR::Type_StructLike *strct) : EBPF::EBPFStructType(strct) {}
    void emit(EBPF::CodeBuilder *builder) override;
    void declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
};

class UBPFEnumType : public EBPF::EBPFEnumType {
 public:
    explicit UBPFEnumType(const IR::Type_Enum *strct) : EBPF::EBPFEnumType(strct) {}

    void emit(EBPF::CodeBuilder *builder) override;
};

class UBPFListType : public EBPF::EBPFType, public EBPF::IHasWidth {
    class UBPFListElement {
     public:
        EBPFType *type;
        const cstring name;

        UBPFListElement(EBPFType *type, const cstring name) : type(type), name(name) {}
        virtual ~UBPFListElement() {}  // to make UBPFListElement polymorphic.
        template <typename T>
        bool is() const {
            return dynamic_cast<const T *>(this) != nullptr;
        }
        template <typename T>
        T *to() {
            return dynamic_cast<T *>(this);
        }
    };

    class Padding : public UBPFListElement {
     public:
        unsigned widthInBytes;

        Padding(const cstring name, unsigned widthInBytes)
            : UBPFListElement(nullptr, name), widthInBytes(widthInBytes) {}
    };

 public:
    cstring kind;
    cstring name;
    std::vector<UBPFListElement *> elements;
    unsigned width;
    unsigned implWidth;

    explicit UBPFListType(const IR::Type_List *lst);
    void emitPadding(EBPF::CodeBuilder *builder, Padding *padding);
    void declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(EBPF::CodeBuilder *builder) override;
    unsigned widthInBits() override { return width; }
    unsigned implementationWidthInBits() override { return implWidth; }
    void emit(EBPF::CodeBuilder *builder) override;
};

}  // namespace UBPF

#endif /* BACKENDS_UBPF_UBPFTYPE_H_ */
