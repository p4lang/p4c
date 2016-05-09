#include "irclass.h"

enum flags { EXTEND = 1, IN_IMPL = 2, OVERRIDE = 4, NOT_DEFAULT = 8, CONCRETE_ONLY = 16 };

const ordered_map<cstring, IrMethod::info_t> IrMethod::Generate = {
{ "operator==", { "bool", "(const %s &a) const", 0,
    [](IrClass *cl, cstring) -> cstring {
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
{ "visit_children", { "void", "(Visitor &v)", IN_IMPL + OVERRIDE,
    [](IrClass *cl, cstring) -> cstring {
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
{ "validate", { "void", "() const", EXTEND + OVERRIDE,
    [](IrClass *cl, cstring body) -> cstring {
        bool needed = false;
        std::stringstream buf;
        buf << "{";
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
        if (body) buf << body;
        buf << " }";
        return needed ? buf.str() : cstring(); } } },
{ "node_type_name", { "cstring", "() const", OVERRIDE,
    [](IrClass *cl, cstring) -> cstring {
        std::stringstream buf;
        buf << " { return \"" << cl->containedIn << cl->name << "\"; }";
        return buf.str(); } } },
{ "dbprint", { "void", "(std::ostream &out) const", OVERRIDE + CONCRETE_ONLY,
    [](IrClass *, cstring) -> cstring {
        return ";"; } } },
{ "dump_fields", { "void", "(std::ostream &out) const", IN_IMPL + OVERRIDE,
    [](IrClass *cl, cstring) -> cstring {
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
{ "toString", { "cstring", "() const", OVERRIDE + NOT_DEFAULT,
    [](IrClass *, cstring) -> cstring { return cstring(); } } },
};

void IrClass::generateMethods() {
    if (kind != NodeKind::Interface && this != nodeClass && this != vectorClass) {
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
            cstring body = def.second.create(this, exist ? exist->body : cstring());
            if (body) {
                if (exist)
                    exist->body = body;
                else
                    elements.push_back(new IrMethod(def.first, body)); } }
        for (auto *parent = getParent(); parent; parent = parent->getParent()) {
            auto eq_overload = new IrMethod("operator==", "{ return a == *this; }");
            eq_overload->isOverride = true;
            eq_overload->rtype = "bool";
            eq_overload->args =
                Util::printf_format("(const IR::%s &a) const", parent->name.c_str());
            elements.push_back(eq_overload); } }
    for (auto m : *getUserMethods()) {
        if (m->rtype) continue;
        if (!IrMethod::Generate.count(m->name))
            throw Util::CompilationError("Unrecognized predefined method %1%", m);
        auto &info = IrMethod::Generate.at(m->name);
        m->rtype = info.rtype;
        m->args = Util::printf_format(info.args, name.c_str());
        if (info.flags & IN_IMPL)
            m->inImpl = true;
        if (info.flags & OVERRIDE)
            m->isOverride = true; }
}
