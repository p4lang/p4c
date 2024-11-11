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

#define DEFINE_APPLY_FUNCTIONS(CLASS, TEMPLATE, TT, INLINE)                                        \
    TEMPLATE INLINE bool IR::CLASS TT::apply_visitor_preorder(Modifier &v) {                       \
        Node::traceVisit("Mod pre");                                                               \
        return v.preorder(this);                                                                   \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_postorder(Modifier &v) {                      \
        Node::traceVisit("Mod post");                                                              \
        v.postorder(this);                                                                         \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(Modifier &v, const Node *n) const {   \
        Node::traceVisit("Mod revisit");                                                           \
        v.revisit(this, n);                                                                        \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(Modifier &v) const {             \
        Node::traceVisit("Mod loop_revisit");                                                      \
        v.loop_revisit(this);                                                                      \
    }                                                                                              \
    TEMPLATE INLINE bool IR::CLASS TT::apply_visitor_preorder(COWNode_info *info, COWModifier &v)  \
        const {                                                                                    \
        Node::traceVisit("Mod pre (COW)");                                                         \
        return v.preorder(IR::COWptr<IR::CLASS TT>(info));                                         \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_postorder(COWNode_info *info, COWModifier &v) \
        const {                                                                                    \
        Node::traceVisit("Mod post (COW)");                                                        \
        v.postorder(IR::COWptr<IR::CLASS TT>(info));                                               \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(COWModifier &v, const Node *n)        \
        const {                                                                                    \
        Node::traceVisit("Mod revisit (COW)");                                                     \
        v.revisit(this, n);                                                                        \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(COWModifier &v) const {          \
        Node::traceVisit("Mod loop_revisit (COW)");                                                \
        v.loop_revisit(this);                                                                      \
    }                                                                                              \
    TEMPLATE INLINE bool IR::CLASS TT::apply_visitor_preorder(Inspector &v) const {                \
        Node::traceVisit("Insp pre");                                                              \
        return v.preorder(this);                                                                   \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_postorder(Inspector &v) const {               \
        Node::traceVisit("Insp post");                                                             \
        v.postorder(this);                                                                         \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(Inspector &v) const {                 \
        Node::traceVisit("Insp revisit");                                                          \
        v.revisit(this);                                                                           \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(Inspector &v) const {            \
        Node::traceVisit("Insp loop_revisit");                                                     \
        v.loop_revisit(this);                                                                      \
    }                                                                                              \
    TEMPLATE INLINE const IR::Node *IR::CLASS TT::apply_visitor_preorder(Transform &v) {           \
        Node::traceVisit("Trans pre");                                                             \
        return v.preorder(this);                                                                   \
    }                                                                                              \
    TEMPLATE INLINE const IR::Node *IR::CLASS TT::apply_visitor_postorder(Transform &v) {          \
        Node::traceVisit("Trans post");                                                            \
        return v.postorder(this);                                                                  \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(Transform &v, const Node *n) const {  \
        Node::traceVisit("Trans revisit");                                                         \
        v.revisit(this, n);                                                                        \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(Transform &v) const {            \
        Node::traceVisit("Trans loop_revisit");                                                    \
        v.loop_revisit(this);                                                                      \
    }                                                                                              \
    TEMPLATE INLINE const IR::Node *IR::CLASS TT::apply_visitor_preorder(COWNode_info *info,       \
                                                                         COWTransform &v) const {  \
        Node::traceVisit("Trans pre (COW)");                                                       \
        return v.preorder(IR::COWptr<IR::CLASS TT>(info));                                         \
    }                                                                                              \
    TEMPLATE INLINE const IR::Node *IR::CLASS TT::apply_visitor_postorder(COWNode_info *info,      \
                                                                          COWTransform &v) const { \
        Node::traceVisit("Trans post (COW)");                                                      \
        return v.postorder(IR::COWptr<IR::CLASS TT>(info));                                        \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_revisit(COWTransform &v, const Node *n)       \
        const {                                                                                    \
        Node::traceVisit("Trans revisit (COW)");                                                   \
        v.revisit(this, n);                                                                        \
    }                                                                                              \
    TEMPLATE INLINE void IR::CLASS TT::apply_visitor_loop_revisit(COWTransform &v) const {         \
        Node::traceVisit("Trans loop_revisit (COW)");                                              \
        v.loop_revisit(this);                                                                      \
    }

IRNODE_ALL_TEMPLATES(DEFINE_APPLY_FUNCTIONS, inline)

template <class T>
void IR::merge_into_safe_vector(safe_vector<const T *> &rv, const IR::Node *n, const char *clss) {
    if (n) {
        if (auto l = n->to<Vector<T>>()) {
            rv.insert(rv.end(), l->begin(), l->end());
        } else if (const auto *v = n->to<VectorBase>()) {
            for (const auto *el : *v) {
                CHECK_NULL(el);
                if (auto e = el->template to<T>()) {
                    rv.push_back(e);
                } else {
                    BUG("visitor returned invalid type %s for %s<%s>", el->node_type_name(), clss,
                        T::static_type_name());
                }
            }
        } else if (auto e = n->to<T>()) {
            rv.push_back(e);
        } else {
            BUG("visitor returned invalid type %s for %s<%s>", n->node_type_name(), clss,
                T::static_type_name());
        }
    }
}
template <class T>
safe_vector<const T *> IR::visit_safe_vector(const safe_vector<const T *> &vec, const char *clss,
                                             Visitor &v, const char *name) {
    safe_vector<const T *> rv;
    for (const T *el : vec) {
        const IR::Node *n = v.apply_visitor(el, name);
        merge_into_safe_vector(rv, n, clss);
    }
    return rv;
}

template <class T>
void IR::Vector<T>::visit_children(Visitor &v, const char *name) {
    auto res = visit_safe_vector(vec, "Vector", v, name);
    if (res != vec) replace_contents(std::move(res));
}
template <class T>
void IR::Vector<T>::visit_children(Visitor &v, const char *name) const {
    for (auto &a : vec) v.visit(a, name);
}
template <class T>
void IR::Vector<T>::COWref_visit_children(COWNode_info *info, Visitor &v, const char *name) const {
    IR::COWinfo<IR::Vector<T>> *p = static_cast<IR::COWinfo<IR::Vector<T>> *>(info);
    auto res = visit_safe_vector(this->get_contents(), "Vector", v, name);
    if (res != p->get()->get_contents()) p->mkClone()->replace_contents(std::move(res));
}

template <class T>
void IR::Vector<T>::parallel_visit_children(Visitor &v, const char *) {
    SplitFlowVisitVector<IR::Vector<T>, T>(v, *this).run_visit();
}
template <class T>
void IR::Vector<T>::parallel_visit_children(Visitor &v, const char *) const {
    SplitFlowVisitVector<IR::Vector<T>, T>(v, *this).run_visit();
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
    auto res = visit_safe_vector(this->get_contents(), "IndexedVector", v, name);
    if (res != this->get_contents()) replace_contents(std::move(res));
}
template <class T>
void IR::IndexedVector<T>::visit_children(Visitor &v, const char *name) const {
    for (auto &a : *this) v.visit(a, name);
}
template <class T>
void IR::IndexedVector<T>::COWref_visit_children(COWNode_info *info, Visitor &v,
                                                 const char *name) const {
    IR::COWinfo<IR::Vector<T>> *p = static_cast<IR::COWinfo<IR::Vector<T>> *>(info);
    auto res = visit_safe_vector(this->get_contents(), "IndexedVector", v, name);
    if (res != p->get()->get_contents()) p->mkClone()->replace_contents(std::move(res));
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
auto IR::NameMap<T, MAP, COMP, ALLOC>::visit_symbols(Visitor &v, const char *) const -> map_t {
    map_t new_symbols;
    for (auto [name_, sym] : symbols) {
        // gcc bug?  auto seems to infer a const type for name_, which is wrong.  Second
        // auto correctly infers non-const type.
        auto name = name_;
        const IR::Node *n = v.apply_visitor(sym, name.c_str());
        if (!n) continue;
        if (auto s = n->to<T>()) {
            if (!match_name(name, s)) name = obj_name(s);
            new_symbols.emplace(name, s);
        } else if (auto m = n->to<NameMap>()) {
            for (auto [n2, s2] : *m) new_symbols.emplace(n2, s2);
        } else {
            BUG("visitor returned invalid type %s for NameMap<%s>", n->node_type_name(),
                T::static_type_name());
        }
    }
    return new_symbols;
}
template <class T, template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<const cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::visit_children(Visitor &v, const char *name) {
    map_t res = visit_symbols(v, name);
    if (!match_contents(res)) replace_contents(std::move(res));
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
          class ALLOC /*= std::allocator<std::pair<const cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::COWref_visit_children(COWNode_info *info, Visitor &v,
                                                             const char *name) const {
    auto *p = static_cast<IR::COWinfo<IR::NameMap<T, MAP, COMP, ALLOC>> *>(info);
    map_t res = visit_symbols(v, name);
    if (!p->get()->match_contents(res)) p->mkClone()->replace_contents(std::move(res));
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
