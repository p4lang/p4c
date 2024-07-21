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

#include "type.h"

#include "irclass.h"

LookupScope::LookupScope(const IrNamespace *ns)
    : in((ns && ns->name) ? new LookupScope(ns->parent) : nullptr),
      global(!ns || !ns->name),
      name(ns ? ns->name : nullptr) {}

IrNamespace *LookupScope::resolve(const IrNamespace *in) const {
    if (global) return &IrNamespace::global();
    if (!in) in = &IrNamespace::global();
    if (this->in) {
        in = this->in->resolve(in);
        return in->lookupChild(name);
    }
    while (in) {
        if (auto *found = in->lookupChild(name)) return found;
        in = in->parent;
    }
    if (name == "IR") return &IrNamespace::global();
    return nullptr;
}

NamedType::NamedType(const IrClass *cl)
    : lookup(new LookupScope(cl->containedIn)), name(cl->name), resolved(cl) {}

const IrClass *NamedType::resolve(const IrNamespace *in) const {
    if (resolved) return resolved;
    if (!in) in = LookupScope().resolve(in);
    if (lookup) {
        in = lookup->resolve(in);
        if (!in) return nullptr;
        if (auto *found = in->lookupClass(name)) {
            foundin = in;
            return (resolved = found);
        }
        if (in->lookupOther(name)) {
            foundin = in;
            return nullptr;
        }
    }
    while (in) {
        if (auto *found = in->lookupClass(name)) {
            foundin = in;
            return (resolved = found);
        }
        if (in->lookupOther(name)) {
            foundin = in;
            return nullptr;
        }
        in = in->parent;
    }
    return nullptr;
}

NamedType &NamedType::Bool() {
    static NamedType nt("bool"_cs);
    return nt;
}

NamedType &NamedType::Int() {
    static NamedType nt("int"_cs);
    return nt;
}

NamedType &NamedType::Void() {
    static NamedType nt("void"_cs);
    return nt;
}

NamedType &NamedType::Cstring() {
    static NamedType nt("cstring"_cs);
    return nt;
}

NamedType &NamedType::Ostream() {
    static NamedType nt(new LookupScope("std"_cs), "ostream"_cs);
    return nt;
}

NamedType &NamedType::Visitor() {
    static NamedType nt("Visitor"_cs);
    return nt;
}

NamedType &NamedType::Unordered_Set() {
    static NamedType nt(new LookupScope("std"_cs), "unordered_set"_cs);
    return nt;
}

NamedType &NamedType::JSONGenerator() {
    static NamedType nt("JSONGenerator"_cs);
    return nt;
}

NamedType &NamedType::JSONLoader() {
    static NamedType nt("JSONLoader"_cs);
    return nt;
}

NamedType &NamedType::JSONObject() {
    static NamedType nt("JSONObject"_cs);
    return nt;
}

NamedType &NamedType::SourceInfo() {
    static NamedType nt(new LookupScope("Util"_cs), "SourceInfo"_cs);
    return nt;
}

cstring NamedType::toString() const {
    if (resolved) return resolved->fullName();
    if (!lookup && name == "ID") return "IR::ID"_cs;  // hack -- ID is in namespace p4c::IR
    if (lookup) return lookup->toString() + name;
    if (foundin) return LookupScope(foundin).toString() + name;
    return name;
}

cstring TemplateInstantiation::toString() const {
    std::string rv = base->toString().c_str();
    rv += '<';
    const char *sep = "";
    for (const auto *arg : args) {
        rv += sep;
        if (arg->isResolved() && !base->isResolved()) rv += "const ";
        rv += arg->toString().c_str();
        if (arg->isResolved() && !base->isResolved()) rv += " *";
        sep = ", ";
    }
    rv += '>';
    return rv;
}

cstring ReferenceType::toString() const {
    std::string rv = base->toString().c_str();
    if (isConst) rv += " const";
    rv += " &";
    return rv;
}

ReferenceType ReferenceType::OstreamRef(&NamedType::Ostream()), ReferenceType::VisitorRef(
                                                                    &NamedType::Visitor());

cstring PointerType::toString() const {
    std::string rv = base->toString().c_str();
    if (isConst) rv += " const";
    rv += " *";
    return rv;
}

cstring ArrayType::declSuffix() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "[%d]", size);
    return cstring(buf);
}

const IrClass *FunctionType::resolve(const IrNamespace *ns) const {
    ret->resolve(ns);
    for (auto arg : args) arg->resolve(ns);
    return nullptr;
}

cstring FunctionType::toString() const {
    std::string result = ret->toString().c_str();
    result += "(";
    const char *sep = "";
    for (const auto *arg : args) {
        result += sep;
        if (arg->isResolved()) result += "const ";
        result += arg->toString().c_str();
        if (arg->isResolved()) result += "*";
        sep = ", ";
    }
    result += ")";
    return result;
}
