// User-defined match_kind identifiers spread across several declarations are
// merged into a single match_kind node. ToP4 prints them together in one block
// after the core.p4 include, dropping the match_kinds already defined there.
// See https://github.com/p4lang/p4c/issues/5085

#include <core.p4>

match_kind {
    first_kind,
    second_kind
}

match_kind {
    third_kind
}
