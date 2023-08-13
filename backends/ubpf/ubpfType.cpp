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

#include <algorithm>
#include <string>

#include "backends/ebpf/ebpfType.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "ubpfHelpers.h"

namespace UBPF {

EBPF::EBPFTypeFactory *instance = UBPFTypeFactory::getInstance();

EBPF::EBPFType *UBPFTypeFactory::create(const IR::Type *type) {
    EBPF::EBPFType *result = nullptr;
    if (type->is<IR::Type_Boolean>()) {
        result = new UBPFBoolType();
    } else if (auto bt = type->to<IR::Type_Bits>()) {
        result = new UBPFScalarType(bt);
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
        result = new UBPFEnumType(te);
    } else if (auto ts = type->to<IR::Type_Stack>()) {
        auto et = create(ts->elementType);
        if (et == nullptr) return nullptr;
        result = new EBPF::EBPFStackType(ts, et);
    } else if (auto tpl = type->to<IR::Type_List>()) {
        result = new UBPFListType(tpl);
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Type %1% not supported", type);
    }
    return result;
}

void UBPFScalarType::emit(EBPF::CodeBuilder *builder) {
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

cstring UBPFScalarType::getAsString() {
    if (width <= 8)
        return cstring("uint8_t");
    else if (width <= 16)
        return cstring("uint16_t");
    else if (width <= 32)
        return cstring("uint32_t");
    else if (width <= 64)
        return cstring("uint64_t");
    else
        return cstring("uint8_t*");
}

void UBPFScalarType::declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) {
    if (EBPFScalarType::generatesScalar(width)) {
        emit(builder);
        if (asPointer) builder->append("*");
        builder->spc();
        builder->append(id);
    } else {
        if (asPointer)
            builder->append("uint8_t*");
        else
            builder->appendFormat("uint8_t %s[%d]", id.c_str(), bytesRequired());
    }
}

void UBPFScalarType::declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) {
    if (EBPFScalarType::generatesScalar(width)) {
        emit(builder);
        if (asPointer) builder->append("*");
        builder->spc();
        id = id + cstring(" = 0");
        builder->append(id);
    } else {
        if (asPointer)
            builder->append("uint8_t*");
        else
            builder->appendFormat("uint8_t %s[%d]", id.c_str(), bytesRequired());
    }
}

void UBPFStructType::emit(EBPF::CodeBuilder *builder) {
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

void UBPFStructType::declare(EBPF::CodeBuilder *builder, cstring id, bool asPointer) {
    builder->append(kind);
    builder->appendFormat(" %s ", name.c_str());
    if (asPointer) builder->append("*");
    builder->appendFormat("%s", id.c_str());
}

void UBPFStructType::declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) {
    declare(builder, id, asPointer);
}
//////////////////////////////////////////////////////////

void UBPFEnumType::emit(EBPF::CodeBuilder *builder) {
    builder->append("enum ");
    auto et = getType();
    builder->append(et->name);
    builder->blockStart();
    for (auto m : et->members) {
        builder->append(m->name);
        builder->appendLine(",");
    }
    builder->blockEnd(false);
    builder->endOfStatement(true);
}

//////////////////////////////////////////////////////////

UBPFListType::UBPFListType(const IR::Type_List *lst) : EBPFType(lst) {
    kind = "struct";
    width = 0;
    implWidth = 0;
    // The first iteration is to compute total width of Type_List.
    for (auto el : lst->components) {
        auto ltype = UBPFTypeFactory::instance->create(el);
        auto wt = dynamic_cast<IHasWidth *>(ltype);
        if (wt == nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "UBPF: Unsupported type in Type_List: %s",
                    el->getP4Type());
        } else {
            width += wt->widthInBits();
            implWidth += wt->implementationWidthInBits();
        }
    }
    unsigned int overallPadding = 16 - implWidth % 16;
    implWidth += overallPadding;  // The total implementation width equals the total
                                  // width of all fields + padding.

    unsigned idx = 0, paddingIndex = 0;
    // The second iteration converts list's elements to UBPFListElement or Padding.
    for (auto el : lst->components) {
        auto etype = UBPFTypeFactory::instance->create(el);
        cstring fname = "field_" + std::to_string(idx);
        elements.push_back(new UBPFListElement(etype, fname));

        auto typeWidth = etype->to<EBPF::IHasWidth>();
        auto remainingBits =
            std::min(16 - typeWidth->implementationWidthInBits() % 16, overallPadding);
        if (remainingBits <= 16 && remainingBits != 0 &&
            typeWidth->implementationWidthInBits() % 16 != 0) {
            if (remainingBits == 8 || remainingBits == 16) {
                cstring name = "pad" + std::to_string(paddingIndex);
                unsigned widthInBytes = remainingBits / 8;
                auto pad = new Padding(name, widthInBytes);
                elements.push_back(pad);
                paddingIndex++;
            } else {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Not supported bitwidth in %1%",
                        this->type->getNode());
            }
        }
        idx++;
    }
}

void UBPFListType::declare(EBPF::CodeBuilder *builder, cstring id, UNUSED bool asPointer) {
    builder->append(kind);
    builder->spc();
    builder->append(name);
    builder->spc();
    builder->append(id.c_str());
}

void UBPFListType::declareInit(EBPF::CodeBuilder *builder, cstring id, bool asPointer) {
    declare(builder, id, asPointer);
}

void UBPFListType::emitInitializer(EBPF::CodeBuilder *builder) {
    builder->blockStart();
    for (auto f : elements) {
        if (!f->is<Padding>()) continue;
        builder->emitIndent();
        builder->appendFormat(".%s = {0},", f->to<Padding>()->name);
        builder->newline();
    }
    builder->blockEnd(false);
}

void UBPFListType::emitPadding(EBPF::CodeBuilder *builder, UBPF::UBPFListType::Padding *pad) {
    builder->appendFormat("uint8_t %s[%u]", pad->name, pad->widthInBytes);
}

/***
 * This method emits uBPF code for Type_List (tuples). As the implementation of
 * hash() extern requires n-byte aligned structs, this method appends paddings between fields.
 */
void UBPFListType::emit(EBPF::CodeBuilder *builder) {
    for (auto f : elements) {
        builder->emitIndent();
        if (!f->is<Padding>())
            f->type->declare(builder, f->name, false);
        else
            emitPadding(builder, f->to<Padding>());
        builder->append("; ");
        builder->newline();
    }
}

}  // namespace UBPF
