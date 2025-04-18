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

#ifndef IR_IR_INLINE_H_
#define IR_IR_INLINE_H_

#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/json_generator.h"
#include "ir/namemap.h"
#include "ir/nodemap.h"
#include "ir/visitor.h"
#include "lib/ordered_map.h"

namespace P4 {

#define DEFINE_APPLY_FUNCTIONS(CLASS, TEMPLATE, TT, INLINE)                                       \
    TEMPLATE INLINE bool IR::CLASS TT::apply_visitor_preorder(Modifier &v) {                      \
        Node::traceVisit("Mod pre");                                                              \
        return v.preorder(this);                                                                  \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_postorder(Modifier &v) {                     \
        Node::traceVisit("Mod post");                                                             \
        v.postorder(this);                                                                        \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(Modifier &v, const Node *n) const {  \
        Node::traceVisit("Mod revisit");                                                          \
        v.revisit(this, n);                                                                       \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(Modifier &v) const {            \
        Node::traceVisit("Mod loop_revisit");                                                     \
        v.loop_revisit(this);                                                                     \
    }                                                                                             \
    TEMPLATE INLINE bool IR::CLASS TT::apply_visitor_preorder(Inspector &v) const {               \
        Node::traceVisit("Insp pre");                                                             \
        return v.preorder(this);                                                                  \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_postorder(Inspector &v) const {              \
        Node::traceVisit("Insp post");                                                            \
        v.postorder(this);                                                                        \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(Inspector &v) const {                \
        Node::traceVisit("Insp revisit");                                                         \
        v.revisit(this);                                                                          \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(Inspector &v) const {           \
        Node::traceVisit("Insp loop_revisit");                                                    \
        v.loop_revisit(this);                                                                     \
    }                                                                                             \
    TEMPLATE INLINE const IR::Node *IR::CLASS TT::apply_visitor_preorder(Transform &v) {          \
        Node::traceVisit("Trans pre");                                                            \
        return v.preorder(this);                                                                  \
    }                                                                                             \
    TEMPLATE INLINE const IR::Node *IR::CLASS TT::apply_visitor_postorder(Transform &v) {         \
        Node::traceVisit("Trans post");                                                           \
        return v.postorder(this);                                                                 \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(Transform &v, const Node *n) const { \
        Node::traceVisit("Trans revisit");                                                        \
        v.revisit(this, n);                                                                       \
    }                                                                                             \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(Transform &v) const {           \
        Node::traceVisit("Trans loop_revisit");                                                   \
        v.loop_revisit(this);                                                                     \
    }

IRNODE_ALL_TEMPLATES(DEFINE_APPLY_FUNCTIONS, inline)

template <class T>
void IR::Vector<T>::visit_children(Visitor &v, const char *name) {
    for (auto i = vec.begin(); i != vec.end();) {
        const IR::Node *n = v.apply_visitor(*i, name);
        if (!n && *i) {
            i = erase(i);
            continue;
        }
        CHECK_NULL(n);
        if (n == *i) {
            i++;
            continue;
        }
        if (auto l = n->to<Vector<T>>()) {
            i = erase(i);
            i = insert(i, l->vec.begin(), l->vec.end());
            i += l->vec.size();
            continue;
        }
        if (const auto *v = n->to<VectorBase>()) {
            if (v->empty()) {
                i = erase(i);
            } else {
                i = insert(i, v->size() - 1, nullptr);
                for (const auto *el : *v) {
                    CHECK_NULL(el);
                    if (auto e = el->template to<T>()) {
                        *i++ = e;
                    } else {
                        BUG("visitor returned invalid type %s for Vector<%s>", el->node_type_name(),
                            T::static_type_name());
                    }
                }
            }
            continue;
        }
        if (auto e = n->to<T>()) {
            *i++ = e;
            continue;
        }
        BUG("visitor returned invalid type %s for Vector<%s>", n->node_type_name(),
            T::static_type_name());
    }
}
template <class T>
void IR::Vector<T>::visit_children(Visitor &v, const char *name) const {
    for (auto &a : vec) v.visit(a, name);
}
template <class T>
void IR::Vector<T>::parallel_visit_children(Visitor &v, const char *) {
    SplitFlowVisitVector<T>(v, *this).run_visit();
}
template <class T>
void IR::Vector<T>::parallel_visit_children(Visitor &v, const char *) const {
    SplitFlowVisitVector<T>(v, *this).run_visit();
}
IRNODE_DEFINE_APPLY_OVERLOAD(Vector, template <class T>, <T>)
template <class T>
void IR::Vector<T>::toJSON(JSONGenerator &json) const {
    Node::toJSON(json);
    json.emit_tag("vec");
    auto state = json.begin_vector();
    for (auto &k : vec) json.emit(k);
    json.end_vector(state);
}

std::ostream &operator<<(std::ostream &out, const IR::Vector<IR::Expression> &v);
std::ostream &operator<<(std::ostream &out, const IR::Vector<IR::Annotation> &v);

template <class T>
void IR::IndexedVector<T>::visit_children(Visitor &v, const char *name) {
    for (auto i = begin(); i != end();) {
        auto n = v.apply_visitor(*i, name);
        if (!n && *i) {
            i = erase(i);
            continue;
        }
        CHECK_NULL(n);
        if (n == *i) {
            i++;
            continue;
        }
        if (auto l = n->template to<Vector<T>>()) {
            i = erase(i);
            i = insert(i, l->begin(), l->end());
            i += l->Vector<T>::size();
            continue;
        }
        if (auto e = n->template to<T>()) {
            i = replace(i, e);
            continue;
        }
        BUG("visitor returned invalid type %s for IndexedVector<%s>", n->node_type_name(),
            T::static_type_name());
    }
}
template <class T>
void IR::IndexedVector<T>::visit_children(Visitor &v, const char *name) const {
    for (auto &a : *this) v.visit(a, name);
}
template <class T>
void IR::IndexedVector<T>::toJSON(JSONGenerator &json) const {
    Vector<T>::toJSON(json);
    json.emit_tag("declarations");
    auto state = json.begin_object();
    for (auto &k : declarations) json.emit(k.first, k.second);
    json.end_object(state);
}
IRNODE_DEFINE_APPLY_OVERLOAD(IndexedVector, template <class T>, <T>)

template <class MAP>
static inline void namemap_insert_helper(typename MAP::iterator, typename MAP::key_type k,
                                         typename MAP::mapped_type v, MAP &, MAP &new_symbols) {
    new_symbols.emplace(std::move(k), std::move(v));
}

template <class MAP, class InputIterator>
static inline void namemap_insert_helper(typename MAP::iterator, InputIterator b, InputIterator e,
                                         MAP &, MAP &new_symbols) {
    new_symbols.insert(b, e);
}

template <class T>
static inline void namemap_insert_helper(typename ordered_map<cstring, T>::iterator it, cstring k,
                                         T v, ordered_map<cstring, T> &symbols,
                                         ordered_map<cstring, T> &) {
    symbols.emplace_hint(it, std::move(k), std::move(v));
}

template <class T, class InputIterator>
static inline void namemap_insert_helper(typename ordered_map<cstring, T>::iterator it,
                                         InputIterator b, InputIterator e,
                                         ordered_map<cstring, T> &symbols,
                                         ordered_map<cstring, T> &) {
    symbols.insert(it, b, e);
}

template <class T, template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<const cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::visit_children(Visitor &v, const char *) {
    map_t new_symbols;
    for (auto i = symbols.begin(); i != symbols.end();) {
        const IR::Node *n = v.apply_visitor(i->second, i->first.c_str());
        if (!n && i->second) {
            i = symbols.erase(i);
            continue;
        }
        CHECK_NULL(n);
        if (n == i->second) {
            i++;
            continue;
        }
        if (auto m = n->to<NameMap>()) {
            namemap_insert_helper(i, m->symbols.begin(), m->symbols.end(), symbols, new_symbols);
            i = symbols.erase(i);
            continue;
        }
        if (auto s = n->to<T>()) {
            if (match_name(i->first, s)) {
                i->second = s;
                i++;
            } else {
                namemap_insert_helper(i, cstring(obj_name(s)), std::move(s), symbols, new_symbols);
                i = symbols.erase(i);
            }
            continue;
        }
        BUG("visitor returned invalid type %s for NameMap<%s>", n->node_type_name(),
            T::static_type_name());
    }
    symbols.insert(new_symbols.begin(), new_symbols.end());
}
template <class T, template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::visit_children(Visitor &v, const char *) const {
    for (auto &k : symbols) {
        v.visit(k.second, k.first.c_str());
    }
}
template <class T, template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::toJSON(JSONGenerator &json) const {
    Node::toJSON(json);
    json.emit_tag("symbols");
    auto state = json.begin_object();
    for (auto &k : symbols) json.emit(k.first, k.second);
    json.end_object(state);
}

template <class KEY, class VALUE,
          template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NodeMap<KEY, VALUE, MAP, COMP, ALLOC>::visit_children(Visitor &v, const char *name) {
    map_t new_symbols;
    for (auto i = symbols.begin(); i != symbols.end();) {
        auto nk = i->first;
        v.visit(nk, name);
        if (!nk && i->first) {
            i = symbols.erase(i);
        } else if (nk == i->first) {
            v.visit(i->second);
            if (i->second) {
                ++i;
            } else {
                i = symbols.erase(i);
            }
        } else {
            auto nv = i->second;
            v.visit(nv, name);
            if (nv) new_symbols.emplace(nk, nv);
            i = symbols.erase(i);
        }
    }
    symbols.insert(new_symbols.begin(), new_symbols.end());
}
template <class KEY, class VALUE,
          template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NodeMap<KEY, VALUE, MAP, COMP, ALLOC>::visit_children(Visitor &v, const char *name) const {
    for (auto &k : symbols) {
        v.visit(k.first, name);
        v.visit(k.second, name);
    }
}

}  // namespace P4

#endif /* IR_IR_INLINE_H_ */
