#include "reservedWords.h"

namespace P4 {

// Keep this in sync with the lexer
std::set<cstring> reservedWords = {
    "action", "actions", "apply", "bool", "bit", "const", "control", "default",
    "else", "enum", "error", "extern", "false", "header", "header_union", "if",
    "in", "inout", "int", "key", "match_kind", "out", "parser", "package",
    "return", "select", "state", "struct", "switch", "table", "transition",
    "true", "typedef", "varbit", "void"
};

}  // namespace P4
