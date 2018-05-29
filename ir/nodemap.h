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

#ifndef _IR_NODEMAP_H_
#define _IR_NODEMAP_H_

namespace IR {

template<class KEY, class VALUE,
         template<class K, class V, class COMP, class ALLOC> class MAP = std::map,
         class COMP = std::less<cstring>,
         class ALLOC = std::allocator<std::pair<const KEY * const, const VALUE *>>>
class NodeMap : public Node {
    typedef MAP<const KEY *, const VALUE *, COMP, ALLOC>        map_t;
    map_t       symbols;

 public:
    typedef typename map_t::value_type          value_type;
    typedef typename map_t::iterator            iterator;
    typedef typename map_t::const_iterator      const_iterator;
    typedef typename map_t::reverse_iterator            reverse_iterator;
    typedef typename map_t::const_reverse_iterator      const_reverse_iterator;

 private:
    struct elem_ref {
        NodeMap         &self;
        const KEY       *key;
        elem_ref(NodeMap &s, const KEY *k) : self(s), key(k) {}
        const VALUE *operator=(const VALUE *v) const {
            return self.symbols[key] = v; }
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
    template<class U> const U *get(const KEY *k) const {
        for (auto it = symbols.find(k); it != symbols.end() && it->first == k; it++)
            if (auto rv = dynamic_cast<const U *>(it->second))
                return rv;
        return nullptr; }
    elem_ref operator[](const KEY *k) { return elem_ref(*this, k); }
    const VALUE *operator[](const KEY *k) const { return symbols.at(k); }
    const VALUE *&at(const KEY *k) { return symbols.at(k); }
    const VALUE *const &at(const KEY *k) const { return symbols.at(k); }
    IRNODE_SUBCLASS(NodeMap)
    bool operator==(const Node &a) const override { return a == *this; }
    bool operator==(const NodeMap &a) const { return symbols == a.symbols; }
    bool equiv(const Node &a_) const override {
        if (static_cast<const Node *>(this) == &a_) return true;
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const NodeMap<KEY, VALUE, MAP, COMP, ALLOC> &>(a_);
        if (size() != a.size()) return false;
        auto it = a.begin();
        for (auto &el : *this)
            if (el.first != it->first || !el.second->equiv(*(it++)->second))
                return false;
        return true; }
    cstring node_type_name() const override {
        return "NodeMap<" + KEY::static_type_name() + "," + VALUE::static_type_name() + ">"; }
    static cstring static_type_name() {
        return "NodeMap<" + KEY::static_type_name() + "," + VALUE::static_type_name() + ">"; }
    void visit_children(Visitor &v) override;
    void visit_children(Visitor &v) const override;
};

}  // namespace IR

#endif /* _IR_NODEMAP_H_ */
