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

#include "ebpfType.h"

namespace EBPF {

EBPFTypeFactory* EBPFTypeFactory::instance;

EBPFType* EBPFTypeFactory::create(const IR::Type* type) {
    CHECK_NULL(type);
    CHECK_NULL(typeMap);
    EBPFType* result = nullptr;
    if (type->is<IR::Type_Boolean>()) {
        result = new EBPFBoolType(builder);
    } else if (type->is<IR::Type_Bits>()) {
        result = new EBPFScalarType(type->to<IR::Type_Bits>(), builder);
    } else if (type->is<IR::Type_StructLike>()) {
        result = new EBPFStructType(type->to<IR::Type_StructLike>(), builder);
    } else if (type->is<IR::Type_Typedef>()) {
        auto canon = typeMap->getTypeType(type, true);
        result = create(canon);
        auto path = new IR::Path(type->to<IR::Type_Typedef>()->name);
        result = new EBPFTypeName(new IR::Type_Name(Util::SourceInfo(), path), result, builder);
    } else if (type->is<IR::Type_Name>()) {
        auto canon = typeMap->getTypeType(type, true);
        result = create(canon);
        result = new EBPFTypeName(type->to<IR::Type_Name>(), result, builder);
    } else {
        ::error("Type %1% unsupported by EBPF", type);
    }

    return result;
}

void
EBPFBoolType::declare(cstring id, bool asPointer) {
    emit();
    if (asPointer)
        builder->append("*");
    builder->appendFormat(" %s", id.c_str());
}

/////////////////////////////////////////////////////////////

unsigned EBPFScalarType::alignment() const {
    if (width <= 8)
        return 1;
    else if (width <= 16)
        return 2;
    else if (width <= 32)
        return 4;
    else
        // compiled as char*
        return 1;
}

void EBPFScalarType::emit() {
    auto prefix = isSigned ? "i" : "u";

    if (width <= 8)
        builder->appendFormat("%s8", prefix);
    else if (width <= 16)
        builder->appendFormat("%s16", prefix);
    else if (width <= 32)
        builder->appendFormat("%s32", prefix);
    else
        builder->appendFormat("char*");
}

void
EBPFScalarType::declare(cstring id, bool asPointer) {
    if (width <= 32) {
        emit();
        if (asPointer)
            builder->append("*");
        builder->spc();
        builder->append(id);
    } else {
        if (asPointer)
            builder->append("char*");
        else
            builder->appendFormat("char %s[%d]", id.c_str(), bytesRequired());
    }
}

//////////////////////////////////////////////////////////

EBPFStructType::EBPFStructType(const IR::Type_StructLike* strct, CodeBuilder* builder) :
        EBPFType(strct, builder) {
    if (strct->is<IR::Type_Struct>())
        kind = "struct";
    else if (strct->is<IR::Type_Header>())
        kind = "struct";
    else if (strct->is<IR::Type_Union>())
        kind = "union";
    else
        BUG("Unexpected struct type %1%", strct);
    name = strct->name.name;
    width = 0;
    implWidth = 0;

    for (auto f : *strct->fields) {
        auto type = EBPFTypeFactory::instance->create(f->type);
        auto wt = dynamic_cast<IHasWidth*>(type);
        if (wt == nullptr) {
            ::error("EBPF: Unsupported type in struct: %s", f->type);
        } else {
            width += wt->widthInBits();
            implWidth += wt->implementationWidthInBits();
        }
        fields.push_back(new EBPFField(type, f));
    }
}

void
EBPFStructType::declare(cstring id, bool asPointer) {
    builder->append(kind);
    if (asPointer)
        builder->append("*");
    const char* n = name.c_str();
    builder->appendFormat(" %s %s", n, id.c_str());
}

void EBPFStructType::emitInitializer() {
    builder->blockStart();
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_Union>()) {
        for (auto f : fields) {
            builder->emitIndent();
            builder->appendFormat(".%s = ", f->field->name.name);
            f->type->emitInitializer();
            builder->append(",");
            builder->newline();
        }
    } else if (type->is<IR::Type_Header>()) {
        builder->emitIndent();
        builder->appendLine(".ebpf_valid = 0");
    } else {
        BUG("Unexpected type %1%", type);
    }
    builder->blockEnd(false);
}

void EBPFStructType::emit() {
    builder->emitIndent();
    builder->append(kind);
    builder->spc();
    builder->append(name);
    builder->spc();
    builder->blockStart();

    for (auto f : fields) {
        auto type = f->type;
        builder->emitIndent();

        type->declare(f->field->name, false);
        builder->append("; ");
        builder->append("/* ");
        builder->append(type->type->toString());
        if (f->comment != nullptr) {
            builder->append(" ");
            builder->append(f->comment);
        }
        builder->append(" */");
        builder->newline();
    }

    if (type->is<IR::Type_Header>()) {
        builder->emitIndent();
        auto type = EBPFTypeFactory::instance->create(IR::Type_Boolean::get());
        if (type != nullptr) {
            type->declare("ebpf_valid", false);
            builder->endOfStatement(true);
        }
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

///////////////////////////////////////////////////////////////

void EBPFTypeName::declare(cstring id, bool asPointer) {
    if (canonical != nullptr)
        canonical->declare(id, asPointer);
}

void EBPFTypeName::emitInitializer() {
    if (canonical != nullptr)
        canonical->emitInitializer();
}

unsigned EBPFTypeName::widthInBits() {
    auto wt = dynamic_cast<IHasWidth*>(canonical);
    if (wt == nullptr) {
        ::error("Type %1% does not have a fixed witdh", type);
        return 0;
    }
    return wt->widthInBits();
}

unsigned EBPFTypeName::implementationWidthInBits() {
    auto wt = dynamic_cast<IHasWidth*>(canonical);
    if (wt == nullptr) {
        ::error("Type %1% does not have a fixed witdh", type);
        return 0;
    }
    return wt->implementationWidthInBits();
}

}  // namespace EBPF
