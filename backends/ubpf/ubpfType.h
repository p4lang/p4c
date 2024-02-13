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

#include "backends/ebpf/ebpfType.h"
#include "lib/sourceCodeBuilder.h"

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

    DECLARE_TYPEINFO(UBPFBoolType, EBPF::EBPFBoolType);
};

class UBPFScalarType : public EBPF::EBPFScalarType {
 public:
    explicit UBPFScalarType(const IR::Type_Bits *bits) : EBPF::EBPFScalarType(bits) {}

    void emit(EBPF::CodeBuilder *builder) override;

    cstring getAsString();

    void declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;

    DECLARE_TYPEINFO(UBPFScalarType, EBPF::EBPFScalarType);
};

class UBPFStructType : public EBPF::EBPFStructType {
 public:
    explicit UBPFStructType(const IR::Type_StructLike *strct) : EBPF::EBPFStructType(strct) {}
    void emit(EBPF::CodeBuilder *builder) override;
    void declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) override;

    DECLARE_TYPEINFO(UBPFStructType, EBPF::EBPFStructType);
};

class UBPFEnumType : public EBPF::EBPFEnumType {
 public:
    explicit UBPFEnumType(const IR::Type_Enum *strct) : EBPF::EBPFEnumType(strct) {}

    void emit(EBPF::CodeBuilder *builder) override;

    DECLARE_TYPEINFO(UBPFEnumType, EBPF::EBPFEnumType);
};

class UBPFListType : public EBPF::EBPFType, public EBPF::IHasWidth {
    class UBPFListElement : public ICastable {
     public:
        EBPFType *type;
        const cstring name;

        UBPFListElement(EBPFType *type, const cstring name) : type(type), name(name) {}
        virtual ~UBPFListElement() {}  // to make UBPFListElement polymorphic.

        DECLARE_TYPEINFO(UBPFListElement);
    };

    class Padding : public UBPFListElement {
     public:
        unsigned widthInBytes;

        Padding(const cstring name, unsigned widthInBytes)
            : UBPFListElement(nullptr, name), widthInBytes(widthInBytes) {}

        DECLARE_TYPEINFO(Padding, UBPFListElement);
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
    unsigned widthInBits() const override { return width; }
    unsigned implementationWidthInBits() const override { return implWidth; }
    void emit(EBPF::CodeBuilder *builder) override;

    DECLARE_TYPEINFO(UBPFListType, EBPF::EBPFType, EBPF::IHasWidth);
};

}  // namespace UBPF

#endif /* BACKENDS_UBPF_UBPFTYPE_H_ */
