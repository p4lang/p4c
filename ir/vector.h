#ifndef _IR_VECTOR_H_
#define _IR_VECTOR_H_

#include "dbprint.h"
#include "lib/enumerator.h"
#include "lib/null.h"

namespace IR {


// Specialization of vector which
// - only stores const IR::Node* objects inside (T should derive from Node)
// - inherits from IR::Node itself

class VectorBase : public Node {
 public:
    typedef const Node * const *iterator;
    virtual iterator VectorBase_begin() const = 0;
    virtual iterator VectorBase_end() const = 0;
    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
    iterator begin() const { return VectorBase_begin(); }
    iterator end() const { return VectorBase_end(); }
};

template<class T>
class Vector : public VectorBase {
    vector<const T *>   vec;

 public:
    Vector() = default;
    Vector(const Vector &) = default;
    Vector(Vector &&) = default;
    Vector &operator=(const Vector &) = default;
    Vector &operator=(Vector &&) = default;
    explicit Vector(const T *a) {
        vec.emplace_back(std::move(a)); }
    explicit Vector(const vector<const T *> &a) {
        vec.insert(vec.end(), a.begin(), a.end()); }

    typedef typename vector<const T *>::iterator        iterator;
    typedef typename vector<const T *>::const_iterator  const_iterator;
    iterator begin() { return vec.begin(); }
    const_iterator begin() const { return vec.begin(); }
    VectorBase::iterator VectorBase_begin() const {
        /* DANGER -- works as long as IR::Node is the first ultimate base class of T */
        return reinterpret_cast<VectorBase::iterator>(&vec[0]); }
    iterator end() { return vec.end(); }
    const_iterator end() const { return vec.end(); }
    VectorBase::iterator VectorBase_end() const {
        /* DANGER -- works as long as IR::Node is the first ultimate base class of T */
        return reinterpret_cast<VectorBase::iterator>(&vec[0] + vec.size()); }
    std::reverse_iterator<iterator> rbegin() { return vec.rbegin(); }
    std::reverse_iterator<const_iterator> rbegin() const { return vec.rbegin(); }
    std::reverse_iterator<iterator> rend() { return vec.rend(); }
    std::reverse_iterator<const_iterator> rend() const { return vec.rend(); }
    size_t size() const { return vec.size(); }
    bool empty() const { return vec.empty(); }
    const T* const & front() const { return vec.front(); }
    const T*& front() { return vec.front(); }
    void clear() { vec.clear(); }
    iterator erase(iterator i) { return vec.erase(i); }
    template<typename ForwardIter>
    iterator insert(iterator i, ForwardIter b, ForwardIter e) {
        /* FIXME -- gcc prior to 4.9 is broken and the insert routine returns void
         * FIXME -- rather than an iterator.  So we recalculate it from an index */
        int index = i - vec.begin();
        vec.insert(i, b, e);
        return vec.begin() + index; }
    iterator append(const Vector<T>& toAppend)
    { return insert(end(), toAppend.begin(), toAppend.end()); }
    iterator insert(iterator i, const T* v) {
        /* FIXME -- gcc prior to 4.9 is broken and the insert routine returns void
         * FIXME -- rather than an iterator.  So we recalculate it from an index */
        int index = i - vec.begin();
        vec.insert(i, v);
        return vec.begin() + index; }
    iterator insert(iterator i, size_t n, const T* v) {
        /* FIXME -- gcc prior to 4.9 is broken and the insert routine returns void
         * FIXME -- rather than an iterator.  So we recalculate it from an index */
        int index = i - vec.begin();
        vec.insert(i, n, v);
        return vec.begin() + index; }

    const T *const &operator[](size_t idx) const { return vec[idx]; }
    const T *&operator[](size_t idx) { return vec[idx]; }
    const T *const &at(size_t idx) const { return vec.at(idx); }
    const T *&at(size_t idx) { return vec.at(idx); }
    template <class... Args> void emplace_back(Args&&... args) {
        vec.emplace_back(new T(std::forward<Args>(args)...)); }
    void push_back(T *a) { vec.push_back(a); }
    void push_back(const T *a) { vec.push_back(a); }
    void pop_back() { vec.pop_back(); }
    const T* const & back() const { return vec.back(); }
    const T*& back() { return vec.back(); }
    template<class U> void push_back(U &a) { vec.push_back(a); }
    void check_null() const { for (auto e : vec) CHECK_NULL(e); }

    IRNODE_SUBCLASS(Vector)
    IRNODE_DECLARE_APPLY_OVERLOAD(Vector)
    bool operator==(const Node &a) const override { return a == *this; }
    bool operator==(const Vector &a) const { return vec == a.vec; }
    cstring node_type_name() const override {
        return "Vector<" + T::static_type_name() + ">"; }
    static cstring static_type_name() {
        return "Vector<" + T::static_type_name() + ">"; }
    void visit_children(Visitor &v) override;
    void visit_children(Visitor &v) const override;
    Util::Enumerator<const T*>* getEnumerator() const {
        return Util::Enumerator<const T*>::createEnumerator(vec); }
    template <typename S>
    Util::Enumerator<const S*>* only() const {
        std::function<bool(const T*)> filter = [](const T* d) { return d->template is<S>(); };
        return getEnumerator()->where(filter)->template as<const S*>(); }
};

}  // namespace IR

template<class T, class U> const T *get(const IR::Vector<T> &vec, U name) {
    for (auto el : vec)
        if (el->name == name)
            return el;
    return nullptr; }
template<class T, class U> const T *get(const IR::Vector<T> *vec, U name) {
    if (vec)
        for (auto el : *vec)
            if (el->name == name)
                return el;
    return nullptr; }

#endif /* _IR_VECTOR_H_ */
