// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "reservedWords.h"

namespace P4 {

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

}  // namespace P4
