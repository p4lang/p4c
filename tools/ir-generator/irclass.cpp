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

#include "lib/enumerator.h"
#include "lib/exceptions.h"

const char *IrClass::indent = "    ";
IrNamespace &IrNamespace::global() {
    static IrNamespace irn(nullptr, nullptr);
    return irn;
}
static const LookupScope utilScope(nullptr, "Util");
static const NamedType srcInfoType(Util::SourceInfo(), &utilScope, "SourceInfo");

IrField *IrField::srcInfoField() {
    static IrField irf(Util::SourceInfo(), &srcInfoType, "srcInfo", nullptr,
                       IrField::Inline | IrField::Optional);
    return &irf;
}

IrClass *IrClass::nodeClass() {
    static IrClass irc(NodeKind::Abstract, "Node", {IrField::srcInfoField()});
    return &irc;
}
IrClass *IrClass::vectorClass() {
    static IrClass irc(NodeKind::Template, "Vector");
    return &irc;
}
IrClass *IrClass::namemapClass() {
    static IrClass irc(NodeKind::Template, "NameMap");
    return &irc;
}
IrClass *IrClass::nodemapClass() {
    static IrClass irc(NodeKind::Template, "NodeMap");
    return &irc;
}
IrClass *IrClass::ideclaration() {
    static IrClass irc(NodeKind::Interface, "IDeclaration");
    return &irc;
}
IrClass *IrClass::indexedVectorClass() {
    static IrClass irc(NodeKind::Template, "IndexedVector");
    return &irc;
}
bool LineDirective::inhibit = false;

////////////////////////////////////////////////////////////////////////////////////

IrNamespace *IrNamespace::get(IrNamespace *parent, cstring name) {
    IrNamespace *ns = parent ? parent : &global();
    IrNamespace *rv = ns->children[name];
    if (!rv) ns->children[name] = rv = new IrNamespace(ns, name);
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
        out << "namespace " << ns->name << " {" << std::endl;
    }
}

void exit_namespace(std::ostream &out, IrNamespace *ns) {
    if (ns && ns->name) {
        exit_namespace(out, ns->parent);
        out << "}  // namespace " << ns->name << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////

Util::Enumerator<IrClass *> *IrDefinitions::getClasses() const {
    return Util::enumerate(elements)->as<IrClass *>()->where(
        [](IrClass *e) { return e != nullptr; });
}

void IrDefinitions::generate(std::ostream &t, std::ostream &out, std::ostream &impl) const {
    std::string macroname = "IR_GENERATED_H_";
    out << "#ifndef " << macroname << "\n"
        << "#define " << macroname << "\n"
        << std::endl;

    impl << "#include \"ir/ir-generated.h\"    // IWYU pragma: keep\n\n"
         << "#include \"ir/ir-inline.h\"       // IWYU pragma: keep\n"
         << "#include \"ir/json_generator.h\"  // IWYU pragma: keep\n"
         << "#include \"ir/json_loader.h\"     // IWYU pragma: keep\n"
         << "#include \"ir/visitor.h\"         // IWYU pragma: keep\n"
         << "#include \"lib/algorithm.h\"      // IWYU pragma: keep\n"
         << "#include \"lib/log.h\"            // IWYU pragma: keep\n"
         << std::endl;

    out << "#include <functional>\n"
        << "#include <map>\n\n"
        << "// Special IR classes and types\n"
        << "#include \"ir/dbprint.h\"         // IWYU pragma: keep\n"
        << "#include \"ir/id.h\"              // IWYU pragma: keep\n"
        << "#include \"ir/indexed_vector.h\"  // IWYU pragma: keep\n"
        << "#include \"ir/namemap.h\"         // IWYU pragma: keep\n"
        << "#include \"ir/node.h\"            // IWYU pragma: keep\n"
        << "#include \"ir/nodemap.h\"         // IWYU pragma: keep\n"
        << "#include \"ir/vector.h\"          // IWYU pragma: keep\n"
        << std::endl
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
            if (cls->containedIn && cls->containedIn->name) impl << cls->containedIn->name << "::";
            impl << cls->name << "::fromJSON)}";
        }
    }
    impl << " };\n" << std::endl;

    for (auto e : elements) {
        e->generate_hdr(out);
        e->generate_impl(impl);
    }

    out << "#endif /* " << macroname << " */" << std::endl;

    ///////////////////////////////// tree

    t << "#pragma once\n"
      << "#include <cstdint>\n"
      << "#include \"lib/rtti.h\"\n";

    t << "#define IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, T, D, B, ...) \\"
      << std::endl;
    for (auto cls : *getClasses())
        if (cls->kind != NodeKind::Interface) cls->generateTreeMacro(t);

    t << "T(Vector<IR::Node>, D(Node), ##__VA_ARGS__) \\" << std::endl;
    t << "T(IndexedVector<IR::Node>, "
         "D(Vector<IR::Node>) "
         "B(Node), ##__VA_ARGS__) \\"
      << std::endl;
    for (auto cls : *getClasses()) {
        if (cls->needVector || cls->needIndexedVector)
            t << "T(Vector<IR::" << cls->containedIn << cls->name
              << ">, D(Node), "
                 "##__VA_ARGS__) \\"
              << std::endl;
        if (cls->needIndexedVector)
            // We generate IndexedVector only if needed; we expect users won't use
            // these if they don't want to place them in fields.
            t << "T(IndexedVector<IR::" << cls->containedIn << cls->name
              << ">, "
                 "D(Vector<IR::"
              << cls->containedIn << cls->name
              << ">) "
                 "B(Node), ##__VA_ARGS__) \\"
              << std::endl;
        if (cls->needNameMap) BUG("visitable (non-inline) NameMap not yet implemented");
        if (cls->needNodeMap) BUG("visitable (non-inline) NodeMap not yet implemented");
    }
    t << std::endl;

    t << "namespace IR {" << std::endl;

    // Emit forward declarations
    for (auto *cls : *getClasses()) {
        enter_namespace(t, cls->containedIn);
        cls->declare(t);
        exit_namespace(t, cls->containedIn);
    }

    t << std::endl;

    // Emit node kinds
    // TODO: Probably it would make sense to topo-sort the IDs to optimize the
    // comparison trees generated by a compiler
    t << "enum class NodeKind : RTTI::TypeId {\n"
      << "  Auto = 0,\n"
      << "  INode = 1,\n"
      << "  Node = 2,\n";

    unsigned nkId = 3;
    auto *irNamespace = IrNamespace::get(nullptr, "IR");
    for (auto *cls : *getClasses())
        t << "  " << cls->qualified_name(irNamespace).replace("::", "_") << " = " << nkId++
          << ",\n";

    // Add some specials:
    t << "  IDeclaration = " << nkId++ << ",\n";
    t << "  VectorBase = " << nkId++ << "\n"
      << "};\n";
    t << "enum class NodeDiscriminator : RTTI::TypeId {\n"
      << "  NodeT = UINT64_C(1),\n"
      << "  VectorT = UINT64_C(1),\n"
      << "  IndexedVectorT = UINT64_C(2),\n"
      << "  Auto = UINT64_C(0xFF)\n"
      << "};\n"
      << " inline bool operator==(RTTI::TypeId lhs, NodeKind rhs) { return lhs == "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator==(NodeKind lhs, RTTI::TypeId rhs) { return RTTI::TypeId(lhs) == "
         "rhs; }\n"
      << " inline bool operator!=(RTTI::TypeId lhs, NodeKind rhs) { return lhs != "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator!=(NodeKind lhs, RTTI::TypeId rhs) { return RTTI::TypeId(lhs) != "
         "rhs; }\n"
      << " inline bool operator==(RTTI::TypeId lhs, NodeDiscriminator rhs) { return lhs == "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator==(NodeDiscriminator lhs, RTTI::TypeId rhs) { return "
         "RTTI::TypeId(lhs) == rhs; }\n"
      << " inline bool operator!=(RTTI::TypeId lhs, NodeDiscriminator rhs) { return lhs != "
         "RTTI::TypeId(rhs); }\n"
      << " inline bool operator!=(NodeDiscriminator lhs, RTTI::TypeId rhs) { return "
         "RTTI::TypeId(lhs) != rhs; }\n";
    t << "}  // namespace IR" << std::endl;
}

void IrClass::generateTreeMacro(std::ostream &out) const {
    auto *p = this;
    for (; p && p != nodeClass(); p = p->getParent()) {
        out << "  ";
    }
    BUG_CHECK(p != nullptr, "Falled out of the class hierarchy");
    out << "M(";
    const char *sep = "";
    for (p = this; p; p = p->getParent()) {
        out << sep << p->containedIn << p->name;
        sep = *sep ? ") B(" : ", D(";
    }
    out << "), ##__VA_ARGS__) \\" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////

void EmitBlock::generate_hdr(std::ostream &out) const {
    if (!impl) out << LineDirective(srcInfo, +1) << body << LineDirective();
}
void EmitBlock::generate_impl(std::ostream &out) const {
    if (impl) out << LineDirective(srcInfo, +1) << body << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrMethod::generate_proto(std::ostream &out, bool fullname, bool defaults) const {
    if (rtype) {
        if (rtype->isResolved()) out << "const ";
        out << rtype->toString() << " ";
        if (rtype->isResolved()) out << "*";
    }
    if (fullname && !isFriend) out << "IR::" << clss->containedIn << clss->name << "::";
    out << name << "(";
    const char *sep = "";
    for (auto *a : args) {
        out << sep;
        a->generate(out, false);
        if (a->initializer && defaults) out << " = " << a->initializer;
        sep = ", ";
    }
    out << ")" << (isConst ? " const" : "");
}

void IrMethod::generate_hdr(std::ostream &out) const {
    if (srcInfo.isValid()) out << LineDirective(srcInfo);
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
            << " static_type_name() " << body << std::endl;
    }
    if (srcInfo.isValid()) out << LineDirective();
}

void IrMethod::generate_impl(std::ostream &out) const {
    if (!inImpl || !body) return;
    out << LineDirective(srcInfo);
    generate_proto(out, true, false);
    out << " " << body << std::endl;
    if (name == "visit_children") {
        out << LineDirective(srcInfo);
        generate_proto(out, true, false);
        out << " const " << body << std::endl;
    }
    if (srcInfo.isValid()) out << LineDirective();
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

void IrClass::declare(std::ostream &out) const { out << "class " << name << ";" << std::endl; }

std::string IrClass::fullName() const {
    std::stringstream tmp;
    tmp << "IR::" << containedIn << name;
    return tmp.str();
}

cstring IrNamespace::qualified_name(const IrNamespace *in) const {
    cstring rv = name ? name : "IR";
    if (parent) {
        for (auto i = in; i; i = i->parent) {
            auto sym = i->lookupChild(name);
            if (sym && this != sym) break;
            if (parent == i) return rv;
        }
        rv = parent->qualified_name(in) + "::" + rv;
    }
    return rv;
}

cstring IrClass::qualified_name(const IrNamespace *in) const {
    cstring rv = name;
    if (containedIn) {
        for (auto i = in; i; i = i->parent) {
            auto sym = i->lookupClass(name);
            if (sym && this != sym) break;
            if (containedIn == i) return rv;
        }
        rv = containedIn->qualified_name(in) + "::" + rv;
    }
    return rv;
}

void IrClass::generate_hdr(std::ostream &out) const {
    if (kind != NodeKind::Nested) {
        out << "namespace IR {" << std::endl;
        enter_namespace(out, containedIn);
    }
    for (auto cblock : comments) cblock->generate_hdr(out);
    out << "class " << name;

    bool concreteParent = false;
    for (auto p : parentClasses) {
        if (p->kind != NodeKind::Interface) concreteParent = true;
    }

    const char *sep = " : ";
    if (!concreteParent && kind != NodeKind::Nested) {
        if (kind != NodeKind::Interface)
            out << sep << "public Node";
        else
            out << sep << "public virtual INode";
        sep = ", ";
    }
    for (auto p : parentClasses) {
        out << sep << "public ";
        if (p->kind == NodeKind::Interface) out << "virtual ";
        out << p->qualified_name(containedIn);
        sep = ", ";
    }

    out << " {" << std::endl;

    auto access = IrElement::Private;
    for (auto e : elements) {
        if (e->access != access) out << (access = e->access);
        e->generate_hdr(out);
    }

    if (kind != NodeKind::Interface && kind != NodeKind::Nested)
        out << indent << "IRNODE" << (kind == NodeKind::Abstract ? "_ABSTRACT" : "") << "_SUBCLASS("
            << name << ")" << std::endl;

    auto *irNamespace = IrNamespace::get(nullptr, "IR");
    if (kind != NodeKind::Nested) {
        out << indent << "DECLARE_TYPEINFO_WITH_TYPEID(" << name
            << ", NodeKind::" << qualified_name(irNamespace).replace("::", "_");
        if (!concreteParent) out << ", " << (kind != NodeKind::Interface ? "Node" : "INode");
        for (const auto *p : parentClasses) out << ", " << p->qualified_name(containedIn);
        out << ");" << std::endl;
    }

    out << "};" << std::endl;
    if (kind != NodeKind::Nested) {
        exit_namespace(out, containedIn);
        out << "}  // namespace IR" << std::endl;
    }
}

void IrClass::generate_impl(std::ostream &out) const {
    for (auto e : elements) e->generate_impl(out);
}

void IrClass::computeConstructorArguments(IrClass::ctor_args_t &args) const {
    if (concreteParent == nullptr) {
        if (kind != NodeKind::Nested) {
            // direct descendant of Node, add srcInfo
            args.emplace_back(IrField::srcInfoField(), IrClass::nodeClass());
        }
    } else {
        concreteParent->computeConstructorArguments(args);
    }

    for (auto field : *getFields())
        if (!field->isStatic && (!field->initializer || field->optional))
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
    auto parent = getParent() ? getParent()->qualified_name(containedIn) : cstring();
    const char *end_parent = "";
    for (auto &arg : arglist) {
        if (arg.first->optional && (skip_opt & (1U << optargs++))) continue;
        if (arg.second == this) {
            body << end_parent;
            end_parent = "";
        } else if (parent) {
            body << sep << parent;
            parent = nullptr;
            sep = "(";
            end_parent = ")";
        }
        body << sep << arg.first->name;
        if (arg.second == this) body << "(" << arg.first->name << ")";
        sep = ", ";
    }

    body << end_parent << std::endl << indent << "{";
    if (user)
        body << '\n'
             << LineDirective(user->getSourceInfo()) << user->body << '\n'
             << LineDirective() << indent;
    if (kind != NodeKind::Nested) body << " validate(); ";
    body << "}";
    auto ctor = new IrMethod(name, body.str());
    ctor->clss = this;
    optargs = 0;
    for (auto a : arglist) {
        if (a.first->optional && (skip_opt & (1U << optargs++))) continue;
        ctor->args.push_back(a.first);
    }

    if (kind == NodeKind::Abstract) ctor->access = IrElement::Protected;
    ctor->inImpl = true;
    elements.push_back(ctor);
    return optargs;
}

Util::Enumerator<IrField *> *IrClass::getFields() const {
    return Util::enumerate(elements)->as<IrField *>()->where(
        [](IrField *f) { return f && !f->isStatic; });
}

Util::Enumerator<IrMethod *> *IrClass::getUserMethods() const {
    return Util::enumerate(elements)->as<IrMethod *>()->where(
        [](IrElement *e) { return e != nullptr; });
}

bool IrClass::shouldSkip(cstring feature) const {
    // skip if there is a 'no' directive
    bool explicitNo =
        Util::enumerate(elements)
            ->where([](IrElement *el) { return el->is<IrNo>(); })
            ->where([feature](IrElement *el) { return el->to<IrNo>()->text == feature; })
            ->any();
    if (explicitNo) return true;
    // also, skip if the user provided an implementation manually
    // (except for validate)
    if (feature == "validate") return false;

    bool provided = Util::enumerate(elements)
                        ->where([feature](IrElement *e) {
                            const auto *m = e->to<IrMethod>();
                            return m && m->name == feature;
                        })
                        ->any();
    return provided;
}

void IrClass::resolve() {
    for (auto s : parents) {
        const IrClass *p = s->resolve(containedIn);
        if (p == nullptr) throw Util::CompilationError("Could not find class %1%", s);
        if (p->kind != NodeKind::Interface) {
            if (concreteParent == nullptr)
                concreteParent = p;
            else
                BUG("Class %1% has more than 1 non-interface parent: %2% and %3%", this,
                    concreteParent, p);
        }
        parentClasses.push_back(p);
    }
    generateMethods();
    for (auto e : elements) e->resolve();
}

////////////////////////////////////////////////////////////////////////////////////
//
void IrEnumType::generate_hdr(std::ostream &out) const {
    out << "enum " << (isClassEnum ? "class " : "") << name << "\n"
        << LineDirective(srcInfo, +1) << body << ";\n"
        << LineDirective();
}

////////////////////////////////////////////////////////////////////////////////////

void IrField::generate(std::ostream &out, bool asField) const {
    if (asField) {
        out << IrClass::indent;
        if (isStatic) out << "static ";
        if (isConst) out << "const ";
    }

    auto tmpl = dynamic_cast<const TemplateInstantiation *>(type);
    const IrClass *cls = type->resolve(clss ? clss->containedIn : nullptr);
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
                    throw Util::CompilationError(
                        "%1% template argment %2% is not "
                        "an IR class",
                        cls->name, tmpl->args[i]);
                }
            }
        } else if (cls->kind == NodeKind::Template) {
            throw Util::CompilationError("No args for template %1%", cls);
        }
    }
    if (cls != nullptr && !isInline) out << "const ";
    out << type->toString();
    if (cls != nullptr && !isInline) out << "*";
    out << " " << name << type->declSuffix();
    if (asField) {
        if (!isStatic) {
            if (!initializer.isNullOrEmpty())
                out << " = " << initializer;
            else if (cls != nullptr && !isInline)
                out << " = nullptr";
        }
        out << ";";
        out << std::endl;
    }
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
