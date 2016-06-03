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
IrNamespace IrNamespace::global(nullptr, cstring(0));
static const LookupScope utilScope(nullptr, "Util");
static const NamedType srcInfoType(Util::SourceInfo(), &utilScope, "SourceInfo");
IrField *IrField::srcInfoField = new IrField(Util::SourceInfo(), &srcInfoType, "srcInfo", nullptr,
                                             IrField::Inline | IrField::Optional);
IrClass *IrClass::ideclaration = new IrClass(NodeKind::Interface, "IDeclaration");
IrClass *IrClass::nodeClass = new IrClass(NodeKind::Abstract, "Node", {IrField::srcInfoField});
IrClass *IrClass::vectorClass = new IrClass(NodeKind::Template, "Vector");
IrClass *IrClass::indexedVectorClass = new IrClass(NodeKind::Template, "IndexedVector");
IrClass *IrClass::namemapClass = new IrClass(NodeKind::Template, "NameMap");
IrClass *IrClass::nodemapClass = new IrClass(NodeKind::Template, "NodeMap");
bool LineDirective::inhibit = false;

////////////////////////////////////////////////////////////////////////////////////

IrNamespace *IrNamespace::get(IrNamespace *parent, cstring name) {
    IrNamespace *ns = parent ? parent : &global;
    IrNamespace *rv = ns->children[name];
    if (!rv)
        ns->children[name] = rv = new IrNamespace(ns, name);
    return rv;
}

void IrNamespace::add_class(IrClass *cl) {
    IrNamespace *ns = cl->containedIn ? cl->containedIn : &global;
    if (ns->classes[cl->name])
        throw Util::CompilationError("%1%: Duplicate class name", cl);
    else
        ns->classes[cl->name] = cl;
}

std::ostream &operator<<(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) out << ns->parent << ns->name << "::";
    return out;
}

////////////////////////////////////////////////////////////////////////////////////

Util::Enumerator<IrClass*>* IrDefinitions::getClasses() const {
    return Util::Enumerator<IrElement*>::createEnumerator(elements)
            ->map<IrClass*>([] (IrElement* e) { return dynamic_cast<IrClass*>(e); })
            ->where([] (IrClass* e) { return e != nullptr; });
}

void IrDefinitions::generate(std::ostream &t, std::ostream &out, std::ostream &impl) const {
    std::string macroname = "_IR_GENERATED_H_";
    out << "#ifndef " << macroname << std::endl;
    out << "#define " << macroname << std::endl << std::endl;
    impl << "#include \"ir/ir.h\"" << std::endl;
    impl << "#include \"ir/visitor.h\"" << std::endl;
    impl << std::endl;

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
        for (auto p = cls->containedIn; p && p->name; p = p->parent)
            t << "namespace " << p->name << " {" << std::endl;
        cls->declare(t);
        for (auto p = cls->containedIn; p && p->name; p = p->parent)
            t << "}" << std::endl;
    }
    t << "}" << std::endl;
}

void IrClass::generateTreeMacro(std::ostream &out) const {
    for (auto p = this; p != nodeClass; p = p->getParent())
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

void IrMethod::generate_hdr(std::ostream &out) const {
    if (!args || !body) return;
    if (inImpl) {
        out << IrClass::indent;
        if (rtype)
            out << rtype << " ";
        out << name << args << (isOverride ? " override" : "")
            << ";" << std::endl;
        if (name == "visit_children")
            out << IrClass::indent << rtype << " " << name << args << " const"
                << (isOverride ? " override" : "") << ";" << std::endl;
    } else {
        out << LineDirective(srcInfo) << IrClass::indent;
        if (rtype)
            out << rtype << " ";
        out << name << args << (isOverride ? " override" : "") << " " << body << std::endl;
        if (name == "node_type_name")
            out << LineDirective(srcInfo) << IrClass::indent << "static " << rtype
                << " static_type_name() " << body << std::endl;
        if (srcInfo.isValid())
            out << LineDirective(); }
}

void IrMethod::generate_impl(std::ostream &out) const {
    if (!inImpl || !args || !body) return;
    out << LineDirective(srcInfo);
    if (rtype)
        out << rtype << " ";
    out << "IR::" << clss->containedIn << clss->name << "::" << name << args
        << " " << body << std::endl;
    if (name == "visit_children") {
        out << LineDirective(srcInfo);
        if (rtype)
            out << rtype << " ";
        out << "IR::" << clss->containedIn << clss->name << "::" << name << args << " const "
            << body << std::endl; }
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

static void output_scope_if_needed(std::ostream &out, const IrNamespace *scope,
                                   const IrNamespace *in) {
    if (!scope) return;
    for (auto i = in; i; i = i->parent)
        if (scope == i) return;
    output_scope_if_needed(out, scope->parent, in);
    out << scope->name << "::";
}

void IrClass::generate_hdr(std::ostream &out) const {
    out << "namespace IR {" << std::endl;
    for (auto p = containedIn; p && p->name; p = p->parent)
        out << "namespace " << p->name << " {" << std::endl;
    out << "class " << name;
#if 0
    // TODO(mbudiu): This is not true, but it may be desirable someday
    if (kind == NodeKind::Concrete)
        out << " final";
#endif
    out << " : ";

    bool concreteParent = false;
    for (auto p : parentClasses) {
        if (p->kind != NodeKind::Interface)
            concreteParent = true;
    }

    bool first = true;
    if (!concreteParent) {
        if (kind != NodeKind::Interface)
            out << "public Node";
        else
            out << "public virtual INode";
        first = false;
    }
    for (auto p : parentClasses) {
        if (!first)
            out << ", ";
        out << "public ";
        if (p->kind == NodeKind::Interface)
            out << "virtual ";
        output_scope_if_needed(out, p->containedIn, containedIn);
        out << p->name;
        first = false;
    }

    out << " {" << std::endl;

    auto access = IrElement::Private;
    for (auto e : elements) {
        if (e->access != access) out << (access = e->access);
        e->generate_hdr(out); }

    if (kind != NodeKind::Interface)
        out << indent << "IRNODE" << (kind == NodeKind::Abstract ?  "_ABSTRACT" : "")
            << "_SUBCLASS(" << name << ")" << std::endl;

    out << "};" << std::endl;
    for (auto p = containedIn; p && p->name; p = p->parent)
        out << "}" << std::endl;
    out << "}" << std::endl;
}

void IrClass::generate_impl(std::ostream &out) const {
    for (auto e : elements)
        e->generate_impl(out);
    out << std::endl;
}

void IrClass::computeConstructorArguments(IrClass::ctor_args_t &args) const {
    if (concreteParent == nullptr)
        // direct descendant of Node, add srcInfo
        args.emplace_back(IrField::srcInfoField, IrClass::nodeClass);
    else
        concreteParent->computeConstructorArguments(args);

    for (auto field : *getFields())
        if (!field->initializer || field->optional)
            args.emplace_back(field, this);

    if (args.size() == 0)
        BUG("No constructor arguments?");
}

int IrClass::generateConstructor(const ctor_args_t &arglist, const IrMethod *user,
                                 unsigned skip_opt) {
    // Constructor call has the following shape
    // class T : public P, public I1, public I2 {
    //     F1 f1;
    //     F2 f2;
    //     T(PF1 pf1, PF2 pf2, F1 f1, F2 f2) :
    //         P(pf1, pf2),
    //         f1(f1),
    //         f2(f2)
    //     { validate(); }
    // }
    int optargs = 0;
    std::stringstream args, body;

    bool first = true;
    args << "(";
    for (auto a : arglist) {
        if (a.first->optional && (skip_opt & (1U << optargs++)))
            continue;
        if (!first)
            args << ",\n" << indent << indent;
        a.first->generate(args, false);
        first = false; }
    args << ")";

    body << ":\n" << indent << indent;
    first = true;
    optargs = 0;
    // First print parent arguments
    body << getParent()->name << "(";
    auto it = arglist.begin();
    for (; it != arglist.end(); ++it) {
        if (it->second == this)
            break;  // end of parent arguments
        if (it->first->optional && (skip_opt & (1U << optargs++)))
            continue;
        if (!first)
            body << ", ";
        body << it->first->name;
        first = false; }
    body << ")";
    // Print my fields
    for (; it != arglist.end(); ++it) {
        if (it->first->optional && (skip_opt & (1U << optargs++)))
            continue;
        body << ",\n" << indent << indent << it->first->name << "(" << it->first->name << ")"; }

    body << std::endl << indent << "{";
    if (user)
        body << '\n' << LineDirective(user->getSourceInfo()) << user->body << '\n'
             << LineDirective() << indent;
    body << " validate(); }";

    auto ctor = new IrMethod(name, body.str());
    ctor->args = args.str();
    if (kind != NodeKind::Concrete)
        ctor->access = IrElement::Protected;
    elements.push_back(ctor);
    return optargs;
}

Util::Enumerator<IrField*>* IrClass::getFields() const {
    return Util::Enumerator<IrElement*>::createEnumerator(elements)
            ->where([] (IrElement* e) { return e->is<IrField>(); })
            ->map<IrField*>([] (IrElement* e)->IrField* { return e->to<IrField>(); });
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
        e->set_class(this);
}

////////////////////////////////////////////////////////////////////////////////////

void IrField::generate(std::ostream &out, bool asField) const {
    if (asField)
        out << IrClass::indent;

    auto tmpl = dynamic_cast<const TemplateInstantiation *>(type);
    const IrClass* cls = type->resolve(clss ? clss->containedIn : nullptr);
    if (cls) {
        if (tmpl) {
            if (cls->kind != NodeKind::Template)
                throw Util::CompilationError("Template args with non-template class %1%", cls);
            unsigned tmpl_args = (cls == IrClass::nodemapClass ? 2 : 1);
            if (tmpl->args.size() < tmpl_args)
                throw Util::CompilationError("Wrong number of args for template %1%", cls);
            for (unsigned i = 0; i < tmpl_args; i++) {
                if (auto acl = tmpl->args[i]->resolve(clss ? clss->containedIn : nullptr)) {
                    if (cls == IrClass::vectorClass)
                        acl->needVector = true;
                    else if (cls == IrClass::indexedVectorClass)
                        acl->needIndexedVector = true;
                    else if (cls == IrClass::namemapClass && !isInline)
                        acl->needNameMap = true;
                    else if (cls == IrClass::nodemapClass && !isInline)
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
    out << " " << name;
    if (asField) {
        if (!initializer.isNullOrEmpty())
            out << " = " << initializer;
        else if (cls != nullptr && !isInline)
            out << " = nullptr";
        out << ";";
        out << std::endl; }
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
