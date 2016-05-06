#include "irclass.h"
#include "lib/exceptions.h"
#include "lib/enumerator.h"

const char* IrClass::indent = "    ";
IrNamespace IrNamespace::global(nullptr, cstring(0));
static const LookupScope utilScope(nullptr, "Util");
static const NamedType srcInfoType(Util::SourceInfo(), &utilScope, "SourceInfo");
IrField *IrField::srcInfoField = new IrField(Util::SourceInfo(), &srcInfoType,
                                             "srcInfo", nullptr, false, true);
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

    out << "namespace IR {" << std::endl;
    for (auto cls : *getClasses()) {
        for (auto p = cls->containedIn; p && p->name; p = p->parent)
            out << "namespace " << p->name << " {" << std::endl;
        cls->declare(out);
        for (auto p = cls->containedIn; p && p->name; p = p->parent)
            out << "}" << std::endl;
    }
    out << "}" << std::endl;

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
            //"D(Vector<IR::Node>) "
            "D(Node), ##__VA_ARGS__) \\" << std::endl;
    for (auto cls : *getClasses()) {
        if (cls->needVector || cls->needIndexedVector)
            // IndexedVector is a subclass of Vector, so we need Vector in both cases
            t << "T(Vector<IR::" << cls->containedIn << cls->name << ">, D(Node), "
                 "##__VA_ARGS__) \\" << std::endl;
        if (cls->needIndexedVector)
            t << "T(IndexedVector<IR::" << cls->containedIn << cls->name << ">, "
                    //"D(Vector<IR::" << cls->containedIn << cls->name << ">) "
                    "D(Node), ##__VA_ARGS__) \\" << std::endl;
        if (cls->needNameMap)
            BUG("visitable (non-inline) NameMap not yet implemented");
        if (cls->needNodeMap)
            BUG("visitable (non-inline) NodeMap not yet implemented"); }
    t << std::endl;
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
    cstring proto = getPrototype(name, clss->name);
    if (name == "visit_children") {
        out << IrClass::indent << proto << " override;" << std::endl;
        out << IrClass::indent << proto << " const override;" << std::endl;
    } else if (name == "dump_fields") {
        out << IrClass::indent << proto << " override;" << std::endl;
    } else {
        out << LineDirective(srcInfo) << IrClass::indent << proto << ' ' << body << std::endl;
        if (name == "node_type_name")
            out << LineDirective(srcInfo) << IrClass::indent
                << "static cstring static_type_name() " << body << std::endl;
        if (srcInfo.isValid())
            out << LineDirective(); }
}

void IrMethod::generate_impl(std::ostream &out) const {
    if (name == "visit_children") {
        out << LineDirective(srcInfo) << "void IR::" << clss->containedIn << clss->name
            << "::visit_children(Visitor &v) " << body << std::endl;
        out << LineDirective(srcInfo) << "void IR::" << clss->containedIn << clss->name
            << "::visit_children(Visitor &v) const " << body << std::endl;
        if (srcInfo.isValid())
            out << LineDirective();
    } else if (name == "dump_fields") {
        out << LineDirective(srcInfo)<< "void IR::" << clss->containedIn
            << clss->name << "::dump_fields(std::ostream& out) const"
            << body << std::endl;
        if (srcInfo.isValid())
            out << LineDirective();
    }
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
        out << p->name;
        first = false;
    }

    out << " {" << std::endl;
    out << " public:" << std::endl;

    for (auto e : elements)
        e->generate_hdr(out);

    if (kind != NodeKind::Interface) {
        out << indent << "IRNODE" << (kind == NodeKind::Abstract ?  "_ABSTRACT" : "")
            << "_SUBCLASS(" << name << ")" << std::endl;
        generateConstructor(out); }

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

void IrClass::generateMethods() {
    // equality test
    if (!shouldSkip("operator==")) {
        std::stringstream buf;
        buf << "{" << std::endl << indent << indent << "return ";
        if (getParent()->name == "Node")
            buf << "typeid(*this) == typeid(a)";
        else
            buf << getParent()->name << "::operator==(a)";
        for (auto f : *getFields()) {
            buf << std::endl;
            buf << indent << indent << "&& " << f->name << " == a." << f->name; }
        buf << ";" << std::endl;
        buf << indent << "}";
        elements.push_back(new IrMethod("operator==", buf.str())); }

    // visitors
    if (!shouldSkip("visit_children")) {
        bool needed = false;
        std::stringstream buf;
        buf << "{" << std::endl;
        buf << indent << getParent()->name << "::visit_children(v);" << std::endl;
        for (auto f : *getFields()) {
            if (f->type->resolve(containedIn) == nullptr)
                // This is not an IR pointer
                continue;
            if (f->isInline)
                buf << indent << f->name << ".visit_children(v);" << std::endl;
            else
                buf << indent << "v.visit(" << f->name << ", \"" << f->name << "\");" << std::endl;
            needed = true; }
        buf << "}";
        if (needed)
            elements.push_back(new IrMethod("visit_children", buf.str())); }

    // validate
    if (!shouldSkip("validate")) {
        auto it = std::find_if(elements.begin(), elements.end(), [](IrElement *el)->bool {
            auto *m = el->to<IrMethod>();
            return m && m->name == "validate"; });
        IrMethod *validateMethod = (it == elements.end()) ? nullptr : (*it)->to<IrMethod>();
        bool needed = false;
        std::stringstream buf;
        buf << "{";
        for (auto f : *getFields()) {
            if (f->type->resolve(containedIn) == nullptr)
                // This is not an IR pointer
                continue;
            if (f->isInline)
                buf << std::endl << indent << indent << f->name << ".validate();";
            else if (!f->nullOK)
                buf << std::endl << indent << indent << "CHECK_NULL(" << f->name << ");";
            else
                continue;
            needed = true; }
        if (validateMethod) buf << validateMethod->body;
        buf << " }";
        if (needed) {
            if (validateMethod)
                validateMethod->body = buf.str();
            else
                elements.push_back(new IrMethod("validate", buf.str())); } }

    if (!shouldSkip("node_type_name")) {
        std::stringstream buf;
        buf << " { return \"" << containedIn << name << "\"; }";
        elements.push_back(new IrMethod("node_type_name", buf.str())); }

    // dbprint declaration
    if (!shouldSkip("dbprint") && kind == NodeKind::Concrete)
        elements.push_back(new IrMethod("dbprint", ";"));

    if (!shouldSkip("dump_fields")) {
        std::stringstream buf;
        buf << "{" << std::endl;
        buf << indent << getParent()->name << "::dump_fields(out);" << std::endl;
        bool needed = false;
        for (auto f : *getFields()) {
            if (f->type->resolve(containedIn) == nullptr &&
                !dynamic_cast<const TemplateInstantiation*>(f->type)) {
                // not an IR pointer
                buf << indent << indent << "out << \" " << f->name << "=\" << " << f->name
                    << ";" << std::endl;
                needed = true; } }
        buf << "}";
        if (needed)
            elements.push_back(new IrMethod("dump_fields", buf.str()));
    }
}

void IrClass::computeConstructorArguments(IrClass::ctor_args_t &args) const {
    if (concreteParent == nullptr)
        // direct descendant of Node, add srcInfo
        args.emplace_back(IrField::srcInfoField, IrClass::nodeClass);
    else
        concreteParent->computeConstructorArguments(args);

    for (auto e : elements)
        if (e->is<IrField>())
            args.emplace_back(e->to<IrField>(), this);

    if (args.size() == 0)
        BUG("No constructor arguments?");
}

void IrClass::generateConstructor(std::ostream &out) const {
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

    if (shouldSkip("constructor")) return;
    if (kind != NodeKind::Concrete)
        out << " protected:" << std::endl;

    std::vector<std::pair<const IrField*, const IrClass*>> args;
    computeConstructorArguments(args);
    out << indent << name << "(";
    bool first = true;
    for (auto a : args) {
        if (!first)
            out << "," << std::endl << indent << indent;
        a.first->generate(out, false);
        first = false;
    }
    out << ") :" << std::endl;

    out << indent << indent;
    first = true;
    // First print parent arguments
    out << getParent()->name << "(";
    auto it = args.begin();
    for (; it != args.end(); ++it) {
        if (it->second == this)
            break;  // end of parent arguments

        if (!first)
            out << ", ";
        out << it->first->name;
        first = false;
    }

    out << ")";
    // Print my fields
    for (; it != args.end(); ++it) {
        out << "," << std::endl << indent << indent;
        out << it->first->name << "(" << it->first->name << ")";
    }

    out << std::endl;
    out << indent << "{ validate(); }" << std::endl;
    if (kind != NodeKind::Concrete)
        out << " public:" << std::endl;
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
    if (kind != NodeKind::Interface && kind != NodeKind::Template && this != nodeClass)
        generateMethods();
    for (auto e : elements)
        e->set_class(this);
}

////////////////////////////////////////////////////////////////////////////////////

cstring IrMethod::getPrototype(cstring name, cstring className) {
    if (name == "toString")
        return "cstring toString() const override";
    else if (name == "dbprint")
        return "void dbprint(std::ostream &out) const override";
    else if (name == "operator==")
        return "bool operator==(const " + className + "&a) const";
    else if (name == "validate")
        return "void validate() const override";
    else if (name == "node_type_name")
        return "cstring node_type_name() const override";
    else if (name == "visit_children")
        return "void visit_children(Visitor &v)";
    else if (name == "dump_fields")
        return "void dump_fields(std::ostream& out) const";
    else
        throw Util::CompilationError("%1%: Unknown method name %2%", className, name);
}

////////////////////////////////////////////////////////////////////////////////////

void IrField::generate(std::ostream &out, bool asField) const {
    if (asField)
        out << IrClass::indent;

    auto tmpl = dynamic_cast<const TemplateInstantiation *>(type);
    const IrClass* cls = type->resolve(clss->containedIn);
    if (cls) {
        if (tmpl) {
            if (cls->kind != NodeKind::Template)
                throw Util::CompilationError("Template args with non-template class %1%", cls);
            unsigned tmpl_args = (cls == IrClass::nodemapClass ? 2 : 1);
            if (tmpl->args.size() < tmpl_args)
                throw Util::CompilationError("Wrong number of args for template %1%", cls);
            for (unsigned i = 0; i < tmpl_args; i++) {
                if (auto acl = tmpl->args[i]->resolve(clss->containedIn)) {
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
            out << " = nullptr"; }
    if (asField) {
        out << ";";
        out << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////

void ConstFieldInitializer::generate_hdr(std::ostream &out) const {
    out << IrClass::indent;
    if (name == "precedence")
        out << "int getPrecedence() const { return ";
    else if (name == "stringOp")
        out << "cstring getStringOp() const { return ";
    else
        throw Util::CompilationError("Unexpected constant field %1%", this);
    out << initializer << "; }" << std::endl;
}
