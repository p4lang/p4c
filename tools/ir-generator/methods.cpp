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

enum flags { EXTEND = 1, IN_IMPL = 2, OVERRIDE = 4, NOT_DEFAULT = 8, CONCRETE_ONLY = 16,
             CONST = 32, CLASSREF = 64, INCL_NESTED = 128, CONSTRUCTOR = 256, FACTORY = 512};

const ordered_map<cstring, IrMethod::info_t> IrMethod::Generate = {
{ "operator==", { &NamedType::Bool, {}, CONST + IN_IMPL + OVERRIDE + CLASSREF,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl << cl->indent << cl->indent << "return ";
        if (cl->getParent()->name == "Node")
            buf << "typeid(*this) == typeid(a)";
        else
            buf << cl->getParent()->name << "::operator==(static_cast<const "
                << cl->getParent()->name << " &>(a))";
        for (auto f : *cl->getFields()) {
            buf << std::endl;
            buf << cl->indent << cl->indent << "&& " << f->name << " == a." << f->name; }
        buf << ";" << std::endl;
        buf << cl->indent << "}";
        return buf.str(); } } },
{ "visit_children", { &NamedType::Void, { new IrField(&ReferenceType::VisitorRef, "v") },
  IN_IMPL + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        bool needed = false;
        std::stringstream buf;
        buf << "{" << std::endl;
        buf << cl->indent << cl->getParent()->name << "::visit_children(v);" << std::endl;
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
{ "validate", { &NamedType::Void, {}, CONST + IN_IMPL + EXTEND + OVERRIDE,
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
        if (body) buf << LineDirective(srcInfo, true) << body;
        buf << " }";
        return needed ? buf.str() : cstring(); } } },
{ "node_type_name", { &NamedType::Cstring, {}, CONST + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{ return \"" << cl->containedIn << cl->name << "\"; }";
        return buf.str(); } } },
{ "dbprint", { &NamedType::Void, { new IrField(&ReferenceType::OstreamRef, "out") },
  CONST + IN_IMPL + OVERRIDE + CONCRETE_ONLY,
    [](IrClass *, Util::SourceInfo, cstring) -> cstring {
        return ""; } } },
{ "dump_fields", { &NamedType::Void, { new IrField(&ReferenceType::OstreamRef, "out") },
  CONST + IN_IMPL + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl;
        buf << cl->indent << cl->getParent()->name << "::dump_fields(out);" << std::endl;
        bool needed = false;
        for (auto f : *cl->getFields()) {
            if (f->type->resolve(cl->containedIn) == nullptr &&
                !dynamic_cast<const TemplateInstantiation*>(f->type)) {
                // not an IR pointer
                buf << cl->indent << cl->indent << "out << \" " << f->name << "=\" << " << f->name
                    << ";" << std::endl;
                needed = true; } }
        buf << "}";
        return needed ? buf.str() : cstring(); } } },
{ "toJSON", { &NamedType::Void, {
        new IrField(new ReferenceType(&NamedType::JSONGenerator), "json")
    }, CONST + IN_IMPL + OVERRIDE,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{" << std::endl
            << cl->indent << cl->getParent()->name << "::toJSON(json);" << std::endl;
        for (auto f : *cl->getFields()) {
            if (!f->isInline && f->nullOK)
                buf << cl->indent << "if (" << f->name << " != nullptr) ";
            buf << cl->indent << "json << \",\" << std::endl << json.indent << \"\\\""
                << f->name << "\\\" : \" << " << "this->" << f->name << ";" << std::endl; }
        buf << "}";
        return buf.str(); } } },
{ nullptr, { nullptr, { new IrField(new ReferenceType(&NamedType::JSONLoader), "json")
    }, IN_IMPL + CONSTRUCTOR,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << ": " << cl->getParent()->name << "(json) {" << std::endl;
        for (auto f : *cl->getFields()) {
            buf << cl->indent << "json.load(\"" << f->name << "\", " << f->name << ");"
                << std::endl; }
        buf << "}";
        return buf.str(); } } },
{ "fromJSON", { nullptr, {
        new IrField(new ReferenceType(&NamedType::JSONLoader), "json"),
    }, FACTORY + IN_IMPL + CONCRETE_ONLY,
    [](IrClass *cl, Util::SourceInfo, cstring) -> cstring {
        std::stringstream buf;
        buf << "{ return new " << cl->name << "(json); }";
        return buf.str();
    } } },
{ "toString", { &NamedType::Cstring, {}, CONST + IN_IMPL + OVERRIDE + NOT_DEFAULT,
    [](IrClass *, Util::SourceInfo, cstring) -> cstring { return cstring(); } } },
};

void IrClass::generateMethods() {
    if (this == nodeClass || this == vectorClass) return;
    if (kind != NodeKind::Interface && kind != NodeKind::Nested) {
        // FIXME -- some methods should be auto-generated for Nested classes
        for (auto &def : IrMethod::Generate) {
            if (def.second.flags & NOT_DEFAULT)
                continue;
            if ((def.second.flags & CONCRETE_ONLY) && kind != NodeKind::Concrete)
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
                } else {
                    auto *m = new IrMethod(def.first, body);
                    m->clss = this;
                    elements.push_back(m); } } }
        for (auto *parent = getParent(); parent; parent = parent->getParent()) {
            auto eq_overload = new IrMethod("operator==", "{ return a == *this; }");
            eq_overload->clss = this;
            eq_overload->isOverride = true;
            eq_overload->rtype = &NamedType::Bool;
            eq_overload->args.push_back(
                new IrField(new ReferenceType(new NamedType(parent), true), "a"));
            eq_overload->isConst = true;
            elements.push_back(eq_overload); } }
    IrMethod *ctor = nullptr;
    for (auto m : *getUserMethods()) {
        if (m->rtype) {
            m->rtype->resolve(&local);
            continue; }
        if (!m->args.empty())
            continue;
        if (m->name == name) {
            if (!m->isUser) ctor = m;
            continue; }
        if (!IrMethod::Generate.count(m->name))
            throw Util::CompilationError("Unrecognized predefined method %1%", m);
        auto &info = IrMethod::Generate.at(m->name);
        if (m->name) {
            if (info.rtype) {
                // This predefined method has an explicit return type.
                m->rtype = info.rtype;
            } else if (info.flags & FACTORY) {
                // This is a factory method. These return an IR:Node*. The
                // exception is nested classes, which typically aren't IR::Nodes
                // and therefore just return a pointer to their concrete type.
                m->rtype = kind == NodeKind::Nested
                         ? new PointerType(new NamedType(this))
                         : new PointerType(new NamedType(IrClass::nodeClass));
            } else {
                // By default predefined methods return a pointer to their
                // concrete type.
                m->rtype = new PointerType(new NamedType(this)); } }
        else
            m->name = name;
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
        if (!args.empty()) {
            int optargs = generateConstructor(args, ctor, 0);
            for (unsigned skip = 1; skip < (1U << optargs); ++skip)
                generateConstructor(args, ctor, skip); } }
}
