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

#ifndef _TOOLS_IR_GENERATOR_IRCLASS_H_
#define _TOOLS_IR_GENERATOR_IRCLASS_H_

#include <vector>
#include <map>
#include <stdexcept>

#include "lib/cstring.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"
#include "lib/map.h"
#include "lib/ordered_map.h"
#include "lib/source_file.h"
#include "type.h"

class IrClass;
class IrField;

class IrNamespace {
    std::map<cstring, IrClass *>        classes;
    std::map<cstring, IrNamespace *>    children;
    static IrNamespace& global();
    IrNamespace(IrNamespace *p, cstring n) : parent(p), name(n) {
        if (p) p->children[name] = this; }
    IrNamespace() = delete;
    friend class IrClass;
    friend struct LookupScope;

 public:
    IrNamespace                         *parent;
    cstring                             name;
    static IrNamespace *get(IrNamespace *, cstring);
    static void add_class(IrClass *);
    IrNamespace *lookupChild(cstring name) const { return ::get(children, name); }
    IrClass *lookupClass(cstring name) const { return ::get(classes, name); }
};

std::ostream &operator<<(std::ostream &, IrNamespace *);

class IrElement  : public Util::IHasSourceInfo {
 public:
    Util::SourceInfo srcInfo;
    const IrClass *clss = nullptr;

    IrElement() = default;
    explicit IrElement(Util::SourceInfo info) : srcInfo(info) {}
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    template<class T> T *to() { return dynamic_cast<T *>(this); }
    template<class T> const T *to() const { return dynamic_cast<const T *>(this); }
    template<class T> bool is() const { return to<T>() != nullptr; }
    virtual void resolve() {}
    virtual void generate_hdr(std::ostream &out) const = 0;
    virtual void generate_impl(std::ostream &out) const = 0;

    enum access_t { Public, Protected, Private } access = Public;
    enum modifier_t { NullOK = 1, Optional = 2, Inline = 4, Virtual = 8, Static = 16, Const = 32 };
    static inline const char *modifier(int m) {
        if (m & IrElement::NullOK) return "NullOK";
        if (m & IrElement::Optional) return "optional";
        if (m & IrElement::Virtual) return "virtual";
        if (m & IrElement::Static) return "static";
        if (m & IrElement::Inline) return "inline";
        if (m & IrElement::Const) return "const";
        return ""; }
};
inline std::ostream &operator<<(std::ostream &out, IrElement::access_t a) {
    switch (a) {
    case IrElement::Public: out << " public:"; break;
    case IrElement::Protected: out << " protected:"; break;
    case IrElement::Private: out << " private:"; break; }
    return out << std::endl; }

class IrMethod : public IrElement {
 public:
    cstring                             name;
    const Type                          *rtype = nullptr;
    std::vector<const IrField *>        args;
    cstring                             body;
    bool inImpl = false, isConst = false, isOverride = false, isStatic = false, isVirtual = false,
         isUser = false, isFriend = false;
    IrMethod(Util::SourceInfo info, cstring name, cstring body)
    : IrElement(info), name(name), body(body) {}
    IrMethod(Util::SourceInfo info, cstring name) : IrElement(info), name(name) {}
    IrMethod(cstring name, cstring body) : name(name), body(body) {}
    cstring toString() const override { return name; }

    void generate_proto(std::ostream &, bool, bool) const;
    void generate_hdr(std::ostream &) const override;
    void generate_impl(std::ostream &) const override;
    struct info_t {
        const Type                      *rtype;
        std::vector<const IrField *>    args;
        int                             flags;
        std::function<cstring(IrClass *, Util::SourceInfo, cstring)>       create;
    };
    static const ordered_map<cstring, info_t> Generate;
};

class IrField : public IrElement {
 public:
    const Type *type;
    const cstring name;
    const cstring initializer;
    const bool nullOK = false;
    const bool optional = false;
    const bool isInline = false;
    const bool isStatic = false;
    const bool isConst = false;

    static IrField* srcInfoField();

    IrField(Util::SourceInfo info, const Type *type, cstring name, cstring init, int flags = 0)
    : IrElement(info), type(type), name(name), initializer(init), nullOK(flags & NullOK),
      optional(flags & Optional), isInline(flags & Inline), isStatic(flags & Static),
      isConst(flags & Const) {}
    IrField(const Type *type, cstring name, cstring init = cstring(), int flags = 0)
    : IrField(Util::SourceInfo(), type, name, init, flags) {}
    IrField(const Type *type, cstring name, int flags)
    : IrField(Util::SourceInfo(), type, name, cstring(), flags) {}
    void generate(std::ostream &out, bool asField) const;
    void generate_hdr(std::ostream &out) const override { generate(out, true); }
    void generate_impl(std::ostream &) const override;
    cstring toString() const override { return name; }
};

class ConstFieldInitializer : public IrElement {
    cstring name;
    cstring initializer;
 public:
    ConstFieldInitializer(Util::SourceInfo info, cstring name, cstring initializer) :
            IrElement(info), name(name), initializer(initializer) {}
    cstring getName() const { return name; }
    cstring toString() const override { return name; }
    void generate_hdr(std::ostream &out) const override;
    void generate_impl(std::ostream &) const override {}
};

class IrNo : public IrElement {
 public:
    cstring     text;
    IrNo(Util::SourceInfo info, cstring text) : IrElement(info), text(text) {}
    void generate_hdr(std::ostream &) const override {}
    void generate_impl(std::ostream &) const override {}
    cstring toString() const override { return "#no" + text; }
};

class IrApply : public IrElement {
 public:
    explicit IrApply(Util::SourceInfo info) : IrElement(info) {}
    void generate_hdr(std::ostream &out) const override;
    void generate_impl(std::ostream &out) const override;
    cstring toString() const override { return "#apply"; }
};

enum class NodeKind {
    Interface,
    Abstract,
    Concrete,
    Template,
    Nested,
};

class EmitBlock : public IrElement {
    bool        impl;
    cstring     body;
 public:
    EmitBlock(Util::SourceInfo si, bool impl, cstring body)
    : IrElement(si), impl(impl), body(body) {}
    cstring toString() const override { return body; }
    void generate_hdr(std::ostream &out) const override;
    void generate_impl(std::ostream &out) const override;
};

class CommentBlock : public IrElement {
    cstring body;
 public:
    CommentBlock(Util::SourceInfo si, cstring body) : IrElement(si), body(body) {}
    cstring toString() const override {
        // print only Doxygen comments
        if (body.startsWith("/**") || body.startsWith("///"))
            return body;
        return "";
    }
    void append(cstring comment) { body += "\n" + comment; }
    void generate_hdr(std::ostream &out) const override { out << toString() << std::endl; };
    void generate_impl(std::ostream &out) const override { out << toString() << std::endl; };
};

// Represents a C++ class for an IR node.
class IrClass : public IrElement {
    std::vector<const IrClass *> parentClasses;
    std::vector<const Type *> parents;
    const IrClass *concreteParent;

    // each argument together with the class that has to receive it
    typedef std::vector<std::pair<const IrField*, const IrClass*>> ctor_args_t;
    void computeConstructorArguments(ctor_args_t &out) const;
    int generateConstructor(const ctor_args_t &args, const IrMethod *user, unsigned skip_opt);
    void generateMethods();
    bool shouldSkip(cstring feature) const;

 public:
    const IrClass *getParent() const {
        if (concreteParent == nullptr && this != nodeClass() && kind != NodeKind::Nested)
            return IrClass::nodeClass();
        return concreteParent; }

    std::vector<const CommentBlock *> comments;
    std::vector<IrElement *> elements;
    IrNamespace *containedIn, local;
    const NodeKind kind;
    const cstring name;
    mutable bool needVector = false;    // using a Vector of this class
    mutable bool needIndexedVector = false;  // using an IndexedVecor of this class
    mutable bool needNameMap = false;   // using a NameMap of this class
    mutable bool needNodeMap = false;   // using a NodeMap of this class
    access_t current_access = Public;   // used while parsing the class body

    static const char* indent;

    IrClass(Util::SourceInfo info, IrNamespace *ns, NodeKind kind, cstring name,
            const std::initializer_list<const Type *> &parents,
            const std::initializer_list<IrElement *> &elements)
    : IrElement(info), parents(parents), concreteParent(nullptr), elements(elements),
      containedIn(ns), local(containedIn, name), kind(kind), name(name) {
        IrNamespace::add_class(this); }
    IrClass(Util::SourceInfo info, IrNamespace *ns, NodeKind kind, cstring name,
            const std::vector<const Type *> *parents = nullptr,
            const std::vector<IrElement *> *elements = nullptr)
    : IrElement(info), concreteParent(nullptr), containedIn(ns),
      local(containedIn, name), kind(kind), name(name) {
        if (parents) this->parents = *parents;
        if (elements) this->elements = *elements;
        IrNamespace::add_class(this); }
    IrClass(NodeKind kind, cstring name) : IrClass(Util::SourceInfo(), nullptr, kind, name) {}
    IrClass(NodeKind kind, cstring name, const std::initializer_list<IrElement *> &elements)
    : concreteParent(nullptr), elements(elements), containedIn(&IrNamespace::global()),
      local(containedIn, name), kind(kind), name(name) {
        IrNamespace::add_class(this); }

    static IrClass* nodeClass();
    static IrClass* vectorClass();
    static IrClass* namemapClass();
    static IrClass* nodemapClass();
    static IrClass* ideclaration();
    static IrClass* indexedVectorClass();

    void declare(std::ostream &out) const;
    void generate_hdr(std::ostream &out) const override;
    void generate_impl(std::ostream &out) const override;
    void generateTreeMacro(std::ostream &out) const;
    void resolve() override;
    cstring toString() const override { return name; }
    std::string fullName() const;
    Util::Enumerator<IrField*>* getFields() const;
    Util::Enumerator<IrMethod*>* getUserMethods() const;
};

class IrDefinitions {
    std::vector<IrElement*> elements;
    Util::Enumerator<IrClass*>* getClasses() const;

 public:
    explicit IrDefinitions(std::vector<IrElement*> classes) : elements(classes) {}
    void resolve() {
        IrClass::nodeClass()->resolve();
        IrClass::vectorClass()->resolve();
        IrClass::namemapClass()->resolve();
        IrClass::nodemapClass()->resolve();
        IrClass::ideclaration()->resolve();
        IrClass::indexedVectorClass()->resolve();
        for (auto cls : *getClasses())
            cls->resolve(); }
    void generate(std::ostream &t, std::ostream &out, std::ostream &impl) const;
};

class LineDirective {
    Util::SourceInfo    loc;
    int                 delta = 0;
    bool                reset = false;
    bool                newline_before = false;
    friend std::ostream &operator<<(std::ostream &, const LineDirective &);
 public:
    static bool inhibit;
    explicit LineDirective(Util::SourceInfo l, bool nb = false) : loc(l), newline_before(nb) {}
    LineDirective(Util::SourceInfo l, int d, bool nb = false)
    : loc(l), delta(d), newline_before(nb) {}
    explicit LineDirective(bool nb = false) : reset(true), newline_before(nb) {}
};

inline std::ostream &operator<<(std::ostream &out, const LineDirective &l) {
    if (!LineDirective::inhibit) {
        if (l.newline_before) out << '\n';
        if (l.reset) {
            out << "#" << std::endl;   // will be fixed by tools/fixup-line-directives
        } else if (l.loc.isValid()) {
            auto pos = l.loc.toPosition();
            out << "#line " << (pos.sourceLine + l.delta + 1) << " \""
                << pos.fileName << '"' << std::endl; } }
    return out; }

#endif /* _TOOLS_IR_GENERATOR_IRCLASS_H_ */
