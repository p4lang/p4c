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

#ifndef _TOOLS_IR_GENERATOR_TYPE_H_
#define _TOOLS_IR_GENERATOR_TYPE_H_

#include <vector>
#include "lib/cstring.h"
#include "lib/source_file.h"

class IrClass;
class IrNamespace;

#define ALL_TYPES(M) M(NamedType) M(TemplateInstantiation) M(ReferenceType) \
                     M(PointerType) M(ArrayType)
#define FORWARD_DECLARE(T) class T;
ALL_TYPES(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class Type : public Util::IHasSourceInfo {
    Util::SourceInfo            srcInfo;
 public:
    Type() = default;
    explicit Type(Util::SourceInfo si) : srcInfo(si) {}
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    virtual const IrClass *resolve(const IrNamespace *) const = 0;
    virtual bool isResolved() const = 0;
    virtual cstring declSuffix() const { return ""; }
    virtual bool operator==(const Type &) const = 0;
    bool operator!=(const Type &t) const { return !operator==(t); }
#define OP_EQUALS(T) virtual bool operator==(const T &) const { return false; }
ALL_TYPES(OP_EQUALS)
#undef OP_EQUALS
};

struct LookupScope : public Util::IHasSourceInfo {
    Util::SourceInfo    srcInfo;
    const LookupScope   *in;
    bool                global;
    cstring             name;
    LookupScope() : in(nullptr), global(true), name(nullptr) {}
    LookupScope(const LookupScope *in, cstring name) : in(in), global(false), name(name) {}
    explicit LookupScope(cstring name) : in(nullptr), global(false), name(name) {}
    explicit LookupScope(const IrNamespace *);

    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    cstring toString() const override {
        if (global) return "IR::";
        return (in ? in->toString() + name : name) + "::"; }
    IrNamespace *resolve(const IrNamespace *) const;
    bool operator==(const LookupScope &l) const {
        if (name != l.name || global != l.global) return false;
        return (in == l.in || (in && l.in && *in == *l.in)); }
};


class NamedType : public Type {
    const LookupScope     *lookup;
    cstring               name;
    mutable const IrClass *resolved = nullptr;

 public:
    NamedType(Util::SourceInfo si, const LookupScope *l, cstring n)
    : Type(si), lookup(l), name(n) {}
    NamedType(const LookupScope *l, cstring n) : lookup(l), name(n) {}
    explicit NamedType(cstring n) : lookup(nullptr), name(n) {}
    explicit NamedType(const IrClass *);

    cstring toString() const override;
    const IrClass *resolve(const IrNamespace *) const override;
    bool isResolved() const override { return resolved != nullptr; }
    bool operator==(const Type &t) const override { return t == *this; }
    bool operator==(const NamedType &t) const override {
        if (resolved && resolved == t.resolved) return true;
        if (name != t.name) return false;
        return (lookup == t.lookup || (lookup && t.lookup && *lookup == *t.lookup)); }

    static NamedType& Bool();
    static NamedType& Int();
    static NamedType& Void();
    static NamedType& Cstring();
    static NamedType& Ostream();
    static NamedType& Visitor();
    static NamedType& Unordered_Set();
    static NamedType& JSONGenerator();
    static NamedType& JSONLoader();
    static NamedType& JSONObject();
    static NamedType& SourceInfo();
};

class TemplateInstantiation : public Type {
 public:
    const Type                  *base;

    std::vector<const Type *>   args;
    TemplateInstantiation(Util::SourceInfo si, Type *b, const std::vector<const Type *> &a)
    : Type(si), base(b), args(a) {}
    TemplateInstantiation(Util::SourceInfo si, Type *b, Type *a)
    : Type(si), base(b) { args.push_back(a); }
    TemplateInstantiation(const Type *b, const Type *a) : base(b) { args.push_back(a); }\
    bool isResolved() const override { return base->isResolved(); }
    const IrClass *resolve(const IrNamespace *ns) const override {
        for (auto arg : args) arg->resolve(ns);
        return base->resolve(ns); }
    cstring toString() const override;
    bool operator==(const Type &t) const override { return t == *this; }
    bool operator==(const TemplateInstantiation &t) const override {
        if (args.size() != t.args.size() || *base != *t.base) return false;
        for (size_t i = 0; i < args.size(); i++)
            if (*args[i] != *t.args[i]) return false;
        return true; }
};

class ReferenceType : public Type {
    const Type          *base;
    bool                isConst;
 public:
    explicit ReferenceType(const Type *t, bool c = false) : base(t), isConst(c) {}
    ReferenceType(Util::SourceInfo si, const Type *t, bool c = false)
    : Type(si), base(t), isConst(c) {}
    bool isResolved() const override { return false; }
    const IrClass *resolve(const IrNamespace *ns) const override {
        base->resolve(ns);
        return nullptr; }
    cstring toString() const override;
    bool operator==(const Type &t) const override { return t == *this; }
    bool operator==(const ReferenceType &t) const override {
        return isConst == t.isConst && *base == *t.base; }
    static ReferenceType OstreamRef, VisitorRef;
};

class PointerType : public Type {
    const Type          *base;
    bool                isConst;
 public:
    explicit PointerType(const Type *t, bool c = false) : base(t), isConst(c) {}
    PointerType(Util::SourceInfo si, const Type *t, bool c = false)
    : Type(si), base(t), isConst(c) {}
    bool isResolved() const override { return false; }
    const IrClass *resolve(const IrNamespace *ns) const override {
        base->resolve(ns);
        return nullptr; }
    cstring toString() const override;
    bool operator==(const Type &t) const override { return t == *this; }
    bool operator==(const PointerType &t) const override {
        return isConst == t.isConst && *base == *t.base; }
};

class ArrayType : public Type {
 public:
    const Type          *base;
    int                 size;
    ArrayType(const Type *t, int s) : base(t), size(s) {}
    ArrayType(Util::SourceInfo si, const Type *t, int s) : Type(si), base(t), size(s) {}
    bool isResolved() const override { return false; }
    const IrClass *resolve(const IrNamespace *ns) const override {
        base->resolve(ns);
        return nullptr; }
    cstring toString() const override { return base->toString(); }
    cstring declSuffix() const override;
    bool operator==(const Type &t) const override { return t == *this; }
    bool operator==(const ArrayType &t) const override {
        return size == t.size && *base == *t.base; }
};

#endif /* _TOOLS_IR_GENERATOR_TYPE_H_ */
