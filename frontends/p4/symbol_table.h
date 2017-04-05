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

/* -*-C++-*- */

#ifndef _P4_SYMBOL_TABLE_H_
#define _P4_SYMBOL_TABLE_H_

/* A very simple symbol table that recognizes types; necessary because
   the v1.2 grammar is ambiguous without type information */

#include <unordered_map>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

namespace Util {
class NamedSymbol;
class Namespace;

// Maintains a high-level representation of the program structure
// built on the fly during parsing.  Used by the lexer to
// disambiguate identifiers.
class ProgramStructure final {
 private:
    bool debug;
    FILE* debugStream;
    Namespace* rootNamespace;
    Namespace* currentNamespace;

    struct PathContext {
        Namespace* lookupContext;
        PathContext() : lookupContext(nullptr) {}
    } identifierContext;

    void push(Namespace* ns);
    NamedSymbol* lookup(const cstring identifier) const;
    void declare(NamedSymbol* symbol);

 public:
    enum class SymbolKind {
        Identifier,
        Type
    };

    ProgramStructure();

    void setDebug(bool debug) { this->debug = debug; }
    void pushNamespace(SourceInfo info, bool allowDuplicates);
    void pushContainerType(IR::ID id, bool allowDuplicates);
    void declareType(IR::ID id);
    void declareObject(IR::ID id);

    // the last namespace has been exited
    void pop();
    // Declares these types in the current scope
    void declareTypes(const IR::IndexedVector<IR::Type_Var>* typeVars);
    SymbolKind lookupIdentifier(cstring identifier) const;

    void startAbsolutePath();
    void clearPath();

    void endParse();

    cstring toString() const;
    void clear();
};

}  // namespace Util
#endif /* _P4_SYMBOL_TABLE_H_ */
