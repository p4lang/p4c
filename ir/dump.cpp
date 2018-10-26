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

#include "ir.h"

namespace {
class IRDumper : public Inspector {
    std::ostream                &out;
    std::set<const IR::Node *>  dumped;
    unsigned                    maxdepth;
    cstring                     ignore;
    bool                        source;
    bool preorder(const IR::Node *n) override {
        if (auto ctxt = getContext()) {
            if (unsigned(ctxt->depth) > maxdepth)
                return false;
            if (ctxt->child_name && ignore == ctxt->child_name)
                return false;
            out << indent_t(ctxt->depth);
            if (ctxt->child_name)
                out << ctxt->child_name << ": "; }
        out << "[" << n->id << "] ";
        if (source && n->srcInfo)
            out << "(" << n->srcInfo.toPositionString() << ") ";
        out << n->node_type_name();
        n->dump_fields(out);
        if (dumped.count(n)) {
            out << "..." << std::endl;
            return false; }
        dumped.insert(n);
        out << std::endl;
        return true; }
    bool preorder(const IR::Expression *e) override {
        if (!preorder(static_cast<const IR::Node *>(e))) return false;
        visit(e->type, "type");
        return true; }
    bool preorder(const IR::Constant *c) override {
        return preorder(static_cast<const IR::Node *>(c)); }
    void postorder(const IR::Node *n) override {
        if (getChildrenVisited() == 0)
            dumped.erase(n); }

 public:
    IRDumper(std::ostream &o, unsigned m, cstring ign, bool src)
    : out(o), maxdepth(m), ignore(ign), source(src) { visitDagOnce = false; }
};
}  // namespace

void dump(std::ostream &out, const IR::Node *n, unsigned maxdepth) {
    n->apply(IRDumper(out, maxdepth, nullptr, false)); }
void dump(std::ostream &out, const IR::Node *n) { dump(out, n, ~0U); }
void dump(const IR::Node *n, unsigned maxdepth) { dump(std::cout, n, maxdepth); }
void dump(const IR::Node *n) { dump(n, ~0U); }
void dump(const IR::INode *n, unsigned maxdepth) { dump(std::cout, n->getNode(), maxdepth); }
void dump(const IR::INode *n) { dump(n, ~0U); }
void dump_notype(const IR::Node *n, unsigned maxdepth) {
    n->apply(IRDumper(std::cout, maxdepth, "type", false)); }
void dump_notype(const IR::Node *n) { dump_notype(n, ~0U); }
void dump_notype(const IR::INode *n, unsigned maxdepth) {
    n->getNode()->apply(IRDumper(std::cout, maxdepth, "type", false)); }
void dump_notype(const IR::INode *n) { dump_notype(n, ~0U); }
void dump_src(const IR::Node *n, unsigned maxdepth) {
    n->apply(IRDumper(std::cout, maxdepth, "type", true)); }
void dump_src(const IR::Node *n) { dump_src(n, ~0U); }
void dump_src(const IR::INode *n, unsigned maxdepth) {
    n->getNode()->apply(IRDumper(std::cout, maxdepth, "type", true)); }
void dump_src(const IR::INode *n) { dump_src(n, ~0U); }

void dump(uintptr_t p, unsigned maxdepth) {
    dump(reinterpret_cast<const IR::Node *>(p), maxdepth); }
void dump(uintptr_t p) { dump(p, ~0U); }
void dump_notype(uintptr_t p, unsigned maxdepth) {
    dump_notype(reinterpret_cast<const IR::Node *>(p), maxdepth); }
void dump_notype(uintptr_t p) { dump_notype(p, ~0U); }

void dump(std::ostream &out, const Visitor::Context *ctxt) {
    if (!ctxt) return;
    dump(ctxt->parent);
    out << indent_t(ctxt->depth-1);
    if (ctxt->parent) {
        if (ctxt->parent->child_name)
            out << ctxt->parent->child_name << ": ";
        else
            out << ctxt->parent->child_index << ": "; }
    if (ctxt->original != ctxt->node) {
        out << "<" << static_cast<const void *>(ctxt->original) << ":["
            << ctxt->original->id << "] " << ctxt->original->node_type_name();
        ctxt->original->dump_fields(out);
        out << std::endl << indent_t(ctxt->depth-1) << ">"; }
    out << static_cast<const void *>(ctxt->node) << ":[" << ctxt->node->id << "] "
        << ctxt->node->node_type_name();
    ctxt->node->dump_fields(out);
    std::cout << std::endl;
}
void dump(const Visitor::Context *ctxt) { dump(std::cout, ctxt); }

std::string dumpToString(const IR::Node* n) {
    std::stringstream str;
    dump(str, n); return str.str(); }
