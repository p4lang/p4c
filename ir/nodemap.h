/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_NODEMAP_H_
#define IR_NODEMAP_H_

#include "ir/node.h"
#include "lib/cstring.h"

namespace P4::IR {

template <class KEY, class VALUE,
          template <class K, class V, class COMP, class ALLOC> class MAP = std::map,
          class COMP = std::less<cstring>,
          class ALLOC = std::allocator<std::pair<const KEY *const, const VALUE *>>>
class NodeMap : public Node {
    typedef MAP<const KEY *, const VALUE *, COMP, ALLOC> map_t;
    map_t symbols;

 public:
    typedef typename map_t::value_type value_type;
    typedef typename map_t::iterator iterator;
    typedef typename map_t::const_iterator const_iterator;
    typedef typename map_t::reverse_iterator reverse_iterator;
    typedef typename map_t::const_reverse_iterator const_reverse_iterator;

 private:
    struct elem_ref {
        NodeMap &self;
        const KEY *key;
        elem_ref(NodeMap &s, const KEY *k) : self(s), key(k) {}
        const VALUE *operator=(const VALUE *v) const { return self.symbols[key] = v; }
        operator const VALUE *() const { return self.symbols.at(key); }
    };

 public:
    const_iterator begin() const { return symbols.begin(); }
    const_iterator end() const { return symbols.end(); }
    iterator begin() { return symbols.begin(); }
    iterator end() { return symbols.end(); }
    const_reverse_iterator rbegin() const { return symbols.rbegin(); }
    const_reverse_iterator rend() const { return symbols.rend(); }
    reverse_iterator rbegin() { return symbols.rbegin(); }
    reverse_iterator rend() { return symbols.rend(); }
    size_t count(cstring name) const { return symbols.count(name); }
    size_t size() const { return symbols.size(); }
    bool empty() const { return symbols.empty(); }
    size_t erase(const KEY *k) { return symbols.erase(k); }
    iterator erase(const_iterator p) { return symbols.erase(p); }
    iterator erase(const_iterator f, const_iterator l) { return symbols.erase(f, l); }
    const_iterator find(const KEY *k) const { return symbols.find(k); }
    template <class U>
    const U *get(const KEY *k) const {
        for (auto it = symbols.find(k); it != symbols.end() && it->first == k; it++)
            if (auto rv = it->second->template to<U>()) return rv;
        return nullptr;
    }
    elem_ref operator[](const KEY *k) { return elem_ref(*this, k); }
    const VALUE *operator[](const KEY *k) const { return symbols.at(k); }
    const VALUE *&at(const KEY *k) { return symbols.at(k); }
    const VALUE *const &at(const KEY *k) const { return symbols.at(k); }
    IRNODE_SUBCLASS(NodeMap)
    bool operator==(const Node &a) const override { return a == *this; }
    bool operator==(const NodeMap &a) const { return symbols == a.symbols; }
    bool equiv(const Node &a_) const override {
        if (static_cast<const Node *>(this) == &a_) return true;
        if (this->typeId() != a_.typeId()) return false;
        auto &a = static_cast<const NodeMap<KEY, VALUE, MAP, COMP, ALLOC> &>(a_);
        if (size() != a.size()) return false;
        auto it = a.begin();
        for (auto &el : *this)
            if (el.first != it->first || !el.second->equiv(*(it++)->second)) return false;
        return true;
    }
    cstring node_type_name() const override {
        return "NodeMap<" + KEY::static_type_name() + "," + VALUE::static_type_name() + ">";
    }
    static cstring static_type_name() {
        return "NodeMap<" + KEY::static_type_name() + "," + VALUE::static_type_name() + ">";
    }
    void visit_children(Visitor &v, const char *) override;
    void visit_children(Visitor &v, const char *) const override;

    DECLARE_TYPEINFO(NodeMap, Node);
};

}  // namespace P4::IR

#endif /* IR_NODEMAP_H_ */
