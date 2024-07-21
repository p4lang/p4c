#include "callGraph.h"

namespace p4c::P4 {
using namespace literals;

cstring cgMakeString(cstring s) { return s; }
cstring cgMakeString(char c) { return ""_cs + c; }
cstring cgMakeString(const IR::Node *node) { return node->toString(); }
cstring cgMakeString(const IR::INode *node) { return node->toString(); }

}  // namespace p4c::P4
