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

#include "irclass.h"
#include "lib/exceptions.h"
#include "lib/enumerator.h"

const char* IrClass::indent = "    ";
IrNamespace& IrNamespace::global() {
    static IrNamespace irn(nullptr, nullptr);
    return irn;
}
static const LookupScope utilScope(nullptr, "Util");
static const NamedType srcInfoType(Util::SourceInfo(), &utilScope, "SourceInfo");

IrField* IrField::srcInfoField() {
    static IrField irf(
        Util::SourceInfo(), &srcInfoType, "srcInfo", nullptr, IrField::Inline | IrField::Optional);
    return &irf;
}

IrClass* IrClass::nodeClass() {
    static IrClass irc(NodeKind::Abstract, "Node", {IrField::srcInfoField()});
    return &irc;
}
IrClass* IrClass::vectorClass() {
    static IrClass irc(NodeKind::Template, "Vector");
    return &irc;
}
IrClass* IrClass::namemapClass() {
    static IrClass irc(NodeKind::Template, "NameMap");
    return &irc;
}
IrClass* IrClass::nodemapClass() {
    static IrClass irc(NodeKind::Template, "NodeMap");
    return &irc;
}
IrClass* IrClass::ideclaration() {
    static IrClass irc(NodeKind::Interface, "IDeclaration");
    return &irc;
}
IrClass* IrClass::indexedVectorClass() {
    static IrClass irc(NodeKind::Template, "IndexedVector");
    return &irc;
}
bool LineDirective::inhibit = false;

////////////////////////////////////////////////////////////////////////////////////

IrNamespace *IrNamespace::get(IrNamespace *parent, cstring name) {
    IrNamespace *ns = parent ? parent : &global();
    IrNamespace *rv = ns->children[name];
    if (!rv)
        ns->children[name] = rv = new IrNamespace(ns, name);
    return rv;
}

void IrNamespace::add_class(IrClass *cl) {
    IrNamespace *ns = cl->containedIn ? cl->containedIn : &global();
    if (ns->classes[cl->name])
        throw Util::CompilationError("%1%: Duplicate class name", cl->name);
    else
        ns->classes[cl->name] = cl;
}

std::ostream &operator<<(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) out << ns->parent << ns->name << "::";
    return out;
}

void enter_namespace(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) {
        enter_namespace(out, ns->parent);
        out << "namespace " << ns->name << " {" << std::endl; }
}

void exit_namespace(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) {
        exit_namespace(out, ns->parent);
        out << "}  // namespace " << ns->name << std::endl; }
}

////////////////////////////////////////////////////////////////////////////////////

Util::Enumerator<IrClass*>* IrDefinitions::getClasses() const {
    return Util::Enumerator<IrElement*>::createEnumerator(elements)
            ->map<IrClass*>([] (IrElement* e) { return dynamic_cast<IrClass*>(e); })
            ->where([] (IrClass* e) { return e != nullptr; });
}

void IrDefinitions::generate(std::ostream &t, std::ostream &out, std::ostream &impl) const {
    std::string macroname = "_IR_GENERATED_H_";
    out << "#ifndef " << macroname << "\n"
        << "#define " << macroname << "\n" << std::endl;

    impl << "#include \"ir/ir.h\"\n"
         << "#include \"ir/visitor.h\"\n"
         << "#include \"ir/json_loader.h\"\n" << std::endl;

    out << "#include <map>\n"
        << "#include <functional>\n" << std::endl
        << "class JSONLoader;\n"
        << "using NodeFactoryFn = IR::Node*(*)(JSONLoader&);\n"
        << std::endl
        << "namespace IR {\n"
        << "extern std::map<cstring, NodeFactoryFn> unpacker_table;\n"
        << "}\n";

    impl << "std::map<cstring, NodeFactoryFn> IR::unpacker_table = {\n";

    bool first = true;
    for (auto cls : *getClasses()) {
        if (cls->kind == NodeKind::Concrete) {
            if (first)
                first = false;
            else
                impl << ",\n";
            impl << "{\"" << cls->name << "\", NodeFactoryFn(&IR::";
            if (cls->containedIn && cls->containedIn->name)
                impl << cls->containedIn->name << "::";
            impl << cls->name << "::fromJSON)}"; } }
    impl << " };\n" << std::endl;

    for (auto e : elements) {
        e->generate_hdr(out);
        e->generate_impl(impl); }

    out << "#endif /* " << macroname << " */" << std::endl;

    ///////////////////////////////// tree

    t << "#define IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, T, D, B, ...) \\"
      << std::endl;
    for (auto cls : *getClasses())
        if (cls->kind != NodeKind::Interface)
            cls->generateTreeMacro(t);

    t << "T(Vector<IR::Node>, D(Node), ##__VA_ARGS__) \\" << std::endl;
    t << "T(IndexedVector<IR::Node>, "
            "D(Vector<IR::Node>) "
            "B(Node), ##__VA_ARGS__) \\" << std::endl;
    for (auto cls : *getClasses()) {
        if (cls->needVector || cls->needIndexedVector)
            t << "T(Vector<IR::" << cls->containedIn << cls->name << ">, D(Node), "
                    "##__VA_ARGS__) \\" << std::endl;
        if (cls->needIndexedVector)
            // We generate IndexedVector only if needed; we expect users won't use
            // these if they don't want to place them in fields.
            t << "T(IndexedVector<IR::" << cls->containedIn << cls->name << ">, "
                    "D(Vector<IR::" << cls->containedIn << cls->name << ">) "
                    "B(Node), ##__VA_ARGS__) \\" << std::endl;
        if (cls->needNameMap)
            BUG("visitable (non-inline) NameMap not yet implemented");
        if (cls->needNodeMap)
            BUG("visitable (non-inline) NodeMap not yet implemented"); }
    t << std::endl;

    t << "namespace IR {" << std::endl;
    for (auto cls : *getClasses()) {
        enter_namespace(t, cls->containedIn);
        cls->declare(t);
        exit_namespace(t, cls->containedIn);
    }
    t << "}  // namespace IR" << std::endl;
}

void IrClass::generateTreeMacro(std::ostream &out) const {
    for (auto p = this; p != nodeClass(); p = p->getParent())
        out << "  ";
    out << "M(";
    const char *sep = "";
    for (auto p = this; p; p = p->getParent()) {
        out << sep << p->containedIn << p->name;
        sep = *sep ? ") B(" : ", D("; }
    out << "), ##__VA_ARGS__) \\" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////

void EmitBlock::generate_hdr(std::ostream &out) const {
    if (!impl)
        out << LineDirective(srcInfo, +1) << body << LineDirective();
}
void EmitBlock::generate_impl(std::ostream &out) const {
    if (impl)
        out << LineDirective(srcInfo, +1) << body << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrMethod::generate_proto(std::ostream &out, bool fullname, bool defaults) const {
    if (rtype) {
        if (rtype->isResolved()) out << "const ";
        out << rtype->toString() << " ";
        if (rtype->isResolved()) out << "*"; }
    if (fullname && !isFriend)
        out << "IR::" << clss->containedIn << clss->name << "::";
    out << name << "(";
    const char *sep = "";
    for (auto *a : args) {
        out << sep;
        a->generate(out, false);
        if (a->initializer && defaults)
            out << " = " << a->initializer;
        sep = ", "; }
    out << ")" << (isConst ? " const" : "");
}

void IrMethod::generate_hdr(std::ostream &out) const {
    if (srcInfo.isValid())
        out << LineDirective(srcInfo);
    out << IrClass::indent;
    if (isStatic) out << "static ";
    if (isVirtual) out << "virtual ";
    if (isFriend) out << "friend ";
    generate_proto(out, false, isUser);
    if (isOverride) out << " override";
    if (inImpl || !body)
        out << ';';
    else
        out << ' ' << body;
    out << std::endl;
    if (name == "visit_children") {
        out << IrClass::indent;
        generate_proto(out, false, isUser);
        out << " const";
        if (isOverride) out << " override";
        out << ";" << std::endl;
    } else if (name == "node_type_name") {
        out << LineDirective(srcInfo) << IrClass::indent << "static " << rtype->toString()
            << " static_type_name() " << body << std::endl; }
    if (srcInfo.isValid())
        out << LineDirective();
}

void IrMethod::generate_impl(std::ostream &out) const {
    if (!inImpl || !body) return;
    out << LineDirective(srcInfo);
    generate_proto(out, true, false);
    out << " " << body << std::endl;
    if (name == "visit_children") {
        out << LineDirective(srcInfo);
        generate_proto(out, true, false);
        out << " const " << body << std::endl; }
    if (srcInfo.isValid())
        out << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrApply::generate_hdr(std::ostream &out) const {
    out << IrClass::indent << "IRNODE_DECLARE_APPLY_OVERLOAD(" << clss->name << ")" << std::endl;
}

void IrApply::generate_impl(std::ostream &out) const {
    out << "IRNODE_DEFINE_APPLY_OVERLOAD(" << clss->containedIn << clss->name << ", , )"
        << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////

void IrClass::declare(std::ostream &out) const {
    out << "class " << name << ";" << std::endl;
}

std::string IrClass::fullName() const {
    std::stringstream tmp;
    tmp << "IR::" << containedIn << name;
    return tmp.str();
}

static void output_scope_if_needed(std::ostream &out, const IrNamespace *scope,
                                   const IrNamespace *in) {
    if (!scope) return;
    for (auto i = in; i; i = i->parent)
        if (scope == i) return;
    output_scope_if_needed(out, scope->parent, in);
    out << scope->name << "::";
}

void IrClass::generate_hdr(std::ostream &out) const {
    if (kind != NodeKind::Nested) {
        out << "namespace IR {" << std::endl;
        enter_namespace(out, containedIn); }
    for (auto cblock : comments) cblock->generate_hdr(out);
    out << "class " << name;

    bool concreteParent = false;
    for (auto p : parentClasses) {
        if (p->kind != NodeKind::Interface)
            concreteParent = true;
    }

    const char *sep = " : ";
    if (!concreteParent && kind != NodeKind::Nested) {
        if (kind != NodeKind::Interface)
            out << sep << "public Node";
        else
            out << sep << "public virtual INode";
        sep = ", "; }
    for (auto p : parentClasses) {
        out << sep << "public ";
        if (p->kind == NodeKind::Interface)
            out << "virtual ";
        output_scope_if_needed(out, p->containedIn, containedIn);
        out << p->name;
        sep = ", "; }

    out << " {" << std::endl;

    auto access = IrElement::Private;
    for (auto e : elements) {
        if (e->access != access) out << (access = e->access);
        e->generate_hdr(out); }

    if (kind != NodeKind::Interface && kind != NodeKind::Nested)
        out << indent << "IRNODE" << (kind == NodeKind::Abstract ?  "_ABSTRACT" : "")
            << "_SUBCLASS(" << name << ")" << std::endl;

    out << "};" << std::endl;
    if (kind != NodeKind::Nested) {
        exit_namespace(out, containedIn);
        out << "}  // namespace IR" << std::endl; }
}

void IrClass::generate_impl(std::ostream &out) const {
    for (auto e : elements)
        e->generate_impl(out);
}

void IrClass::computeConstructorArguments(IrClass::ctor_args_t &args) const {
    if (concreteParent == nullptr) {
        if (kind != NodeKind::Nested) {
            // direct descendant of Node, add srcInfo
            args.emplace_back(IrField::srcInfoField(), IrClass::nodeClass()); }
    } else {
        concreteParent->computeConstructorArguments(args); }

    for (auto field : *getFields())
        if (!field->isStatic && (!field->initializer|| field->optional))
            args.emplace_back(field, this);
}

int IrClass::generateConstructor(const ctor_args_t &arglist, const IrMethod *user,
                                 unsigned skip_opt) {
    // Constructor call has the following shape
    // class T : public P, public I1, public I2 {
    //     F1 f1;
    //     F2 f2;
    //     T(PF1 pf1, PF2 pf2, F1 f1, F2 f2) :
    //         P(pf1, pf2), f1(f1), f2(f2)
    //     { validate(); }
    // }
    int optargs = 0;
    std::stringstream body;
    const char *sep = ":\n    ";
    auto parent = getParent() ? getParent()->name : cstring();
    const char *end_parent = "";
    for (auto &arg : arglist) {
        if (arg.first->optional && (skip_opt & (1U << optargs++)))
            continue;
        if (arg.second == this) {
            body << end_parent;
            end_parent = "";
        } else if (parent) {
            body << sep << parent;
            parent = nullptr;
            sep = "(";
            end_parent = ")"; }
        body << sep << arg.first->name;
        if (arg.second == this)
            body << "(" << arg.first->name << ")";
        sep = ", "; }

    body << end_parent << std::endl << indent << "{";
    if (user)
        body << '\n' << LineDirective(user->getSourceInfo()) << user->body << '\n'
             << LineDirective() << indent;
    if (kind != NodeKind::Nested)
        body << " validate(); ";
    body << "}";
    auto ctor = new IrMethod(name, body.str());
    ctor->clss = this;
    optargs = 0;
    for (auto a : arglist) {
        if (a.first->optional && (skip_opt & (1U << optargs++)))
            continue;
        ctor->args.push_back(a.first); }

    if (kind == NodeKind::Abstract)
        ctor->access = IrElement::Protected;
    ctor->inImpl = true;
    elements.push_back(ctor);
    return optargs;
}

Util::Enumerator<IrField*>* IrClass::getFields() const {
    return Util::Enumerator<IrElement*>::createEnumerator(elements)
            ->where([] (IrElement *e) { return e->is<IrField>(); })
            ->map<IrField*>([] (IrElement *e)->IrField* { return e->to<IrField>(); })
            ->where([] (IrField *f) { return !f->isStatic; });
}

Util::Enumerator<IrMethod*>* IrClass::getUserMethods() const {
    return Util::Enumerator<IrElement*>::createEnumerator(elements)
            ->where([] (IrElement* e) { return e->is<IrMethod>(); })
            ->map<IrMethod*>([] (IrElement* e)->IrMethod* { return e->to<IrMethod>(); });
}

bool IrClass::shouldSkip(cstring feature) const {
    // skip if there is a 'no' directive
    auto *e = Util::Enumerator<IrElement*>::createEnumerator(elements);
    bool explicitNo = e->where([] (IrElement *el) { return el->is<IrNo>(); })
            ->where([feature] (IrElement *el) { return el->to<IrNo>()->text == feature; })
            ->any();
    if (explicitNo) return true;
    // also, skip if the user provided an implementation manually
    // (except for validate)
    if (feature == "validate") return false;

    e = Util::Enumerator<IrElement*>::createEnumerator(elements);
    bool provided = e->where([] (IrElement* e) { return e->is<IrMethod>(); })
            ->where([feature] (IrElement* e) { return e->to<IrMethod>()->name == feature; })
            ->any();
    return provided;
}

void IrClass::resolve() {
    for (auto s : parents) {
        const IrClass *p = s->resolve(containedIn);
        if (p == nullptr)
            throw Util::CompilationError("Could not find class %1%", s);
        if (p->kind != NodeKind::Interface) {
            if (concreteParent == nullptr)
                concreteParent = p;
            else
                BUG(
                    "Class %1% has more than 1 non-interface parent: %2% and %3%",
                    this, concreteParent, p); }
        parentClasses.push_back(p); }
    generateMethods();
    for (auto e : elements)
        e->resolve();
}

////////////////////////////////////////////////////////////////////////////////////

void IrField::generate(std::ostream &out, bool asField) const {
    if (asField) {
        out << IrClass::indent;
        if (isStatic) out << "static ";
        if (isConst) out << "const "; }

    auto tmpl = dynamic_cast<const TemplateInstantiation *>(type);
    const IrClass* cls = type->resolve(clss ? clss->containedIn : nullptr);
    if (cls) {
        // FIXME -- should be doing this in resolve and converting type to PointerType as needed
        if (tmpl) {
            if (cls->kind != NodeKind::Template)
                throw Util::CompilationError("Template args with non-template class %1%", cls);
            unsigned tmpl_args = (cls == IrClass::nodemapClass() ? 2 : 1);
            if (tmpl->args.size() < tmpl_args)
                throw Util::CompilationError("Wrong number of args for template %1%", cls);
            for (unsigned i = 0; i < tmpl_args; i++) {
                if (auto acl = tmpl->args[i]->resolve(clss ? clss->containedIn : nullptr)) {
                    if (cls == IrClass::vectorClass())
                        acl->needVector = true;
                    else if (cls == IrClass::indexedVectorClass())
                        acl->needIndexedVector = true;
                    else if (cls == IrClass::namemapClass() && !isInline)
                        acl->needNameMap = true;
                    else if (cls == IrClass::nodemapClass() && !isInline)
                        acl->needNodeMap = true;
                } else {
                    throw Util::CompilationError("%1% template argment %2% is not "
                                                 "an IR class", cls->name, tmpl->args[i]); } }
        } else if (cls->kind == NodeKind::Template) {
            throw Util::CompilationError("No args for template %1%", cls); } }
    if (cls != nullptr && !isInline)
        out << "const ";
    out << type->toString();
    if (cls != nullptr && !isInline)
        out << "*";
    out << " " << name << type->declSuffix();
    if (asField) {
        if (!isStatic) {
            if (!initializer.isNullOrEmpty())
                out << " = " << initializer;
            else if (cls != nullptr && !isInline)
                out << " = nullptr"; }
        out << ";";
        out << std::endl; }
}

void IrField::generate_impl(std::ostream &) const {
    if (!isStatic) return;
    // FIXME -- for now statics are manually generated elsewhere
}

////////////////////////////////////////////////////////////////////////////////////

void ConstFieldInitializer::generate_hdr(std::ostream &out) const {
    out << IrClass::indent;
    if (name == "precedence")
        out << "int getPrecedence() const override { return ";
    else if (name == "stringOp")
        out << "cstring getStringOp() const override { return ";
    else
        throw Util::CompilationError("Unexpected constant field %1%", this);
    out << initializer << "; }" << std::endl;
}
