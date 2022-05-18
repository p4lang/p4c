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
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

namespace P4::IR {

IR::Ptr<Type_Bits> Type_Bits::get(int width, bool isSigned) {
    // map (width, signed) to type
    using bit_type_key = std::pair<int, bool>;
    static std::map<bit_type_key, IR::Ptr<Type_Bits>> *type_map = nullptr;
    if (type_map == nullptr) type_map = new std::map<bit_type_key, IR::Ptr<Type_Bits>>();
    auto &result = (*type_map)[std::make_pair(width, isSigned)];
    if (!result) result = new Type_Bits(width, isSigned);
    if (width > P4CContext::getConfig().maximumWidthSupported())
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "%1%: Compiler only supports widths up to %2%",
                    result, P4CContext::getConfig().maximumWidthSupported());
    return result;
}

IR::Ptr<Type_Bits> Type_Bits::get(const Util::SourceInfo &si, int sz, bool isSigned) {
    if (sz < 0) {
        ::P4::error(ErrorType::ERR_INVALID, "%1%Width %2% of type cannot be negative", si, sz);
        // Return a value that will not cause crashes later on
        return new IR::Type_Bits(si, 1024, isSigned);
    }
    if (sz == 0 && isSigned) {
        ::P4::error(ErrorType::ERR_INVALID, "%1%Width of signed type cannot be zero", si);
        // Return a value that will not cause crashes later on
        return new IR::Type_Bits(si, 1024, isSigned);
    }
    return new IR::Type_Bits(si, sz, isSigned);
}

IR::Ptr<Type_Bits> Type_Bits::get(const Util::SourceInfo &si, const IR::Expression *expression,
                                  bool isSigned) {
    if (auto *k = expression->to<IR::Constant>()) {
        if (!k->fitsInt())
            error(ErrorType::ERR_OVERLIMIT,
                  "%1$x: this implementation does not support bitstrings this large", k);
        else
            return get(si, k->asInt(), isSigned);
    }
    return new IR::Type_Bits(si, expression, isSigned);
}

IR::Ptr<Type_Unknown> Type_Unknown::get() {
    static IR::Ptr<Type_Unknown> singleton = nullptr;
    if (!singleton) singleton = (new Type_Unknown());
    return singleton;
}

IR::Ptr<Type_Unknown> Type_Unknown::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Unknown(si);
}

IR::Ptr<Type_Boolean> Type_Boolean::get() {
    static IR::Ptr<Type_Boolean> singleton = nullptr;
    if (!singleton) singleton = (new Type_Boolean());
    return singleton;
}

IR::Ptr<Type_Boolean> Type_Boolean::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Boolean(si);
}

IR::Ptr<Type_String> Type_String::get() {
    static IR::Ptr<Type_String> singleton = nullptr;
    if (!singleton) singleton = (new Type_String());
    return singleton;
}

IR::Ptr<Type_String> Type_String::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_String(si);
}

IR::Ptr<Type_Varbits> Type_Varbits::get(const Util::SourceInfo &si, const IR::Expression *expr) {
    if (auto *k = expr->to<IR::Constant>()) {
        if (!k->fitsInt())
            error(ErrorType::ERR_OVERLIMIT,
                  "%1$x: this implementation does not support bitstrings this large", k);
        else
            return get(si, k->asInt());
    }
    return new Type_Varbits(si, expr);
}

IR::Ptr<Type_Varbits> Type_Varbits::get(const Util::SourceInfo &si, int sz) {
    auto result = new Type_Varbits(si, sz);
    if (sz < 0) {
        ::P4::error(ErrorType::ERR_INVALID, "%1%: Width cannot be negative or zero", result);
        // Return a value that will not cause crashes later on
        return new IR::Type_Varbits(si, 1024);
    }
    return result;
}

IR::Ptr<Type_Varbits> Type_Varbits::get(int sz) { return new Type_Varbits(sz); }

IR::Ptr<Type_Varbits> Type_Varbits::get() { return new Type_Varbits(0); }

IR::Ptr<Type_InfInt> Type_InfInt::get() {
    // We do not cache types with declaration IDs (yet).
    return new Type_InfInt();
}

IR::Ptr<Type_InfInt> Type_InfInt::get(const Util::SourceInfo &si) {
    // We do not cache types with source info and declaration IDs (yet).
    return new Type_InfInt(si);
}

IR::Ptr<Type_Dontcare> Type_Dontcare::get() {
    static IR::Ptr<Type_Dontcare> singleton;
    if (!singleton) singleton = (new Type_Dontcare());
    return singleton;
}

IR::Ptr<Type_Dontcare> Type_Dontcare::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Dontcare(si);
}

IR::Ptr<Type_State> Type_State::get() {
    static IR::Ptr<Type_State> singleton;
    if (!singleton) singleton = (new Type_State());
    return singleton;
}

IR::Ptr<Type_State> Type_State::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_State(si);
}

IR::Ptr<Type_Void> Type_Void::get() {
    static IR::Ptr<Type_Void> singleton = nullptr;
    if (!singleton) singleton = (new Type_Void());
    return singleton;
}

IR::Ptr<Type_Void> Type_Void::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_Void(si);
}

IR::Ptr<Type_MatchKind> Type_MatchKind::get() {
    static IR::Ptr<Type_MatchKind> singleton = nullptr;
    if (!singleton) singleton = (new Type_MatchKind());
    return singleton;
}

IR::Ptr<Type_MatchKind> Type_MatchKind::get(const Util::SourceInfo &si) {
    // We do not cache types with source info (yet).
    return new Type_MatchKind(si);
}

IR::Ptr<Type_Any> Type_Any::get() {
    // We do not cache types with declaration IDs (yet).
    return new Type_Any();
}

IR::Ptr<Type_Any> Type_Any::get(const Util::SourceInfo &si) {
    // We do not cache types with source info and declaration IDs (yet).
    return new Type_Any(si);
}

bool Type_ActionEnum::contains(cstring name) const {
    for (auto a : actionList->actionList) {
        if (a->getName() == name) return true;
    }
    return false;
}

IR::Ptr<Type> Type_List::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_List(srcInfo, *args);
}

IR::Ptr<Type> Type_Tuple::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_Tuple(srcInfo, *args);
}

IR::Ptr<Type> Type_P4List::getP4Type() const {
    return new IR::Type_P4List(srcInfo, elementType->getP4Type());
}

IR::Ptr<Type> Type_Specialized::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    return new IR::Type_Specialized(srcInfo, baseType, args);
}

IR::Ptr<Type> Type_SpecializedCanonical::getP4Type() const {
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

}  // namespace P4::IR
