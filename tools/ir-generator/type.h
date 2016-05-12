#ifndef _TOOLS_IR_GENERATOR_TYPE_H_
#define _TOOLS_IR_GENERATOR_TYPE_H_

#include <vector>
#include "lib/cstring.h"
#include "lib/source_file.h"

class IrClass;
class IrNamespace;

#define ALL_TYPES(M) M(NamedType) M(TemplateInstantiation)
#define FORWARD_DECLARE(T) class T;
ALL_TYPES(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class Type : public Util::IHasSourceInfo {
    Util::SourceInfo            srcInfo;
 public:
    explicit Type(Util::SourceInfo si) : srcInfo(si) {}
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    virtual IrClass *resolve(const IrNamespace *) const = 0;
    virtual bool isResolved() const = 0;
    virtual bool templateArgResolved() const = 0;
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
    const LookupScope   *lookup;
    cstring             name;
    mutable IrClass     *resolved = nullptr;
 public:
    NamedType(Util::SourceInfo si, const LookupScope *l, cstring n)
    : Type(si), lookup(l), name(n) {}

    cstring toString() const override { return lookup ? lookup->toString() + name : name; }
    IrClass *resolve(const IrNamespace *) const override;
    bool isResolved() const override { return resolved != nullptr; }
    bool templateArgResolved() const override { return resolved != nullptr; }
    bool operator==(const Type &t) const override { return t == *this; }
    bool operator==(const NamedType &t) const override {
        if (resolved && resolved == t.resolved) return true;
        if (name != t.name) return false;
        return (lookup == t.lookup || (lookup && t.lookup && *lookup == *t.lookup)); }
};

class TemplateInstantiation : public Type {
    const Type                  *base;
 public:
    std::vector<const Type *>   args;
    TemplateInstantiation(Util::SourceInfo si, Type *b, const std::vector<const Type *> &a)
    : Type(si), base(b), args(a) {}
    TemplateInstantiation(Util::SourceInfo si, Type *b, Type *a)
    : Type(si), base(b) { args.push_back(a); }
    bool isResolved() const override { return base->isResolved(); }
    bool templateArgResolved() const override {
        for (auto arg : args) if (arg->templateArgResolved()) return true;
        return base->templateArgResolved(); }
    IrClass *resolve(const IrNamespace *ns) const override {
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


#endif /* _TOOLS_IR_GENERATOR_TYPE_H_ */
