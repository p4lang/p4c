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

#include "ir.h"

namespace IR {

const cstring IR::Type_Stack::next = "next";
const cstring IR::Type_Stack::last = "last";
const cstring IR::Type_Stack::full = "full";
const cstring IR::Type_Stack::empty = "empty";
const cstring IR::Type_Stack::push_front = "push_front";
const cstring IR::Type_Stack::pop_front = "pop_front";
const cstring IR::Type_Header::isValid = "isValid";
const cstring IR::Type_Header::setValid = "setValid";
const cstring IR::Type_Header::setInvalid = "setInvalid";

const IR::ID IR::Type_Table::hit = ID("hit");
const IR::ID IR::Type_Table::action_run = ID("action_run");

const cstring IR::Annotation::nameAnnotation = "name";

std::map<int, const IR::Type_Bits*> *Type_Bits::signedTypes = nullptr;
std::map<int, const IR::Type_Bits*> *Type_Bits::unsignedTypes = nullptr;

int Type_Declaration::nextId = 0;
int Type_InfInt::nextId = 0;

Annotations* Annotations::empty = new Annotations(new Vector<Annotation>());

const Type_Bits* Type_Bits::get(int width, bool isSigned) {
    std::map<int, const IR::Type_Bits*> *map;
    if (isSigned) {
        if (signedTypes == nullptr)
            signedTypes = new std::map<int, const IR::Type_Bits*>();
        map = signedTypes;
    } else {
        if (unsignedTypes == nullptr)
            unsignedTypes = new std::map<int, const IR::Type_Bits*>();
        map = unsignedTypes;
    }

    auto it = map->find(width);
    if (it != map->end())
        return it->second;
    auto result = IR::Type_Bits::get(Util::SourceInfo(), width, isSigned);
    map->emplace(width, result);
    return result;
}

const Type::Unknown *Type::Unknown::get() {
    static const Type::Unknown *singleton = nullptr;
    if (!singleton)
        singleton = (new Type::Unknown(Util::SourceInfo()));
    return singleton;
}

const Type::Boolean *Type::Boolean::get() {
    static const Type::Boolean *singleton = nullptr;
    if (!singleton)
        singleton = (new Type::Boolean(Util::SourceInfo()));
    return singleton;
}

const Type_String *Type_String::get() {
    static const Type_String *singleton = nullptr;
    if (!singleton)
        singleton = (new Type_String(Util::SourceInfo()));
    return singleton;
}

const Type::Bits *Type::Bits::get(Util::SourceInfo si, int sz, bool isSigned) {
    if (sz <= 0)
        ::error("%1%: Width cannot be negative or zero", si);
    return new Type::Bits(si, sz, isSigned);
}

const Type::Varbits *Type::Varbits::get(Util::SourceInfo si, int sz) {
    if (sz <= 0)
        ::error("%1%: Width cannot be negative or zero", si);
    return new Type::Varbits(si, sz);
}

const Type::Varbits *Type::Varbits::get() {
    return new Type::Varbits(Util::SourceInfo(), 0);
}

const Type_Dontcare *Type_Dontcare::get() {
    static const Type_Dontcare *singleton;
    if (!singleton)
        singleton = (new Type_Dontcare(Util::SourceInfo()));
    return singleton;
}

const Type_State *Type_State::get() {
    static const Type_State *singleton;
    if (!singleton)
        singleton = (new Type_State(Util::SourceInfo()));
    return singleton;
}

const Type_Void *Type_Void::get() {
    static const Type_Void *singleton;
    if (!singleton)
        singleton = (new Type_Void(Util::SourceInfo()));
    return singleton;
}

const Type_MatchKind *Type_MatchKind::get() {
    static const Type_MatchKind *singleton;
    if (!singleton)
        singleton = (new Type_MatchKind(Util::SourceInfo()));
    return singleton;
}

bool Type_ActionEnum::contains(cstring name) const {
    for (auto a : *actionList->actionList) {
        if (a->getName() == name)
            return true;
    }
    return false;
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
