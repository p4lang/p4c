/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ir/ir.h"

/* static */ size_t IR::BFN::ContainerRef::nextId = 0;

IR::InstanceRef::InstanceRef(cstring prefix, IR::ID n, const IR::Type *t,
                             const IR::Vector<IR::Annotation> *ann)
    : HeaderRef(t), name(n), nested() {
    if (n.srcInfo.isValid()) srcInfo = n.srcInfo;
    if (prefix) name.name = prefix + "." + n.name;
    IR::HeaderOrMetadata *obj = nullptr;
    if (auto *hdr = t->to<IR::Type_Header>()) {
        obj = new IR::Header(name, hdr);
    } else if (auto *meta = t->to<IR::Type_Struct>()) {
        obj = new IR::Metadata(name, meta);
        for (auto f : meta->fields)
            if (f->type->is<IR::Type_StructLike>() || f->type->is<IR::Type_Stack>())
                nested.add(name + "." + f->name, new InstanceRef(name, f->name, f->type));
    } else if (auto *stk = t->to<IR::Type_Stack>()) {
        if (stk->elementType->is<IR::Type_HeaderUnion>()) {
            P4C_UNIMPLEMENTED("Unsupported type %s %s", stk, n);
        }
        obj = new IR::HeaderStack(name, stk->elementType->to<IR::Type_Header>(), stk->getSize());
    } else if (t->is<IR::Type::Bits>() || t->is<IR::Type::Boolean>() || t->is<IR::Type_Set>()) {
        obj = nullptr;
    } else if (t->is<IR::Type_Enum>()) {
        // non-converted enum type -- probably StatefulAlu predicate output.
        obj = nullptr;
    } else {
        P4C_UNIMPLEMENTED("Unsupported type %s %s", t, n);
    }
    if (obj && ann) obj->annotations = *ann;
    this->obj = obj;
}

/* a V1InstanceRef is the same as an InstanceRef except we ignore the prefix -- see tofino.def
 * we also don't handle StructFlexible as that never occurs in V1 */
IR::V1InstanceRef::V1InstanceRef(cstring /* prefix */, IR::ID n, const IR::Type *t,
                                 const IR::Vector<IR::Annotation> *ann)
    : InstanceRef(t) {
    name = n;
    if (n.srcInfo.isValid()) srcInfo = n.srcInfo;
    IR::HeaderOrMetadata *obj = nullptr;
    if (auto *hdr = t->to<IR::Type_Header>()) {
        obj = new IR::Header(n, hdr);
    } else if (auto *meta = t->to<IR::Type_Struct>()) {
        obj = new IR::Metadata(n, meta);
        for (auto f : meta->fields)
            if (f->type->is<IR::Type_StructLike>() || f->type->is<IR::Type_Stack>())
                nested.add(f->name, new V1InstanceRef(n, f->name, f->type));
    } else if (auto *stk = t->to<IR::Type_Stack>()) {
        obj = new IR::HeaderStack(n, stk->elementType->to<IR::Type_Header>(), stk->getSize());
    } else if (t->is<IR::Type::Bits>() || t->is<IR::Type::Boolean>() || t->is<IR::Type_Set>()) {
        obj = nullptr;
    } else {
        P4C_UNIMPLEMENTED("Unsupported P4_14 type %s %s", t, n);
    }
    if (obj && ann) obj->annotations = *ann;
    this->obj = obj;
}

int IR::TempVar::uid = 0;

int IR::Padding::uid = 0;

bool IR::BFN::Pipe::has_pragma(const char *name) const {
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name == name) return true;
    }
    return false;
}
