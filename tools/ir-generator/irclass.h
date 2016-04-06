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

class IrNamespace {
    std::map<cstring, IrClass *>        classes;
    std::map<cstring, IrNamespace *>    children;
    static IrNamespace                  global;
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
 protected:
    Util::SourceInfo srcInfo;
    const IrClass *clss = nullptr;

 public:
    IrElement() = default;
    explicit IrElement(Util::SourceInfo info) : srcInfo(info) {}
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    template<class T> T *to() { return dynamic_cast<T *>(this); }
    template<class T> const T *to() const { return dynamic_cast<const T *>(this); }
    template<class T> bool is() const { return to<T>() != nullptr; }
    virtual void generate_hdr(std::ostream &out) const = 0;
    virtual void generate_impl(std::ostream &out) const = 0;
    void set_class(const IrClass *cl) { clss = cl; }
};

class IrMethod : public IrElement {
 public:
    const cstring name;
    cstring rtype, args, body;
    bool inImpl = false, isOverride = false;
    IrMethod(Util::SourceInfo info, cstring name, cstring body)
    : IrElement(info), name(name), body(body) {}
    IrMethod(cstring name, cstring body) : name(name), body(body) {}
    cstring toString() const override { return name; }

    void generate_hdr(std::ostream &) const override;
    void generate_impl(std::ostream &) const override;
    struct info_t {
        cstring rtype, args;
        int flags;
        std::function<cstring(IrClass *, cstring)>       create;
    };
    static const ordered_map<cstring, info_t> Generate;
};

class IrField : public IrElement {
 public:
    const Type *type;
    const cstring name;
    const cstring initializer;
    const bool nullOK;
    const bool isInline;

    static IrField *srcInfoField;

    IrField(Util::SourceInfo info, const Type *type, cstring name, cstring initializer,
            bool nullOK, bool isInline)
    : IrElement(info), type(type), name(name), initializer(initializer), nullOK(nullOK),
      isInline(isInline) {}
    void generate(std::ostream &out, bool asField) const;
    void generate_hdr(std::ostream &out) const override { generate(out, true); }
    void generate_impl(std::ostream &) const override {}
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

// Represents a C++ class for an IR node.
class IrClass : public IrElement {
    std::vector<const IrClass *> parentClasses;

    std::vector<const Type *> parents;
    std::vector<IrElement *> elements;

    void generateConstructor(std::ostream &out) const;
    void generateMethods();

    // each argument together with the class that has to receive it
    typedef std::vector<std::pair<const IrField*, const IrClass*>> ctor_args_t;
    void computeConstructorArguments(ctor_args_t &out) const;
    bool shouldSkip(cstring feature) const;

    const IrClass *concreteParent;

 public:
    const IrClass *getParent() const {
        if (concreteParent == nullptr && this != nodeClass)
            return IrClass::nodeClass;
        return concreteParent; }

    IrNamespace *containedIn, local;
    const NodeKind kind;
    const cstring name;
    bool needVector = false;  // using a Vector of this class
    bool needIndexedVector = false;  // using an IndexedVecor of this class
    bool needNameMap = false;  // using a NameMap of this class
    bool needNodeMap = false;  // using a NodeMap of this class

    static const char* indent;

    IrClass(Util::SourceInfo info, IrNamespace *ns, NodeKind kind, cstring name,
            const std::initializer_list<const Type *> &parents,
            const std::initializer_list<IrElement *> &elements)
    : IrElement(info), parents(parents), elements(elements), concreteParent(nullptr),
      containedIn(ns && ns->name ? ns : nullptr), local(containedIn, name), kind(kind), name(name) {
        IrNamespace::add_class(this); }
    IrClass(Util::SourceInfo info, IrNamespace *ns, NodeKind kind, cstring name,
            const std::vector<const Type *> *parents = nullptr,
            const std::vector<IrElement *> *elements = nullptr)
    : IrElement(info), concreteParent(nullptr), containedIn(ns && ns->name ? ns : nullptr),
      local(containedIn, name), kind(kind), name(name) {
        if (parents) this->parents = *parents;
        if (elements) this->elements = *elements;
        IrNamespace::add_class(this); }
    IrClass(NodeKind kind, cstring name) : IrClass(Util::SourceInfo(), nullptr, kind, name) {}
    IrClass(NodeKind kind, cstring name, const std::initializer_list<IrElement *> &elements)
    : elements(elements), concreteParent(nullptr), containedIn(nullptr), local(containedIn, name),
      kind(kind), name(name) {
        IrNamespace::add_class(this); }

    static IrClass *nodeClass, *vectorClass, *namemapClass, *nodemapClass, *ideclaration, *indexedVectorClass;

    void declare(std::ostream &out) const;
    void generate_hdr(std::ostream &out) const override;
    void generate_impl(std::ostream &out) const override;
    void generateTreeMacro(std::ostream &out) const;
    void resolve();
    cstring toString() const override { return name; }
    Util::Enumerator<IrField*>* getFields() const;
    Util::Enumerator<IrMethod*>* getUserMethods() const;
};

class IrDefinitions {
    std::vector<IrElement*> elements;
    Util::Enumerator<IrClass*>* getClasses() const;

 public:
    explicit IrDefinitions(std::vector<IrElement*> classes) : elements(classes) {}
    void resolve() {
        IrClass::nodeClass->resolve();
        IrClass::vectorClass->resolve();
        for (auto c : elements) {
            IrClass* cls = dynamic_cast<IrClass*>(c);
            if (cls != nullptr)
                cls->resolve();
        }
    }
    void generate(std::ostream &t, std::ostream &out, std::ostream &impl) const;
};

class LineDirective {
    Util::SourceInfo    loc;
    int                 delta = 0;
    bool                reset = false;
    friend std::ostream &operator<<(std::ostream &, const LineDirective &);
 public:
    static bool inhibit;
    explicit LineDirective(Util::SourceInfo l) : loc(l) {}
    LineDirective(Util::SourceInfo l, int d) : loc(l), delta(d) {}
    LineDirective() : reset(true) {}
};

inline std::ostream &operator<<(std::ostream &out, const LineDirective &l) {
    if (!LineDirective::inhibit) {
        if (l.reset) {
            out << "#" << std::endl;   // will be fixed by tools/fixup-line-directives
        } else if (l.loc.isValid()) {
            auto pos = l.loc.toPosition();
            out << "#line " << (pos.sourceLine + l.delta + 1) << " \""
                << pos.fileName << '"' << std::endl; } }
    return out; }

#endif /* _TOOLS_IR_GENERATOR_IRCLASS_H_ */
