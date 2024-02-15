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

#ifndef BACKENDS_EBPF_EBPFTYPE_H_
#define BACKENDS_EBPF_EBPFTYPE_H_

#include "ebpfObject.h"
#include "ir/ir.h"
#include "lib/algorithm.h"
#include "lib/rtti.h"
#include "lib/sourceCodeBuilder.h"

namespace EBPF {

// Base class for EBPF types
class EBPFType : public EBPFObject {
 protected:
    explicit EBPFType(const IR::Type *type) : type(type) {}

 public:
    const IR::Type *type;
    virtual void emit(CodeBuilder *builder) = 0;
    virtual void declare(CodeBuilder *builder, cstring id, bool asPointer) = 0;
    virtual void declareInit(CodeBuilder *builder, cstring id, bool asPointer) = 0;
    virtual void emitInitializer(CodeBuilder *builder) = 0;
    virtual void declareArray(CodeBuilder * /*builder*/, cstring /*id*/, unsigned /*size*/) {
        BUG("%1%: unsupported array", type);
    }

    DECLARE_TYPEINFO(EBPFType, EBPFObject);
};

class IHasWidth : public ICastable {
 public:
    virtual ~IHasWidth() {}
    // P4 width
    virtual unsigned widthInBits() const = 0;
    // Width in the target implementation.
    // Currently a multiple of 8.
    virtual unsigned implementationWidthInBits() const = 0;

    DECLARE_TYPEINFO(IHasWidth);
};

class EBPFTypeFactory {
 protected:
    const P4::TypeMap *typeMap;
    explicit EBPFTypeFactory(const P4::TypeMap *typeMap) : typeMap(typeMap) { CHECK_NULL(typeMap); }

 public:
    static EBPFTypeFactory *instance;
    static void createFactory(const P4::TypeMap *typeMap) {
        EBPFTypeFactory::instance = new EBPFTypeFactory(typeMap);
    }
    virtual EBPFType *create(const IR::Type *type);
};

class EBPFBoolType : public EBPFType, public IHasWidth {
 public:
    EBPFBoolType() : EBPFType(IR::Type_Boolean::get()) {}
    void emit(CodeBuilder *builder) override { builder->append("u8"); }
    void declare(CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder *builder) override { builder->append("0"); }
    unsigned widthInBits() const override { return 1; }
    unsigned implementationWidthInBits() const override { return 8; }

    DECLARE_TYPEINFO(EBPFBoolType, EBPFType, IHasWidth);
};

class EBPFStackType : public EBPFType, public IHasWidth {
    EBPFType *elementType;
    unsigned size;

 public:
    EBPFStackType(const IR::Type_Stack *type, EBPFType *elementType)
        : EBPFType(type), elementType(elementType), size(type->getSize()) {
        CHECK_NULL(type);
        CHECK_NULL(elementType);
        BUG_CHECK(elementType->is<IHasWidth>(), "Unexpected element type %1%",
                  typeid(*elementType).name());
    }
    void emit(CodeBuilder *) override {}
    void declare(CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder *builder) override;
    unsigned widthInBits() const override;
    unsigned implementationWidthInBits() const override;

    DECLARE_TYPEINFO(EBPFStackType, EBPFType, IHasWidth);
};

class EBPFScalarType : public EBPFType, public IHasWidth {
 public:
    const unsigned width;
    const bool isSigned;
    explicit EBPFScalarType(const IR::Type_Bits *bits)
        : EBPFType(bits), width(bits->size), isSigned(bits->isSigned) {}
    explicit EBPFScalarType(const IR::Type_Varbits *bits)
        : EBPFType(bits), width(bits->size), isSigned(false) {}
    unsigned bytesRequired() const { return ROUNDUP(width, 8); }
    unsigned alignment() const;
    void emit(CodeBuilder *builder) override;
    void declare(CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder *builder) override;
    unsigned widthInBits() const override { return width; }
    unsigned implementationWidthInBits() const override { return bytesRequired() * 8; }
    // True if this width is small enough to store in a machine scalar
    static bool generatesScalar(unsigned width) { return width <= 64; }

    DECLARE_TYPEINFO(EBPFScalarType, EBPFType, IHasWidth);
};

// This should not always implement IHasWidth, but it may...
class EBPFTypeName : public EBPFType, public IHasWidth {
    const IR::Type_Name *type;
    EBPFType *canonical;

 public:
    EBPFTypeName(const IR::Type_Name *type, EBPFType *canonical)
        : EBPFType(type), type(type), canonical(canonical) {}
    void emit(CodeBuilder *builder) override { canonical->emit(builder); }
    void declare(CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder *builder) override;
    unsigned widthInBits() const override;
    unsigned implementationWidthInBits() const override;
    void declareArray(CodeBuilder *builder, cstring id, unsigned size) override;

    template <typename T>
    bool canonicalTypeIs() const {
        return this->canonical->is<T>();
    }

    DECLARE_TYPEINFO(EBPFTypeName, EBPFType, IHasWidth);
};

// Also represents headers and unions
class EBPFStructType : public EBPFType, public IHasWidth {
    class EBPFField {
     public:
        cstring comment;
        EBPFType *type;
        const IR::StructField *field;

        EBPFField(EBPFType *type, const IR::StructField *field, cstring comment = nullptr)
            : comment(comment), type(type), field(field) {}
    };

 public:
    cstring kind;
    cstring name;
    std::vector<EBPFField *> fields;
    unsigned width;
    unsigned implWidth;

    explicit EBPFStructType(const IR::Type_StructLike *strct);
    void declare(CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder *builder) override;
    unsigned widthInBits() const override { return width; }
    unsigned implementationWidthInBits() const override { return implWidth; }
    void emit(CodeBuilder *builder) override;
    void declareArray(CodeBuilder *builder, cstring id, unsigned size) override;

    DECLARE_TYPEINFO(EBPFStructType, EBPFType, IHasWidth);
};

class EBPFEnumType : public EBPFType, public EBPF::IHasWidth {
 public:
    explicit EBPFEnumType(const IR::Type_Enum *type) : EBPFType(type) {}
    void emit(CodeBuilder *builder) override;
    void declare(CodeBuilder *builder, cstring id, bool asPointer) override;
    void declareInit(CodeBuilder *builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder *builder) override { builder->append("0"); }
    unsigned widthInBits() const override { return 32; }
    unsigned implementationWidthInBits() const override { return 32; }
    const IR::Type_Enum *getType() const { return type->to<IR::Type_Enum>(); }

    DECLARE_TYPEINFO(EBPFEnumType, EBPFType, IHasWidth);
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_EBPFTYPE_H_ */
