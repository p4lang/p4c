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

namespace p4c::P4 {

using namespace literals;

// Keep this in sync with the lexer
const std::set<cstring> reservedWords = {
    "abstract"_cs,  // experimental
    "action"_cs,     "actions"_cs, "apply"_cs,  "bool"_cs,         "bit"_cs,    "const"_cs,
    "control"_cs,    "default"_cs, "else"_cs,   "enum"_cs,         "error"_cs,  "exit"_cs,
    "extern"_cs,     "false"_cs,   "header"_cs, "header_union"_cs, "if"_cs,     "in"_cs,
    "inout"_cs,      "int"_cs,     "key"_cs,    "match_kind"_cs,   "out"_cs,    "parser"_cs,
    "package"_cs,    "return"_cs,  "select"_cs, "set"_cs,          "state"_cs,  "struct"_cs,
    "switch"_cs,     "table"_cs,
    "this"_cs,  // experimental
    "transition"_cs, "true"_cs,    "tuple"_cs,  "typedef"_cs,      "varbit"_cs, "verify"_cs,
    "void"_cs,       "_"_cs,
    "NoAction"_cs  // core.p4
};

}  // namespace p4c::P4
