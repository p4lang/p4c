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

#include "reservedWords.h"

namespace P4 {

// Keep this in sync with the lexer
std::set<cstring> reservedWords = {
    "abstract",  // experimental
    "action", "actions", "apply", "bool", "bit", "const", "control", "default",
    "else", "enum", "error", "exit", "extern", "false", "header", "header_union",
    "if", "in", "inout", "int", "key", "match_kind", "out", "parser", "package",
    "return", "select", "set", "state", "struct", "switch", "table", "this",  // experimental
    "transition", "true", "tuple", "typedef", "varbit", "verify", "void", "_",
    "NoAction"  // core.p4
};

}  // namespace P4
