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

#include <cstddef>
#include <map>
#include <utility>

#include "frontends/common/parser_options.h"
#include "ir/configuration.h"
#include "ir/id.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

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
const cstring IR::Type::minSizeInBits = "minSizeInBits";
const cstring IR::Type::minSizeInBytes = "minSizeInBytes";
const cstring IR::Type::maxSizeInBits = "maxSizeInBits";
const cstring IR::Type::maxSizeInBytes = "maxSizeInBytes";

const IR::ID IR::Type_Table::hit = ID("hit");
const IR::ID IR::Type_Table::miss = ID("miss");
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
const cstring IR::Annotation::pureAnnotation = "pure";
const cstring IR::Annotation::noSideEffectsAnnotation = "noSideEffects";
const cstring IR::Annotation::noWarnAnnotation = "noWarn";
const cstring IR::Annotation::matchAnnotation = "match";
const cstring IR::Annotation::fieldListAnnotation = "field_list";
const cstring IR::Annotation::debugLoggingAnnotation = "__debug";

long Type_Declaration::nextId = 0;
long Type_InfInt::nextId = 0;
long Type_Any::nextId = 0;

Annotations *Annotations::empty = new Annotations(Vector<Annotation>());

const Type *Type_Stack::at(size_t) const { return elementType; }

const Type_Bits *Type_Bits::get(int width, bool isSigned) {
    // map (width, signed) to type
    using bit_type_key = std::pair<int, bool>;
    static std::map<bit_type_key, const IR::Type_Bits *> *type_map = nullptr;
    if (type_map == nullptr) type_map = new std::map<bit_type_key, const IR::Type_Bits *>();
    auto &result = (*type_map)[std::make_pair(width, isSigned)];
    if (!result) result = new Type_Bits(width, isSigned);
    if (width > P4CContext::getConfig().maximumWidthSupported())
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: Compiler only supports widths up to %2%", result,
                P4CContext::getConfig().maximumWidthSupported());
    return result;
}

const Type_Bits *Type_Bits::get(const Util::SourceInfo &si, int sz, bool isSigned) {
    auto *result = new IR::Type_Bits(si, sz, isSigned);
    if (sz < 0) {
        ::error(ErrorType::ERR_INVALID, "%1%: Width of type cannot be negative", result);
        // Return a value that will not cause crashes later on
        return new IR::Type_Bits(si, 1024, isSigned);
    }
    if (sz == 0 && isSigned) {
        ::error(ErrorType::ERR_INVALID, "%1%: Width of signed type cannot be zero", result);
        // Return a value that will not cause crashes later on
        return new IR::Type_Bits(si, 1024, isSigned);
    }
    return result;
}

const Type_Bits *Type_Bits::get(const Util::SourceInfo &si, const IR::Expression *expression,
                                bool isSigned) {
    return new IR::Type_Bits(si, expression, isSigned);
}

const Type_Unknown *Type_Unknown::get() {
    static const Type_Unknown *singleton = nullptr;
    if (!singleton) singleton = (new Type_Unknown());
    return singleton;
}

const Type_Unknown *Type_Unknown::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Unknown(si);
}

const Type_Boolean *Type_Boolean::get() {
    static const Type_Boolean *singleton = nullptr;
    if (!singleton) singleton = (new Type_Boolean());
    return singleton;
}

const Type_Boolean *Type_Boolean::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Boolean(si);
}

const Type_String *Type_String::get() {
    static const Type_String *singleton = nullptr;
    if (!singleton) singleton = (new Type_String());
    return singleton;
}

const Type_String *Type_String::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_String(si);
}

const Type_Varbits *Type_Varbits::get(const Util::SourceInfo &si, const IR::Expression *expr) {
    return new Type_Varbits(si, expr);
}

const Type_Varbits *Type_Varbits::get(const Util::SourceInfo &si, int sz) {
    auto result = new Type_Varbits(si, sz);
    if (sz < 0) {
        ::error(ErrorType::ERR_INVALID, "%1%: Width cannot be negative or zero", result);
        // Return a value that will not cause crashes later on
        return new IR::Type_Varbits(si, 1024);
    }
    return result;
}

const Type_Varbits *Type_Varbits::get(int sz) { return new Type_Varbits(sz); }

const Type_Varbits *Type_Varbits::get() { return new Type_Varbits(0); }

const Type_InfInt *Type_InfInt::get() {
    // We do not cache types with declaration IDs (yet).
    return new Type_InfInt();
}

const Type_InfInt *Type_InfInt::get(const Util::SourceInfo &si) {
    // We do not cache types with source info and declaration IDs (yet).
    return new Type_InfInt(si);
}

const Type_Dontcare *Type_Dontcare::get() {
    static const Type_Dontcare *singleton;
    if (!singleton) singleton = (new Type_Dontcare());
    return singleton;
}

const Type_Dontcare *Type_Dontcare::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Dontcare(si);
}

const Type_State *Type_State::get() {
    static const Type_State *singleton;
    if (!singleton) singleton = (new Type_State());
    return singleton;
}

const Type_State *Type_State::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_State(si);
}

const Type_Void *Type_Void::get() {
    static const Type_Void *singleton;
    if (!singleton) singleton = (new Type_Void());
    return singleton;
}

const Type_Void *Type_Void::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Void(si);
}

const Type_MatchKind *Type_MatchKind::get() {
    static const Type_MatchKind *singleton;
    if (!singleton) singleton = (new Type_MatchKind());
    return singleton;
}

const Type_MatchKind *Type_MatchKind::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_MatchKind(si);
}

const Type_Any *Type_Any::get() {
    // We do not cache types with declaration IDs (yet).
    return new Type_Any();
}

const Type_Any *Type_Any::get(const Util::SourceInfo &si) {
    // We do not cache types with source info and declaration IDs (yet).
    return new Type_Any(si);
}

bool Type_ActionEnum::contains(cstring name) const {
    for (auto a : actionList->actionList) {
        if (a->getName() == name) return true;
    }
    return false;
}

size_t Type_MethodBase::minParameterCount() const {
    size_t rv = 0;
    for (auto p : *parameters)
        if (!p->isOptional()) ++rv;
    return rv;
}

const Type *Type_List::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_List(srcInfo, *args);
}

const Type *Type_Tuple::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_Tuple(srcInfo, *args);
}

const Type *Type_P4List::getP4Type() const {
    return new IR::Type_P4List(srcInfo, elementType->getP4Type());
}

const Type *Type_Specialized::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    return new IR::Type_Specialized(srcInfo, baseType, args);
}

const Type *Type_SpecializedCanonical::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    auto bt = baseType->getP4Type();
    if (auto tn = bt->to<IR::Type_Name>()) return new IR::Type_Specialized(srcInfo, tn, args);
    auto st = baseType->to<IR::Type_StructLike>();
    BUG_CHECK(st != nullptr, "%1%: expected a struct", baseType);
    return new IR::Type_Specialized(srcInfo, new IR::Type_Name(st->getName()), args);
}

}  // namespace IR
