/*
Copyright 2024 NVIDIA CORPORATION.

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

#ifndef IR_COPY_ON_WRITE_INL_H_
#define IR_COPY_ON_WRITE_INL_H_

/* template methods declared in "copy_on_write_ptr.h" that can't be defined there
 * due to order-of-declaration issues. */

// Really only need Visitor::Context here -- split that out into a separate header?
#include "ir/visitor.h"

namespace P4::IR {

template <class T>
const T *COWinfo<T>::get() const {
    if (clone) return clone->checkedTo<T>();
    return orig->checkedTo<T>();
}

template <class T>
const T *COWinfo<T>::getOrig() const {
    return orig->checkedTo<T>();
}

template <class T>
T *COWinfo<T>::mkClone() {
    if (!clone) ctxt->node = clone = orig->clone();
    return clone->checkedTo<T>();
}

template <class T>
T *COWinfo<T>::getClone() const {
    BUG_CHECK(clone != nullptr, "Not yet cloned in getClone");
    return clone->checkedTo<T>();
}

template <class T, class C, class U, C T::*field>
struct COW_element_ref {
    COWinfo<T> *info;
    bool is_const;
    union {
        typename C::const_iterator ci;
        typename C::iterator ni;
    };
    COW_element_ref(COWinfo<T> *inf, typename C::const_iterator i) : info(inf), is_const(true) {
        ci = i;
    }
    COW_element_ref(COWinfo<T> *inf, typename C::iterator i) : info(inf), is_const(false) {
        ni = i;
    }
    void clone_fixup() {
        if (is_const) {
            // messy problem -- need to clone (iff not yet cloned) and then find the
            // corresponding iterator in the clone
            auto i = (info->mkClone()->*field).begin();
            auto &orig_vec = info->getOrig()->*field;
            for (auto oi = orig_vec.begin(); oi != ci; ++oi, ++i)
                BUG_CHECK(oi != orig_vec.end(), "Invalid iterator in clone_fixup");
            ni = i;
            is_const = false;
        }
    }
    U get() {
        if (is_const && info->isCloned()) clone_fixup();
        return *ci;
    }
    operator U() { return get(); }
    U operator=(U val) {
        clone_fixup();
        return *ni = val;
    }
    void set(U val) {
        clone_fixup();
        *ni = val;
    }
    void visit_children(Visitor &v, const char *name) { get().visit_children(v, name); }
};

template <class T, class C, class U, C T::*field>
struct COW_iterator {
    COW_element_ref<T, C, U, field> ref;
    COW_iterator(COWinfo<T> *inf, typename C::const_iterator i) : ref(inf, i) {}
    COW_iterator(COWinfo<T> *inf, typename C::iterator i) : ref(inf, i) {}
    COW_iterator &operator++() {
        ++ref.ci;
        return *this;
    }
    COW_iterator operator++(int) {
        COW_iterator<T, C, U, field> rv = *this;
        ++ref.ci;
        return rv;
    }
    COW_iterator &operator--() {
        --ref.ci;
        return *this;
    }
    COW_iterator operator--(int) {
        COW_iterator<T, C, U, field> rv = *this;
        --ref.ci;
        return rv;
    }
    bool operator==(COW_iterator<T, C, U, field> &i) {
        if (ref.is_const != i.ref.is_const) {
            ref.clone_fixup();
            i.ref.clone_fixup();
        }
        return ref.ci == i.ref.ci;
    }
    bool operator!=(COW_iterator<T, C, U, field> &i) {
        if (ref.is_const != i.ref.is_const) {
            ref.clone_fixup();
            i.ref.clone_fixup();
        }
        return ref.ci != i.ref.ci;
    }
    COW_element_ref<T, C, U, field> &operator*() { return ref; }
};

/* specialization for safe_vector */
template <class T, typename U, safe_vector<U> T::*field>
struct COWfieldref<T, safe_vector<U>, field> {
    COWinfo<T> *info;

    using iterator = COW_iterator<T, safe_vector<U>, U, field>;

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get(), "safe_vector", v, name);
        if (res != get()) modify() = std::move(res);
    }

    const safe_vector<U> &get() const { return info->get()->*field; }
    safe_vector<U> &modify() const { return info->mkClone()->*field; }
    operator const safe_vector<U> &() const { return get(); }
    safe_vector<U> &operator=(const safe_vector<U> &val) const { return modify() = val; }
    safe_vector<U> &operator=(safe_vector<U> &&val) const { return modify() = std::move(val); }
    iterator begin() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).begin());
        else
            return iterator(info, (info->get()->*field).begin());
    }
    iterator end() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).begin());
        else
            return iterator(info, (info->get()->*field).begin());
    }
    // FIXME need to add insert/appeand/prepend/emplace_back specializations
};

/* specializations for IR::Vector */
template <class T, typename U, Vector<U> T::*field>
struct COWfieldref<T, Vector<U>, field> {
    COWinfo<T> *info;

    using iterator = COW_iterator<T, Vector<U>, const U *, field>;

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get().get_contents(), "Vector", v, name);
        if (res != get().get_contents()) modify().replace_contents(std::move(res));
    }

    const Vector<U> &get() const { return info->get()->*field; }
    Vector<U> &modify() const { return info->mkClone()->*field; }
    operator const Vector<U> &() const { return get(); }
    Vector<U> &operator=(const Vector<U> &val) const { return modify() = val; }
    Vector<U> &operator=(Vector<U> &&val) const { return modify() = std::move(val); }
    iterator begin() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).begin());
        else
            return iterator(info, (info->get()->*field).begin());
    }
    iterator end() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).end());
        else
            return iterator(info, (info->get()->*field).end());
    }
    iterator erase(iterator i) {
        i.ref.clone_fixup();
        Vector<U> &vec = info->getClone()->*field;
        return iterator(info, vec.erase(i.ref.ni));
    }
    // FIXME need to add insert/appeand/prepend/emplace_back specializations
};

/* specializations for IR::IndexedVector */
template <class T, typename U, IndexedVector<U> T::*field>
struct COWfieldref<T, IndexedVector<U>, field> {
    COWinfo<T> *info;

    using iterator = COW_iterator<T, IndexedVector<U>, const U *, field>;

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get().get_contents(), "IndexedVector", v, name);
        if (res != get().get_contents()) modify().replace_contents(std::move(res));
    }

    const IndexedVector<U> &get() const { return info->get()->*field; }
    IndexedVector<U> &modify() const { return info->mkClone()->*field; }
    operator const IndexedVector<U> &() const { return get(); }
    IndexedVector<U> &operator=(const IndexedVector<U> &val) const { return modify() = val; }
    IndexedVector<U> &operator=(IndexedVector<U> &&val) const { return modify() = std::move(val); }
    iterator begin() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).begin());
        else
            return iterator(info, (info->get()->*field).begin());
    }
    iterator end() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).end());
        else
            return iterator(info, (info->get()->*field).end());
    }
    iterator erase(iterator i) {
        i.ref.clone_fixup();
        IndexedVector<U> &vec = info->getClone()->*field;
        return iterator(info, vec.erase(i.ref.ni));
    }
    // FIXME need to add insert/appeand/prepend/emplace_back/removeByName specializations
};

/* specializations for IR::NameMap */
template <class T, typename U, template <class K, class V, class COMP, class ALLOC> class MAP,
          NameMap<U, MAP> T::*field>
struct COWfieldref<T, NameMap<U, MAP>, field> {
    COWinfo<T> *info;

    using iterator = COW_iterator<T, NameMap<U, MAP>, const U *, field>;

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = get().visit_symbols(v, name);
        if (!get().match_contents(res)) modify().replace_contents(std::move(res));
    }

    const NameMap<U, MAP> &get() const { return info->get()->*field; }
    NameMap<U, MAP> &modify() const { return info->mkClone()->*field; }
    operator const NameMap<U, MAP> &() const { return get(); }
    NameMap<U, MAP> &operator=(const NameMap<U, MAP> &val) const { return modify() = val; }
    NameMap<U, MAP> &operator=(NameMap<U, MAP> &&val) const { return modify() = std::move(val); }
    iterator begin() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).begin());
        else
            return iterator(info, (info->get()->*field).begin());
    }
    iterator end() {
        if (info->isCloned())
            return iterator(info, (info->getClone()->*field).end());
        else
            return iterator(info, (info->get()->*field).end());
    }
    iterator erase(iterator i) {
        i.ref.clone_fixup();
        NameMap<U, MAP> &nm = info->getClone()->*field;
        return iterator(info, nm.erase(i.ref.ni));
    }
    // FIXME need to add insert/appeand/prepend/emplace_back/removeByName specializations
};

// FIXME -- need NodeMap specializations if any backend ever uses that template

#if 0
/* specialization for std::variant */
template <class T, class... TYPES, std::variant<TYPES...> T::*field>
struct COWfieldref<T, std::variant<TYPES...>, field> {
    COWinfo<T>        *info;

    size_t index() const { return info->get()->*field.index(); }
    // FIXME -- what do we need here?  Do we need a specialization of std::visit?
};
#endif

}  // namespace P4::IR

#endif /* IR_COPY_ON_WRITE_INL_H_ */
