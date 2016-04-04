#include "irclass.h"

IrNamespace *LookupScope::resolve(const IrNamespace *in) const {
    if (global)
        return &IrNamespace::global;
    if (!in) in = &IrNamespace::global;
    if (this->in) {
        in = this->in->resolve(in);
        return in->lookupChild(name); }
    while (in) {
        if (auto *found = in->lookupChild(name))
            return found;
        in = in->parent; }
    if (name == "IR")
        return &IrNamespace::global;
    return nullptr;
}

IrClass *NamedType::resolve(const IrNamespace *in) const {
    if (resolved) return resolved;
    if (!in) in = LookupScope().resolve(in);
    if (lookup) {
        in = lookup->resolve(in);
        if (!in) return nullptr;
        return (resolved = in->lookupClass(name)); }
    while (in) {
        if (auto *found = in->lookupClass(name))
            return (resolved = found);
        in = in->parent; }
    return nullptr;
}

cstring TemplateInstantiation::toString() const {
    std::string rv = base->toString().c_str();
    rv += '<';
    const char *sep = "";
    for (auto arg : args) {
        rv += sep;
        if (arg->isResolved() && !base->isResolved())
            rv += "const ";
        rv += arg->toString().c_str();
        if (arg->isResolved() && !base->isResolved())
            rv += " *";
        sep = ", "; }
    rv += '>';
    return rv;
}
