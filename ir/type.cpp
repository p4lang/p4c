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

#include <utility>
#include "ir.h"

namespace IR {

const cstring IR::Type_Stack::next = "next";
const cstring IR::Type_Stack::last = "last";
const cstring IR::Type_Stack::arraySize = "size";
const cstring IR::Type_Stack::lastIndex = "lastIndex";
const cstring IR::Type_Stack::push_front = "push_front";
const cstring IR::Type_Stack::pop_front = "pop_front";
const cstring IR::Type_Header::isValid = "isValid";
const cstring IR::Type_Header::setValid = "setValid";
const cstring IR::Type_Header::setInvalid = "setInvalid";

const IR::ID IR::Type_Table::hit = ID("hit");
const IR::ID IR::Type_Table::action_run = ID("action_run");

const cstring IR::Annotation::nameAnnotation = "name";
const cstring IR::Annotation::tableOnlyAnnotation = "tableonly";
const cstring IR::Annotation::defaultOnlyAnnotation = "defaultonly";
const cstring IR::Annotation::atomicAnnotation = "atomic";
const cstring IR::Annotation::hiddenAnnotation = "hidden";
const cstring IR::Annotation::lengthAnnotation = "length";
const cstring IR::Annotation::optionalAnnotation = "optional";
const cstring IR::Annotation::pkginfoAnnotation = "pkginfo";
const cstring IR::Annotation::deprecatedAnnotation = "deprecated";
const cstring IR::Annotation::synchronousAnnotation = "synchronous";

int Type_Declaration::nextId = 0;
int Type_InfInt::nextId = 0;

Annotations* Annotations::empty = new Annotations(Vector<Annotation>());

const Type_Bits* Type_Bits::get(int width, bool isSigned) {
    // map (width, signed) to type
    using bit_type_key = std::pair<int, bool>;
    static std::map<bit_type_key, const IR::Type_Bits*> *type_map = nullptr;
    if (type_map == nullptr)
        type_map = new std::map<bit_type_key, const IR::Type_Bits*>();
    auto &result = (*type_map)[std::make_pair(width, isSigned)];
    if (!result)
        result = new Type_Bits(width, isSigned);
    return result;
}

const Type::Unknown *Type::Unknown::get() {
    static const Type::Unknown *singleton = nullptr;
    if (!singleton)
        singleton = (new Type::Unknown());
    return singleton;
}

const Type::Boolean *Type::Boolean::get() {
    static const Type::Boolean *singleton = nullptr;
    if (!singleton)
        singleton = (new Type::Boolean());
    return singleton;
}

const Type_String *Type_String::get() {
    static const Type_String *singleton = nullptr;
    if (!singleton)
        singleton = (new Type_String());
    return singleton;
}

const Type::Bits *Type::Bits::get(Util::SourceInfo si, int sz, bool isSigned) {
    if (sz <= 0)
        ::error("%1%: Width cannot be negative or zero", si);
    return get(sz, isSigned);
}

const Type::Varbits *Type::Varbits::get(Util::SourceInfo si, int sz) {
    if (sz <= 0)
        ::error("%1%: Width cannot be negative or zero", si);
    return new Type::Varbits(si, sz);
}

const Type::Varbits *Type::Varbits::get() {
    return new Type::Varbits(0);
}

const Type_Dontcare *Type_Dontcare::get() {
    static const Type_Dontcare *singleton;
    if (!singleton)
        singleton = (new Type_Dontcare());
    return singleton;
}

const Type_State *Type_State::get() {
    static const Type_State *singleton;
    if (!singleton)
        singleton = (new Type_State());
    return singleton;
}

const Type_Void *Type_Void::get() {
    static const Type_Void *singleton;
    if (!singleton)
        singleton = (new Type_Void());
    return singleton;
}

const Type_MatchKind *Type_MatchKind::get() {
    static const Type_MatchKind *singleton;
    if (!singleton)
        singleton = (new Type_MatchKind());
    return singleton;
}

bool Type_ActionEnum::contains(cstring name) const {
    for (auto a : actionList->actionList) {
        if (a->getName() == name)
            return true;
    }
    return false;
}

size_t Type_MethodBase::minParameterCount() const {
    size_t rv = 0;
    for (auto p : *parameters)
        if (!p->isOptional())
            ++rv;
    return rv;
}

const Type* Type_Tuple::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_Tuple(srcInfo, *args);
}

const Type* Type_Specialized::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    return new IR::Type_Specialized(srcInfo, baseType, args);
}

const Type* Type_SpecializedCanonical::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    return new IR::Type_Specialized(srcInfo, baseType->getP4Type()->to<IR::Type_Name>(), args);
}

}  // namespace IR
