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

#include "symbol_table.h"

#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"

namespace Util {

class NamedSymbol {
 protected:
    Util::SourceInfo sourceInfo;
    Namespace *parent;
    cstring name;

 public:
    NamedSymbol(cstring name, Util::SourceInfo si) : sourceInfo(si), parent(nullptr), name(name) {}
    virtual ~NamedSymbol() {}

    void setParent(Namespace *ns) {
        BUG_CHECK(parent == nullptr, "Parent already set");
        parent = ns;
    }

    Namespace *getParent() const { return parent; }
    SourceInfo getSourceInfo() const { return sourceInfo; }
    cstring getName() const { return name; }
    virtual cstring toString() const { return getName(); }
    virtual void dump(std::stringstream &into, unsigned indent) const {
        std::string s(indent, ' ');
        into << s << toString() << std::endl;
    }
    virtual bool sameType(const NamedSymbol *other) const {
        return typeid(*this) == typeid(*other);
    }
    virtual const Namespace *symNamespace() const;

    bool template_args = false;  // does the symbol expect template args
};

class Namespace : public NamedSymbol {
 protected:
    std::unordered_map<cstring, NamedSymbol *> contents;
    bool allowDuplicates;

 public:
    Namespace(cstring name, Util::SourceInfo si, bool allowDuplicates)
        : NamedSymbol(name, si), allowDuplicates(allowDuplicates) {}
    void declare(NamedSymbol *symbol) {
        cstring symname = symbol->getName();
        if (symname.isNullOrEmpty()) return;

        auto it = contents.find(symname);
        if (it != contents.end()) {
            // Check that both declarations have the same type
            if (!it->second->sameType(symbol)) {
                ::error(ErrorType::ERR_DUPLICATE,
                        "Re-declaration of %1%%2% with different type: %3%", symbol->getName(),
                        symbol->getSourceInfo(), it->second->getSourceInfo());
                return;
            }

            if (!allowDuplicates) {
                ::error(ErrorType::ERR_DUPLICATE,
                        "Duplicate declaration of %1%%2%; previous at %3%", symbol->getName(),
                        symbol->getSourceInfo(), it->second->getSourceInfo());
                return;
            }
        }
        contents.emplace(symbol->getName(), symbol);
    }
    NamedSymbol *lookup(cstring name) const {
        auto it = contents.find(name);
        if (it == contents.end()) return nullptr;
        return it->second;
    }
    cstring toString() const override { return cstring("Namespace ") + getName(); }
    void dump(std::stringstream &into, unsigned indent) const override {
        std::string s(indent, ' ');
        into << s;
        into << toString() << "{" << std::endl;
        for (auto it : contents) it.second->dump(into, indent + 1);
        into << s;
        into << "}" << std::endl;
    }
    void clear() { contents.clear(); }
    static const Namespace empty;
};

const Namespace Namespace::empty("<empty>", Util::SourceInfo(), false);
const Namespace *NamedSymbol::symNamespace() const { return &Namespace::empty; }

class Object : public NamedSymbol {
    const Namespace *typeNamespace = &Namespace::empty;

 public:
    Object(cstring name, Util::SourceInfo si) : NamedSymbol(name, si) {}
    cstring toString() const override { return cstring("Object ") + getName(); }
    const Namespace *symNamespace() const override { return typeNamespace; }
    void setNamespace(const Namespace *ns) { typeNamespace = ns; }
};

class SimpleType : public NamedSymbol {
 public:
    SimpleType(cstring name, Util::SourceInfo si) : NamedSymbol(name, si) {}
    cstring toString() const { return cstring("SimpleType ") + getName(); }
};

// A Type that is also a namespace (e.g., a parser)
class ContainerType : public Namespace {
 public:
    ContainerType(cstring name, Util::SourceInfo si, bool allowDuplicates)
        : Namespace(name, si, allowDuplicates) {}
    cstring toString() const { return cstring("ContainerType ") + getName(); }
};

/////////////////////////////////////////////////

ProgramStructure::ProgramStructure()
    : debug(false), debugStream(nullptr), rootNamespace(nullptr), currentNamespace(nullptr) {
    rootNamespace = new Namespace("", Util::SourceInfo(), true);
    currentNamespace = rootNamespace;
    // We use stderr because we want debugging output
    // to be the same as the bison debugging output.
    debugStream = stderr;
}

void ProgramStructure::push(Namespace *ns) {
    CHECK_NULL(ns);
    if (debug) fprintf(debugStream, "ProgramStructure: pushing %s\n", ns->toString().c_str());
    LOG4("ProgramStructure: pushing " << ns->toString());
    BUG_CHECK(currentNamespace != nullptr, "Null currentNamespace");
    currentNamespace->declare(ns);
    ns->setParent(currentNamespace);
    currentNamespace = ns;
}

void ProgramStructure::pushNamespace(SourceInfo si, bool allowDuplicates) {
    // Today we don't have named namespaces
    auto ns = new Util::Namespace("", si, allowDuplicates);
    push(ns);
}

void ProgramStructure::pushContainerType(IR::ID id, bool allowDuplicates) {
    auto ct = new Util::ContainerType(id.name, id.srcInfo, allowDuplicates);
    push(ct);
}

void ProgramStructure::pop() {
    Namespace *parent = currentNamespace->getParent();
    BUG_CHECK(parent != nullptr, "Popping root namespace");
    if (debug)
        fprintf(debugStream, "ProgramStructure: popping %s\n",
                currentNamespace->toString().c_str());
    LOG4("ProgramStructure: popping " << currentNamespace->toString());
    currentNamespace = parent;
}

void ProgramStructure::declareType(IR::ID id) {
    if (debug) fprintf(debugStream, "ProgramStructure: adding type %s\n", id.name.c_str());

    LOG3("ProgramStructure: adding type " << id);
    auto st = new SimpleType(id.name, id.srcInfo);
    currentNamespace->declare(st);
}

void ProgramStructure::declareObject(IR::ID id, cstring type) {
    if (debug) fprintf(debugStream, "ProgramStructure: adding object %s\n", id.name.c_str());

    LOG3("ProgramStructure: adding object " << id << " with type " << type);
    auto type_sym = lookup(type);
    auto o = new Object(id.name, id.srcInfo);
    if (auto tns = dynamic_cast<const Namespace *>(type_sym)) o->setNamespace(tns);
    currentNamespace->declare(o);
}

void ProgramStructure::markAsTemplate(IR::ID id) {
    LOG3("ProgramStructure: " << id << " has template args");
    lookup(id)->template_args = true;
}

void ProgramStructure::startAbsolutePath() { identifierContext.lookupContext = rootNamespace; }

void ProgramStructure::relativePathFromLastSymbol() {
    if (identifierContext.previousSymbol) {
        identifierContext.lookupContext = identifierContext.previousSymbol->symNamespace();
    } else {
        identifierContext.lookupContext = &Namespace::empty;
    }
}

void ProgramStructure::clearPath() {
    identifierContext.previousSymbol = nullptr;
    identifierContext.lookupContext = nullptr;
}

NamedSymbol *ProgramStructure::lookup(cstring identifier) {
    const Namespace *parent = identifierContext.lookupContext;
    NamedSymbol *rv = nullptr;
    if (parent == nullptr) {
        // We don't have a parent, try lookup up the stack
        parent = currentNamespace;
        while (parent != nullptr) {
            NamedSymbol *symbol = parent->lookup(identifier);
            if (symbol != nullptr) {
                rv = symbol;
                break;
            }
            parent = parent->getParent();
        }
    } else {
        rv = parent->lookup(identifier);
    }
    identifierContext.previousSymbol = rv;
    identifierContext.lookupContext = nullptr;
    return rv;
}

ProgramStructure::SymbolKind ProgramStructure::lookupIdentifier(cstring identifier) {
    NamedSymbol *ns = lookup(identifier);
    if (ns == nullptr || dynamic_cast<Object *>(ns) != nullptr) {
        LOG2("Identifier " << identifier);
        if (ns && ns->template_args) return ProgramStructure::SymbolKind::TemplateIdentifier;
        return ProgramStructure::SymbolKind::Identifier;
    }
    if (dynamic_cast<SimpleType *>(ns) != nullptr || dynamic_cast<ContainerType *>(ns) != nullptr) {
        if (ns && ns->template_args) return ProgramStructure::SymbolKind::TemplateType;
        return ProgramStructure::SymbolKind::Type;
        LOG2("Type " << identifier);
    }
    BUG("Should be unreachable");
}

void ProgramStructure::declareTypes(const IR::IndexedVector<IR::Type_Var> *typeVars) {
    if (typeVars == nullptr) return;
    for (auto tv : *typeVars) declareType(tv->name);
}

void ProgramStructure::declareParameters(const IR::IndexedVector<IR::Parameter> *params) {
    if (params == nullptr) return;
    for (auto param : *params) declareObject(param->name, param->type->toString());
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

void ProgramStructure::clear() {
    rootNamespace->clear();
    currentNamespace = rootNamespace;
    debugStream = stderr;
}
}  // namespace Util
