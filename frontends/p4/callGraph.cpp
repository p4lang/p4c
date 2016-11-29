#include "callGraph.h"

namespace P4 {

cstring cgMakeString(cstring s) { return s; }
cstring cgMakeString(char c) { return cstring("") + c; }
cstring cgMakeString(const IR::Node* node) { return node->toString(); }
cstring cgMakeString(const IR::INode* node) { return node->toString(); }

}
