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

#ifndef _IR_IR_INLINE_H_
#define _IR_IR_INLINE_H_

#define DEFINE_APPLY_FUNCTIONS(CLASS, TEMPLATE, TT)                               \
    TEMPLATE inline bool IR::CLASS TT::apply_visitor_preorder(Modifier &v)              \
    { Node::traceVisit("Mod pre"); return v.preorder(this); }                           \
    TEMPLATE inline void IR::CLASS TT::apply_visitor_postorder(Modifier &v)             \
    { Node::traceVisit("Mod post"); v.postorder(this); }                                \
    TEMPLATE inline bool IR::CLASS TT::apply_visitor_preorder(Inspector &v) const       \
    { Node::traceVisit("Insp pre"); return v.preorder(this); }                          \
    TEMPLATE inline void IR::CLASS TT::apply_visitor_postorder(Inspector &v) const      \
    { Node::traceVisit("Insp post"); v.postorder(this); }                               \
    TEMPLATE inline const IR::Node *IR::CLASS TT::apply_visitor_preorder(Transform &v)  \
    { Node::traceVisit("Trans pre"); return v.preorder(this); }                         \
    TEMPLATE inline const IR::Node *IR::CLASS TT::apply_visitor_postorder(Transform &v) \
    { Node::traceVisit("Trans post"); return v.postorder(this); }
    IRNODE_ALL_NON_TEMPLATE_CLASSES(DEFINE_APPLY_FUNCTIONS, , )
    IRNODE_ALL_TEMPLATES(DEFINE_APPLY_FUNCTIONS)
#undef DEFINE_APPLY_FUNCTIONS

#define DEFINE_VISIT_FUNCTIONS(CLASS, BASE)                                             \
    inline void Visitor::visit(const IR::CLASS *&n, const char *name) {                 \
        auto t = apply_visitor(n, name);                                                \
        n = dynamic_cast<const IR::CLASS *>(t);                                         \
        if (t && !n)                                                                    \
            BUG("visitor returned non-" #CLASS " type: %1%", t); }                      \
    inline void Visitor::visit(const IR::CLASS *const &n, const char *name) {           \
        /* This function needed solely due to order of declaration issues */            \
        visit(static_cast<const IR::Node *const &>(n), name); }
    IRNODE_ALL_SUBCLASSES(DEFINE_VISIT_FUNCTIONS)
#undef DEFINE_VISIT_FUNCTIONS

template<class T> void IR::Vector<T>::visit_children(Visitor &v) {
    for (auto i = vec.begin(); i != vec.end();) {
        auto n = v.apply_visitor(*i);
        if (!n) {
            i = erase(i);
        } else if (n == *i) {
            i++;
        } else if (auto l = dynamic_cast<const Vector *>(n)) {
            i = erase(i);
            i = insert(i, l->vec.begin(), l->vec.end());
            i += l->vec.size();
        } else if (auto v = dynamic_cast<const VectorBase *>(n)) {
            if (v->empty()) {
                i = erase(i);
            } else {
                i = insert(i, v->size() - 1, nullptr);
                for (auto el : *v) {
                    if (auto e = dynamic_cast<const T *>(el))
                        *i++ = e;
                    else
                        BUG("visitor returned invalid type %s for Vector<%s>",
                            e->node_type_name(), T::static_type_name()); } }
        } else if (auto e = dynamic_cast<const T *>(n)) {
            *i++ = e;
        } else {
            BUG("visitor returned invalid type %s for Vector<%s>",
                n->node_type_name(), T::static_type_name()); } } }
template<class T> void IR::Vector<T>::visit_children(Visitor &v) const {
    for (auto &a : vec) v.visit(a); }
IRNODE_DEFINE_APPLY_OVERLOAD(Vector, template<class T>, <T>)

std::ostream &operator<<(std::ostream &out, const IR::Vector<IR::Expression> &v);
inline std::ostream &operator<<(std::ostream &out, const IR::Vector<IR::Expression> *v) {
    return v ? out << *v : out << "<null>"; }

template<class T> void IR::IndexedVector<T>::visit_children(Visitor &v) {
    for (auto i = begin(); i != end();) {
        auto n = v.apply_visitor(*i);
        if (!n) {
            i = erase(i);
        } else if (n == *i) {
            i++;
        } else if (auto l = dynamic_cast<const Vector<T> *>(n)) {
            i = erase(i);
            i = insert(i, l->begin(), l->end());
            i += l->Vector<T>::size();
        } else if (auto e = dynamic_cast<const T *>(n)) {
            i = replace(i, e);
        } else {
            BUG("visitor returned invalid type %s for IndexedVector<%s>",
                n->node_type_name(), T::static_type_name()); } } }
template<class T> void IR::IndexedVector<T>::visit_children(Visitor &v) const {
    for (auto &a : *this) v.visit(a); }
IRNODE_DEFINE_APPLY_OVERLOAD(IndexedVector, template<class T>, <T>)

#include "lib/ordered_map.h"
template <class MAP> static inline void
namemap_insert_helper(typename MAP::iterator, typename MAP::key_type k,
                      typename MAP::mapped_type v, MAP &, MAP &new_symbols) {
    new_symbols.emplace(std::move(k), std::move(v));
}

template <class MAP, class InputIterator> static inline void
namemap_insert_helper(typename MAP::iterator, InputIterator b, InputIterator e,
                      MAP &, MAP &new_symbols) {
    new_symbols.insert(b, e);
}

template <class T> static inline void
namemap_insert_helper(typename ordered_map<cstring, T>::iterator it, cstring k, T v,
                      ordered_map<cstring, T> &symbols, ordered_map<cstring, T> &) {
    symbols.emplace_hint(it, std::move(k), std::move(v));
}

template <class T, class InputIterator> static inline void
namemap_insert_helper(typename ordered_map<cstring, T>::iterator it,
                      InputIterator b, InputIterator e,
                      ordered_map<cstring, T> &symbols, ordered_map<cstring, T> &) {
    symbols.insert(it, b, e);
}

template<class T, template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<const cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::visit_children(Visitor &v) {
    map_t   new_symbols;
    for (auto i = symbols.begin(); i != symbols.end();) {
        auto n = v.apply_visitor(i->second, i->first);
        if (!n) {
            i = symbols.erase(i);
        } else if (n == i->second) {
            i++;
        } else if (auto m = dynamic_cast<const NameMap *>(n)) {
            namemap_insert_helper(i, m->symbols.begin(), m->symbols.end(), symbols, new_symbols);
            i = symbols.erase(i);
        } else if (auto s = dynamic_cast<const T *>(n)) {
            if (match_name(i->first, s)) {
                i->second = s;
                i++;
            } else {
                namemap_insert_helper(i, cstring(obj_name(s)), std::move(s), symbols, new_symbols);
                i = symbols.erase(i); }
        } else {
            BUG("visitor returned invalid type %s for NameMap<%s>",
                n->node_type_name(), T::static_type_name()); } }
    symbols.insert(new_symbols.begin(), new_symbols.end()); }
template<class T, template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::visit_children(Visitor &v) const {
    for (auto &k : symbols) v.visit(k.second, k.first); }

template<class KEY, class VALUE,
         template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NodeMap<KEY, VALUE, MAP, COMP, ALLOC>::visit_children(Visitor &v) {
    map_t   new_symbols;
    for (auto i = symbols.begin(); i != symbols.end();) {
        auto nk = i->first;
        v.visit(nk);
        if (!nk) {
            i = symbols.erase(i);
        } else if (nk == i->first) {
            v.visit(i->second);
            if (i->second)
                ++i;
            else
                i = symbols.erase(i);
        } else {
            auto nv = i->second;
            v.visit(nv);
            if (nv) new_symbols.emplace(nk, nv);
            i = symbols.erase(i); } }
    symbols.insert(new_symbols.begin(), new_symbols.end()); }
template<class KEY, class VALUE,
         template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NodeMap<KEY, VALUE, MAP, COMP, ALLOC>::visit_children(Visitor &v) const {
    for (auto &k : symbols) { v.visit(k.first); v.visit(k.second); } }
#endif /* _IR_IR_INLINE_H_ */
