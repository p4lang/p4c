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

#ifndef _IR_NAMEMAP_H_
#define _IR_NAMEMAP_H_

class JSONLoader;

namespace IR {

template<class T, template<class K, class V, class COMP, class ALLOC> class MAP = std::map,
         class COMP = std::less<cstring>,
         class ALLOC = std::allocator<std::pair<const cstring, const T*>>>
class NameMap : public Node {
    typedef MAP<cstring, const T *, COMP, ALLOC>        map_t;
    map_t       symbols;
    /* if the object has a 'name' field, is it the same as name? */
    template<class U> auto match_name(cstring name, const U *obj) -> decltype(name == obj->name) {
        return name == obj->name; }
    bool match_name(cstring, const void *) { return true; }
    template<class U> auto obj_name(const U *obj) -> decltype(obj->name) { return obj->name; }
    cstring obj_name(const void *) { return cstring(0); }

 public:
    NameMap() = default;
    NameMap(const NameMap &) = default;
    NameMap(NameMap &&) = default;
    explicit NameMap(JSONLoader &);
    NameMap &operator=(const NameMap &) = default;
    NameMap &operator=(NameMap &&) = default;
    typedef typename map_t::value_type          value_type;
    typedef typename map_t::iterator            iterator;
    typedef typename map_t::const_iterator      const_iterator;
    typedef typename map_t::reverse_iterator            reverse_iterator;
    typedef typename map_t::const_reverse_iterator      const_reverse_iterator;

 private:
    struct elem_ref {
        NameMap         &self;
        cstring         name;
        elem_ref(NameMap &s, cstring n) : self(s), name(n) {}
        const T *operator=(const T *v) const {
            if (!self.match_name(name, v))
                BUG("Inserting into NameMap with incorrect name");
            return self.symbols[name] = v; }
        operator const T *() const { return self.count(name) ? self.at(name) : nullptr; }
    };

 public:
    const_iterator begin() const { return symbols.begin(); }
    const_iterator end() const { return symbols.end(); }
    iterator begin() { return symbols.begin(); }
    iterator end() { return symbols.end(); }
    // For multimaps
    std::pair<const_iterator, const_iterator> equal_range(cstring key) const
    { return symbols.equal_range(key); }
    const_reverse_iterator rbegin() const { return symbols.rbegin(); }
    const_reverse_iterator rend() const { return symbols.rend(); }
    reverse_iterator rbegin() { return symbols.rbegin(); }
    reverse_iterator rend() { return symbols.rend(); }
    size_t count(cstring name) const { return symbols.count(name); }
    void clear() { symbols.clear(); }
    size_t size() const { return symbols.size(); }
    bool empty() const { return symbols.empty(); }
    size_t erase(cstring n) { return symbols.erase(n); }
    /* should be erase(const_iterator), but glibc++ std::list::erase is broken */
    iterator erase(iterator p) { return symbols.erase(p); }
    iterator erase(iterator f, iterator l) { return symbols.erase(f, l); }
    const_iterator find(cstring name) const { return symbols.find(name); }
    template<class U> const U *get(cstring name) const {
        for (auto it = symbols.find(name); it != symbols.end() && it->first == name; it++)
            if (auto rv = dynamic_cast<const U *>(it->second))
                return rv;
        return nullptr; }
    void add(cstring name, const T *n) {
        if (!match_name(name, n))
            BUG("Inserting into NameMap with incorrect name");
        symbols.emplace(std::move(name), std::move(n)); }
    // Expects to have a single node with each name.
    // Only works for map and unordered_map
    void addUnique(cstring name, const T *n) {
        auto prev = symbols.find(name);
        if (prev != symbols.end())
            ::error(ErrorType::ERR_DUPLICATE,
                    "%1%: duplicated name (%2% is previous instance)", n, prev->second);
        symbols.emplace(std::move(name), std::move(n)); }
    // Expects to have a single node with each name.
    // Only works for map and unordered_map
    const T *getUnique(cstring name) const {
        auto it = symbols.find(name);
        if (it == symbols.end())
            return nullptr;
        return it->second;
    }
    elem_ref operator[](cstring name) { return elem_ref(*this, name); }
    const T *operator[](cstring name) const { return count(name) ? at(name) : nullptr; }
    const T *&at(cstring name) { return symbols.at(name); }
    const T *const &at(cstring name) const { return symbols.at(name); }
    void check_null() const { for (auto &e : symbols) CHECK_NULL(e.second); }

    IRNODE_SUBCLASS(NameMap)
    bool operator==(const Node &a) const override { return a == *this; }
    bool operator==(const NameMap &a) const { return symbols == a.symbols; }
    bool equiv(const Node &a_) const override {
        if (static_cast<const Node *>(this) == &a_) return true;
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const NameMap<T, MAP, COMP, ALLOC> &>(a_);
        if (size() != a.size()) return false;
        auto it = a.begin();
        for (auto &el : *this)
            if (el.first != it->first || !el.second->equiv(*(it++)->second))
                return false;
        return true; }
    cstring node_type_name() const override {
        return "NameMap<" + T::static_type_name() + ">"; }
    static cstring static_type_name() {
        return "NameMap<" + T::static_type_name() + ">"; }
    void visit_children(Visitor &v) override;
    void visit_children(Visitor &v) const override;
    void toJSON(JSONGenerator &json) const override;
    static NameMap<T, MAP, COMP, ALLOC> *fromJSON(JSONLoader &json);

    Util::Enumerator<const T*>* valueEnumerator() const {
        return Util::Enumerator<const T*>::createEnumerator(Values(symbols).begin(),
                                                            Values(symbols).end()); }
    template <typename S>
    Util::Enumerator<const S*>* only() const {
        std::function<bool(const T*)> filter = [](const T* d) { return d->template is<S>(); };
        return valueEnumerator()->where(filter)->template as<const S*>(); }
};

}  // namespace IR

#endif /* _IR_NAMEMAP_H_ */
