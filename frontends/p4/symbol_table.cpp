#include <sstream>

#include "symbol_table.h"
#include "lib/exceptions.h"
#include "lib/error.h"
#include "lib/log.h"

namespace Util {

class NamedSymbol {
 protected:
    Util::SourceInfo sourceInfo;
    Namespace* parent;
    cstring name;

 public:
    NamedSymbol(cstring name, Util::SourceInfo si) :
            sourceInfo(si),
            parent(nullptr),
            name(name) {}
    virtual ~NamedSymbol() {}

    void setParent(Namespace* ns) {
        BUG_CHECK(parent == nullptr, "Parent already set");
        parent = ns;
    }

    Namespace* getParent() const { return parent; }
    SourceInfo getSourceInfo() const { return sourceInfo; }
    cstring getName() const { return name; }
    virtual cstring toString() const { return getName(); }
    virtual void dump(std::stringstream& into, unsigned indent) const = 0;
    virtual bool sameType(const NamedSymbol* other) const {
        return typeid(*this) == typeid(*other);
    }
};

class Namespace : public NamedSymbol {
 protected:
    std::unordered_map<cstring, NamedSymbol*> contents;
    bool allowDuplicates;

 public:
    Namespace(cstring name, Util::SourceInfo si, bool allowDuplicates) :
            NamedSymbol(name, si),
            allowDuplicates(allowDuplicates) {}
    void declare(NamedSymbol* symbol) {
        cstring symname = symbol->getName();
        if (symname.isNullOrEmpty()) return;

        auto it = contents.find(symname);
        if (it != contents.end()) {
            // Check that both declarations have the same type
            if (!it->second->sameType(symbol)) {
                ::error("Re-declaration of %1%%2% with different type: %3%",
                        symbol->getName(), symbol->getSourceInfo(), it->second->getSourceInfo());
                return;
            }

            if (!allowDuplicates) {
                ::error("Duplicate declaration of %1%%2%; previous at %3%",
                        symbol->getName(), symbol->getSourceInfo(), it->second->getSourceInfo());
                return;
            }
        }
        contents.emplace(symbol->getName(), symbol);
    }
    NamedSymbol* lookup(cstring name) const {
        auto it = contents.find(name);
        if (it == contents.end())
            return nullptr;
        return it->second;
    }
    cstring toString() const { return cstring("Namespace ") + getName(); }
    void dump(std::stringstream& into, unsigned indent) const {
        std::string s(indent, ' ');
        into << s;
        into << toString() << "{" << std::endl;
        for (auto it : contents)
            it.second->dump(into, indent+1);
        into << s;
        into << "}" << std::endl;
    }
};

class SimpleType : public NamedSymbol {
 public:
    SimpleType(cstring name, Util::SourceInfo si) : NamedSymbol(name, si) {}
    cstring toString() const { return cstring("SimpleType ") + getName(); }
    void dump(std::stringstream& into, unsigned indent) const {
        std::string s(indent, ' ');
        into << s << toString() << std::endl;
    }
};

// A Type that is also a namespace (e.g., a parser)
class ContainerType : public Namespace {
 public:
    ContainerType(cstring name, Util::SourceInfo si, bool allowDuplicates) :
            Namespace(name, si, allowDuplicates)
    {}
    cstring toString() const { return cstring("ContainerType ") + getName(); }
};

/////////////////////////////////////////////////

ProgramStructure::ProgramStructure()
        : debug(false),
          debugStream(nullptr),
          rootNamespace(nullptr),
          currentNamespace(nullptr) {
    rootNamespace = new Namespace("", Util::SourceInfo(), false);
    currentNamespace = rootNamespace;
    // We use stderr because debugging because we want debugging output
    // to be the same as the bison debugging output.
    debugStream = stderr;
}

void ProgramStructure::push(Namespace* ns) {
    CHECK_NULL(ns);
    if (debug)
        fprintf(debugStream, "ProgramStructure: pushing %s\n", ns->toString().c_str());
    BUG_CHECK(currentNamespace != nullptr, "Null currentNamespace");
    currentNamespace->declare(ns);
    ns->setParent(currentNamespace);
    currentNamespace = ns;
}

void ProgramStructure::pushNamespace(cstring name, SourceInfo si, bool allowDuplicates) {
    auto ns = new Util::Namespace(name, si, allowDuplicates);
    push(ns);
}

void ProgramStructure::pushContainerType(IR::ID id, bool allowDuplicates) {
    auto ct = new Util::ContainerType(id.name, id.srcInfo, allowDuplicates);
    push(ct);
}

void ProgramStructure::pop() {
    Namespace* parent = currentNamespace->getParent();
    BUG_CHECK(parent != nullptr, "Popping root namespace");
    if (debug)
        fprintf(debugStream, "ProgramStructure: popping %s\n",
                currentNamespace->toString().c_str());
    currentNamespace = parent;
}

void ProgramStructure::declareType(IR::ID id) {
    if (debug)
        fprintf(debugStream, "ProgramStructure: adding type %s\n", id.name.c_str());

    auto st = new SimpleType(id.name, id.srcInfo);
    currentNamespace->declare(st);
}

void ProgramStructure::startAbsolutePath() {
    identifierContext.components.clear();
    identifierContext.lookupContext = rootNamespace;
}

void ProgramStructure::startRelativePath() {
    identifierContext.components.clear();
    identifierContext.lookupContext = nullptr;
}

void ProgramStructure::clearPath() {
    // The implementation is the same
    startRelativePath();
}

void ProgramStructure::pathAppendNamespace(cstring ns) {
    NamedSymbol* current = lookup(ns);
    BUG_CHECK(current != nullptr, "Null namespace");
    Namespace* nsp = dynamic_cast<Namespace*>(current);
    BUG_CHECK(nsp != nullptr, "Expected a namespace, got %s%", current->toString());
    identifierContext.components.push_back(ns);
    identifierContext.lookupContext = nsp;
}

NamedSymbol* ProgramStructure::lookup(cstring identifier) const {
    Namespace* parent = identifierContext.lookupContext;
    if (parent == nullptr) {
        // We don't have a parent, try lookup up the stack
        parent = currentNamespace;
        while (parent != nullptr) {
            NamedSymbol* symbol = parent->lookup(identifier);
            if (symbol != nullptr)
                return symbol;
            parent = parent->getParent();
        }
    }

    if (parent == nullptr)
        return nullptr;
    return parent->lookup(identifier);
}

ProgramStructure::SymbolKind ProgramStructure::lookupIdentifier(cstring identifier) const {
    NamedSymbol* ns = lookup(identifier);
    if (ns == nullptr) {
        LOG2("Identifier " << identifier);
        return ProgramStructure::SymbolKind::Identifier;
    }
    if (dynamic_cast<SimpleType*>(ns) != nullptr || dynamic_cast<ContainerType*>(ns) != nullptr) {
        return ProgramStructure::SymbolKind::Type;
        LOG2("Type " << identifier);
    }
    LOG2("Namespace " << identifier);
    return ProgramStructure::SymbolKind::Namespace;
}

void ProgramStructure::declareTypes(const IR::IndexedVector<IR::Type_Var>* typeVars) {
    if (typeVars == nullptr)
        return;
    for (auto tv : *typeVars)
        declareType(IR::ID(tv->srcInfo, tv->name));
}

void ProgramStructure::endParse() {
    BUG_CHECK(currentNamespace == rootNamespace,
              "Namespace stack is not empty at the end of parsing");
}

cstring ProgramStructure::toString() const {
    std::stringstream res;
    rootNamespace->dump(res, 0);
    return cstring(res.str());
}
}  // namespace Util
