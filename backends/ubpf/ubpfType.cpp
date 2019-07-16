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

#include "ubpfType.h"

namespace UBPF {

    EBPF::EBPFTypeFactory* instance = UBPFTypeFactory::getInstance();

    EBPF::EBPFType* UBPFTypeFactory::create(const IR::Type *type) {

        EBPF::EBPFType* result = nullptr;
        if (type->is<IR::Type_Boolean>()) {
            result = new UBPFBoolType();
        } else if (auto bt = type->to<IR::Type_Bits>()) {
            result = new UBPFScalarType(bt); // using UBPF Scalar Type
        } else if (auto st = type->to<IR::Type_StructLike>()) {
            result = new UBPFStructType(st);
        } else if (auto tt = type->to<IR::Type_Typedef>()) {
            auto canon = typeMap->getTypeType(type, true);
            result = create(canon);
            auto path = new IR::Path(tt->name);
            result = new EBPF::EBPFTypeName(new IR::Type_Name(path), result);
        } else if (auto tn = type->to<IR::Type_Name>()) {
            auto canon = typeMap->getTypeType(type, true);
            result = create(canon);
            result = new EBPF::EBPFTypeName(tn, result);
        } else if (auto te = type->to<IR::Type_Enum>()) {
            result = new EBPF::EBPFEnumType(te);
        } else if (auto ts = type->to<IR::Type_Stack>()) {
            auto et = create(ts->elementType);
            if (et == nullptr)
                return nullptr;
            result = new EBPF::EBPFStackType(ts, et);
        } else {
            ::error("Type %1% not supported", type);
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////

    void UBPFScalarType::emit(EBPF::CodeBuilder* builder) {
        if (width <= 8)
            builder->appendFormat("uint8_t");
        else if (width <= 16)
            builder->appendFormat("uint16_t");
        else if (width <= 32)
            builder->appendFormat("uint32_t");
        else if (width <= 64)
            builder->appendFormat("uint64_t");
        else
            builder->appendFormat("uint8_t*");

    }

    void UBPFScalarType::declare(EBPF::CodeBuilder* builder, cstring id, bool asPointer) {
        if (EBPFScalarType::generatesScalar(width)) {
            emit(builder);
            if (asPointer)
                builder->append("*");
            builder->spc();
            builder->append(id);
        } else {
            if (asPointer)
                builder->append("uint8_t*");
            else
                builder->appendFormat("uint8_t %s[%d]", id.c_str(), bytesRequired());
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    void UBPFStructType::emit(EBPF::CodeBuilder* builder) {
        builder->emitIndent();
        builder->append(kind);
        builder->spc();
        builder->append(name);
        builder->spc();
        builder->blockStart();

        for (auto f : fields) {
            auto ltype = f->type;

            builder->emitIndent();

            ltype->declare(builder, f->field->name, false);
            builder->append("; ");
            builder->append("/* ");
            builder->append(ltype->type->toString());
            if (f->comment != nullptr) {
                builder->append(" ");
                builder->append(f->comment);
            }
            builder->append(" */");
            builder->newline();

            if (type->is<IR::Type_Header>()) {
                // add offset field
                builder->emitIndent();
                builder->appendFormat("int %sOffset;", f->field->name.name);
                builder->newline();
            }
        }

        if (type->is<IR::Type_Header>()) {
            builder->emitIndent();
            auto type = UBPFTypeFactory::instance->create(IR::Type_Boolean::get());
            if (type != nullptr) {
                type->declare(builder, "ebpf_valid", false);
                builder->endOfStatement(true);
            }
        }

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }



}
