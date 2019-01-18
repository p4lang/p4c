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
#include "lib/algorithm.h"

enum flags {
    // flags that control the creation of auto-created methods
    EXTEND = 1,          // call the creation function to extend even when user-defined
    IN_IMPL = 2,         // output in the implementation file, not the header
    OVERRIDE = 4,        // give an 'override' tag (overrides something in IR::Node)
    NOT_DEFAULT = 8,     // don't create if not user-defined
    CONCRETE_ONLY = 16,  // don't create in abstract classes
    CONST = 32,          // give a 'const' tag
    CLASSREF = 64,       // add an argument `const CLASS &a'
    INCL_NESTED = 128,   // create even in nested (non-Node sublass) classes
    CONSTRUCTOR = 256,   // is a constructor
    FACTORY = 512,       // factory (static) method
    FRIEND = 1024        // friend function, not a method
};

const ordered_map<cstring, IrMethod::info_t> IrMethod::Generate = {
{ "operator==", { &NamedType::Bool(), {}, CONST + IN_IMPL + INCL_NESTED + OVERRIDE + CLASSREF,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl << cl->indent << cl->indent << "return ";
        bool first = true;
        if (auto parent = cl->getParent()) {
            if (parent->name == "Node")
                buf << "typeid(*this) == typeid(a)";
            else
                buf << parent->name << "::operator==(static_cast<const "
                    << parent->name << " &>(a))";
            first = false; }
        for (auto f : *cl->getFields()) {
            if (*f->type == NamedType::SourceInfo()) continue;  // FIXME -- deal with SourcInfo
            if (!first)
                buf << std::endl << cl->indent << cl->indent << "&& ";
            first = false;
            buf << f->name << " == a." << f->name; }
        if (first) {  // a nested class with no fields?
            buf << "false"; }
        buf << ";" << std::endl;
        buf << cl->indent << "}";
        return buf.str(); } } },
{ "equiv", { &NamedType::Bool(),
             { new IrField(new ReferenceType(new NamedType(IrClass::nodeClass()), true), "a_") },
             EXTEND + CONST + IN_IMPL + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo srcInfo, cstring body) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl;
        buf << cl->indent << cl->indent
            << "if (static_cast<const Node *>(this) == &a_) return true;\n";
        if (auto parent = cl->getParent()) {
            if (parent->name == "Node") {
                buf << cl->indent << cl->indent << "if (typeid(*this) != typeid(a_)) "
                                                   "return false;\n";
            } else {
                buf << cl->indent << cl->indent << "if (!" << parent->name
                    << "::equiv(a_)) return false;\n"; } }
        if (body) {
            buf << cl->indent << cl->indent << "auto &a = static_cast<const " << cl->name
                                            << " &>(a_);\n";
            buf << LineDirective(srcInfo, true) << body;
        } else {
            bool first = true;
            for (auto f : *cl->getFields()) {
                if (*f->type == NamedType::SourceInfo()) continue;  // FIXME -- deal with SourcInfo
                if (first) {
                    buf << cl->indent << cl->indent << "auto &a = static_cast<const " << cl->name
                                                    << " &>(a_);\n";
                    buf << cl->indent << cl->indent << "return ";
                    first = false;
                } else {
                    buf << std::endl << cl->indent << cl->indent << "&& "; }
                if (f->type->resolve(cl->containedIn) == nullptr) {
                    // This is not an IR pointer
                    buf << f->name << " == a." << f->name;
                } else if (f->isInline) {
                    buf << f->name << ".equiv(a." << f->name << ")";
                } else {
                    buf << "(" << f->name << " ? a." << f->name << " ? "
                        << f->name << "->equiv(*a." << f->name << ")"
                        << " : false : a." << f->name << " == nullptr)"; } }
            if (first) {  // no fields?
                buf << cl->indent << cl->indent << "return true"; }
            buf << ";" << std::endl; }
        buf << cl->indent << "}";
        return buf.str(); } } },
{ "operator<<", { &ReferenceType::OstreamRef, { new IrField(&ReferenceType::OstreamRef, "out") },
  EXTEND + IN_IMPL + NOT_DEFAULT + INCL_NESTED + CLASSREF + FRIEND,
    [](IrClass *cl, Util::SourceInfo srcInfo, cstring body) -> cstring {
        std::stringstream buf;
        buf << "{";
        if (body) buf << LineDirective(srcInfo, true) << body;
        buf << LineDirective(true) << cl->indent << "return out; }";
        return buf.str(); } } },
{ "visit_children", { &NamedType::Void(), { new IrField(&ReferenceType::VisitorRef, "v") },
  IN_IMPL + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        bool needed = false;
        std::stringstream buf;
        buf << "{" << std::endl;
        if (auto parent = cl->getParent())
            buf << cl->indent << parent->name << "::visit_children(v);" << std::endl;
        for (auto f : *cl->getFields()) {
            if (f->type->resolve(cl->containedIn) == nullptr)
                // This is not an IR pointer
                continue;
            if (f->isInline)
                buf << cl->indent << f->name << ".visit_children(v);" << std::endl;
            else
                buf << cl->indent << "v.visit(" << f->name << ", \"" << f->name << "\");"
                    << std::endl;
            needed = true; }
        buf << "}";
        return needed ? buf.str() : cstring(); } } },
{ "validate", { &NamedType::Void(), {}, CONST + IN_IMPL + EXTEND + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo srcInfo, cstring body) -> cstring {
        bool needed = false;
        std::stringstream buf;
        buf << "{" << LineDirective(true);
        for (auto f : *cl->getFields()) {
            if (f->type->resolve(cl->containedIn) == nullptr)
                // This is not an IR pointer
                continue;
            if (f->isInline)
                buf << std::endl << cl->indent << cl->indent << f->name << ".validate();";
            else if (!f->nullOK)
                buf << std::endl << cl->indent << cl->indent << "CHECK_NULL(" << f->name << ");";
            else
                continue;
            needed = true; }
        if (body) {
            buf << LineDirective(srcInfo, true) << body;
            needed = true; }
        buf << " }";
        return needed ? buf.str() : cstring(); } } },
{ "node_type_name", { &NamedType::Cstring(), {}, CONST + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{ return \"" << cl->containedIn << cl->name << "\"; }";
        return buf.str(); } } },
{ "dbprint", { &NamedType::Void(), { new IrField(&ReferenceType::OstreamRef, "out") },
  CONST + IN_IMPL + OVERRIDE + CONCRETE_ONLY,
    [](IrClass *, Util::SourceInfo, cstring) -> cstring {
        return ""; } } },
{ "dump_fields", { &NamedType::Void(), { new IrField(&ReferenceType::OstreamRef, "out") },
  CONST + IN_IMPL + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl;
        if (auto parent = cl->getParent())
            buf << cl->indent << parent->name << "::dump_fields(out);" << std::endl;
        bool needed = false;
        for (auto f : *cl->getFields()) {
            if (*f->type == NamedType::SourceInfo()) continue;  // FIXME -- deal with SourcInfo
            if (f->type->resolve(cl->containedIn) == nullptr &&
                !dynamic_cast<const TemplateInstantiation*>(f->type)) {
                // not an IR pointer
                buf << cl->indent << cl->indent << "out << \" " << f->name << "=\" << " << f->name
                    << ";" << std::endl;
                needed = true; } }
        buf << "}";
        return needed ? buf.str() : cstring(); } } },
{ "toJSON", { &NamedType::Void(), {
        new IrField(new ReferenceType(&NamedType::JSONGenerator()), "json")
    }, CONST + IN_IMPL + OVERRIDE + INCL_NESTED,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl;
        if (auto parent = cl->getParent())
            buf << cl->indent << parent->name << "::toJSON(json);" << std::endl;
        for (auto f : *cl->getFields()) {
            if (*f->type == NamedType::SourceInfo()) continue;  // FIXME -- deal with SourcInfo
            if (!f->isInline && f->nullOK)
                buf << cl->indent << "if (" << f->name << " != nullptr) ";
            buf << cl->indent << "json << \",\" << std::endl << json.indent << \"\\\""
                << f->name << "\\\" : \" << " << "this->" << f->name << ";" << std::endl; }
        buf << "}";
        return buf.str(); } } },
{ nullptr, { nullptr, { new IrField(new ReferenceType(&NamedType::JSONLoader()), "json")
    }, IN_IMPL + CONSTRUCTOR + INCL_NESTED,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        if (auto parent = cl->getParent())
            buf << ": " << parent->name << "(json)";
        buf << " {" << std::endl;
        for (auto f : *cl->getFields()) {
            if (*f->type == NamedType::SourceInfo()) continue;  // FIXME -- deal with SourcInfo
            buf << cl->indent << "json.load(\"" << f->name << "\", " << f->name << ");"
                << std::endl; }
        buf << "}";
        return buf.str(); } } },
{ "fromJSON", { nullptr, {
        new IrField(new ReferenceType(&NamedType::JSONLoader()), "json"),
    }, FACTORY + IN_IMPL + CONCRETE_ONLY + INCL_NESTED,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{ return new " << cl->name << "(json); }";
        return buf.str();
    } } },
{ "toString", { &NamedType::Cstring(), {}, CONST + IN_IMPL + OVERRIDE + NOT_DEFAULT,
    [](IrClass *, Util::SourceInfo, cstring) -> cstring { return cstring(); } } },
};

void IrClass::generateMethods() {
    if (this == nodeClass() || this == vectorClass()) return;
    if (kind != NodeKind::Interface) {
        for (auto &def : IrMethod::Generate) {
            if (def.second.flags & NOT_DEFAULT)
                continue;
            if (kind == NodeKind::Nested && !(def.second.flags & INCL_NESTED))
                continue;
            if ((def.second.flags & CONCRETE_ONLY) && kind == NodeKind::Abstract)
                continue;
            if (Util::Enumerator<IrElement*>::createEnumerator(elements)
                ->where([] (IrElement *el) { return el->is<IrNo>(); })
                ->where([&def] (IrElement *el) { return el->to<IrNo>()->text == def.first; })
                ->any())
                continue;
            IrMethod *exist = dynamic_cast<IrMethod *>(
                Util::Enumerator<IrElement*>::createEnumerator(elements)
                ->where([] (IrElement *el) { return el->is<IrMethod>(); })
                ->where([&def] (IrElement *el) { return el->to<IrMethod>()->name == def.first; })
                ->nextOrDefault());
            if (exist && !(def.second.flags & EXTEND))
                continue;
            cstring body;
            if (exist)
                body = def.second.create(this, exist->srcInfo, exist->body);
            else
                body = def.second.create(this, Util::SourceInfo(), cstring());
            if (body) {
                if (!body.size())
                    body = nullptr;
                if (exist) {
                    exist->body = body;
                    if (def.second.flags & FRIEND)
                        exist->isFriend = true;
                } else {
                    auto *m = new IrMethod(def.first, body);
                    if (def.second.flags & FRIEND)
                        m->isFriend = true;
                    m->clss = this;
                    elements.push_back(m); } } }
        for (auto *parent = getParent(); parent; parent = parent->getParent()) {
            auto eq_overload = new IrMethod("operator==", "{ return a == *this; }");
            eq_overload->clss = this;
            eq_overload->isOverride = true;
            eq_overload->rtype = &NamedType::Bool();
            eq_overload->args.push_back(
                new IrField(new ReferenceType(new NamedType(parent), true), "a"));
            eq_overload->isConst = true;
            elements.push_back(eq_overload); } }
    IrMethod *ctor = nullptr;
    bool user_defined_default_ctor = false;
    for (auto m : *getUserMethods()) {
        if (m->rtype) {
            m->rtype->resolve(&local);
            continue; }
        if (!m->args.empty())
            continue;
        if (m->name == name) {
            if (!m->isUser)
                ctor = m;
            else
                user_defined_default_ctor = true;
            continue; }
        if (!IrMethod::Generate.count(m->name))
            throw Util::CompilationError("Unrecognized predefined method %1%", m->name);
        auto &info = IrMethod::Generate.at(m->name);
        if (m->name) {
            if (info.rtype) {
                // This predefined method has an explicit return type.
                m->rtype = info.rtype;
            } else if ((info.flags & FACTORY) && kind != NodeKind::Nested) {
                // This is a factory method. These return an IR:Node*. The
                // exception is nested classes, which typically aren't IR::Nodes
                // and therefore just return a pointer to their concrete type.
                m->rtype = new PointerType(new NamedType(IrClass::nodeClass()));
            } else {
                // By default predefined methods return a pointer to their
                // concrete type.
                m->rtype = new PointerType(new NamedType(this)); }
            if (m->isUser && (info.flags & EXTEND))
                m->body = info.create(this, m->srcInfo, m->body);
            if (info.flags & FRIEND)
                m->isFriend = true;
        } else {
            m->name = name; }
        m->args = info.args;
        if (info.flags & CLASSREF)
            m->args.push_back(
                new IrField(new ReferenceType(new NamedType(this), true), "a"));
        if (info.flags & IN_IMPL)
            m->inImpl = true;
        if (info.flags & CONST)
            m->isConst = true;
        if ((info.flags & OVERRIDE) && kind != NodeKind::Nested)
            m->isOverride = true;
        if (info.flags & FACTORY)
            m->isStatic = true; }
    if (ctor) elements.erase(find(elements, ctor));
    if (kind != NodeKind::Interface && !shouldSkip("constructor")) {
        ctor_args_t args;
        computeConstructorArguments(args);
        if (!user_defined_default_ctor || !args.empty()) {
            int optargs = generateConstructor(args, ctor, 0);
            for (unsigned skip = 1; skip < (1U << optargs); ++skip)
                generateConstructor(args, ctor, skip); } }
}
