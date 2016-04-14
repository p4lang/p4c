#ifndef _IR_IR_INLINE_H_
#define _IR_IR_INLINE_H_

#define DEFINE_EQUALS_IMPL(CLASS, BASE, TEMPLATE, TT)   \
    TEMPLATE                                                                                    \
    inline bool IR::CLASS TT::operator ==(const IR::Node &n) const {                            \
        if (this == &n) return true;                                                            \
        if (typeid(*this) != typeid(n)) return false;                                           \
        return *this == dynamic_cast<const CLASS&>(n); }
    IRNODE_ALL_NON_TEMPLATE_SUBCLASSES(DEFINE_EQUALS_IMPL, , )
    IRNODE_ALL_TEMPLATES_AND_BASES(DEFINE_EQUALS_IMPL)
#undef DEFINE_EQUALS_IMPL

#define DEFINE_APPLY_FUNCTIONS(CLASS, BASE, TEMPLATE, TT)                               \
    TEMPLATE inline bool IR::CLASS TT::apply_visitor_preorder(Modifier &v)              \
    { traceVisit("Mod pre"); return v.preorder(this); }                                 \
    TEMPLATE inline void IR::CLASS TT::apply_visitor_postorder(Modifier &v)             \
    { traceVisit("Mod post"); v.postorder(this); }                                      \
    TEMPLATE inline bool IR::CLASS TT::apply_visitor_preorder(Inspector &v) const       \
    { traceVisit("Insp pre"); return v.preorder(this); }                                \
    TEMPLATE inline void IR::CLASS TT::apply_visitor_postorder(Inspector &v) const      \
    { traceVisit("Insp post"); v.postorder(this); }                                     \
    TEMPLATE inline const IR::Node *IR::CLASS TT::apply_visitor_preorder(Transform &v)  \
    { traceVisit("Trans pre"); return v.preorder(this); }                               \
    TEMPLATE inline const IR::Node *IR::CLASS TT::apply_visitor_postorder(Transform &v) \
    { traceVisit("Trans post"); return v.postorder(this); }
    IRNODE_ALL_NON_TEMPLATE_SUBCLASSES(DEFINE_APPLY_FUNCTIONS, , )
    IRNODE_ALL_TEMPLATES_AND_BASES(DEFINE_APPLY_FUNCTIONS)
    DEFINE_APPLY_FUNCTIONS(Node, , , )
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

template<class T, template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
void IR::NameMap<T, MAP, COMP, ALLOC>::visit_children(Visitor &v) {
    map_t   new_symbols;
    for (auto i = symbols.begin(); i != symbols.end();) {
        auto n = v.apply_visitor(i->second, i->first);
        if (!n) {
            i = symbols.erase(i);
        } else if (n == i->second) {
            i++;
        } else if (auto m = dynamic_cast<const NameMap *>(n)) {
            new_symbols.insert(m->symbols.begin(), m->symbols.end());
            i = symbols.erase(i);
        } else if (auto s = dynamic_cast<const T *>(n)) {
            if (match_name(i->first, s)) {
                i->second = s;
                i++;
            } else {
                new_symbols.emplace(obj_name(s), std::move(s));
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
