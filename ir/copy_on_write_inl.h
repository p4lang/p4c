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
#include "lib/ordered_map.h"

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

/* COW reference to an element from a container.  Used by COWfieldref specializations
 * of safe_vector, std::vector, Vector, IndexedVector, and NameMap */
template <class T, class C, class U, class COW>
struct COW_element_ref : public COWref_base<T, U, COW_element_ref<T, C, U, COW>> {
    union {
        COWinfo<T> *info;
        COW ref;
    };
    mutable bool is_const;
    union {
        typename C::const_iterator ci;
        mutable typename C::iterator ni;
    };
    COW_element_ref(COW r, typename C::const_iterator i) : ref(r), is_const(true), ci(i) {}
    COW_element_ref(COW r, typename C::iterator i) : ref(r), is_const(false), ni(i) {}
    void clone_fixup() const {
        if (is_const) {
            // messy problem -- need to clone (iff not yet cloned) and then find the
            // corresponding iterator in the clone
            auto i = ref.modify().begin();
            auto &orig_vec = ref.getOrig();
            for (auto oi = orig_vec.begin(); oi != ci; ++oi, ++i)
                BUG_CHECK(oi != orig_vec.end(), "Invalid iterator in clone_fixup");
            ni = i;
            is_const = false;
        }
    }
    using COWref_base<T, U, COW_element_ref<T, C, U, COW>>::operator=;
    const U &get() const {
        if (is_const && ref.isCloned()) clone_fixup();
        return *ci;
    }
    const U &getOrig() const {
        if (is_const) return *ci;
        BUG("getOrig on cloned COW_element_ref");
    }
    bool isCloned() const { return ref.isCloned(); }
    U &modify() const {
        clone_fixup();
        return *ni;
    }
};

template <class T, class C, class U, class COW>
struct COW_iterator {
    COW_element_ref<T, C, U, COW> ref;
    COW_iterator(COW r, typename C::const_iterator i) : ref(r, i) {}
    COW_iterator(COW r, typename C::iterator i) : ref(r, i) {}
    COW_iterator &operator++() {
        ++ref.ci;
        return *this;
    }
    COW_iterator operator++(int) {
        COW_iterator<T, C, U, COW> rv = *this;
        ++ref.ci;
        return rv;
    }
    COW_iterator &operator--() {
        --ref.ci;
        return *this;
    }
    COW_iterator operator--(int) {
        COW_iterator<T, C, U, COW> rv = *this;
        --ref.ci;
        return rv;
    }
    bool operator==(COW_iterator<T, C, U, COW> &i) {
        if (ref.is_const != i.ref.is_const) {
            ref.clone_fixup();
            i.ref.clone_fixup();
        }
        return ref.ci == i.ref.ci;
    }
    bool operator!=(COW_iterator<T, C, U, COW> &i) {
        if (ref.is_const != i.ref.is_const) {
            ref.clone_fixup();
            i.ref.clone_fixup();
        }
        return ref.ci != i.ref.ci;
    }
    COW_element_ref<T, C, U, COW> &operator*() { return ref; }
};

template <class T, class U, class REF, class ITER>
struct COW_iterableref_base : public COWref_base<T, U, REF> {
    using iterator = ITER;
    iterator begin() {
        REF *self = static_cast<REF *>(this);
        if (self->info->isCloned())
            return iterator(*self, self->modify().begin());
        else
            return iterator(*self, self->get().begin());
    }
    iterator end() {
        REF *self = static_cast<REF *>(this);
        if (self->info->isCloned())
            return iterator(*self, self->modify().end());
        else
            return iterator(*self, self->get().end());
    }
    iterator erase(iterator i) {
        REF *self = static_cast<REF *>(this);
        i.ref.clone_fixup();
        U &vec = self->modify();
        return iterator(*self, vec.erase(i.ref.ni));
    }
    // FIXME should add insert/append/prepend/emplace_back specializations
};

/* specialization for safe_vector */
template <class T, typename U, safe_vector<U> T::*field>
struct COWfieldref<T, safe_vector<U>, field>
    : public COW_iterableref_base<
          T, safe_vector<U>, COWfieldref<T, safe_vector<U>, field>,
          COW_iterator<T, safe_vector<U>, U, COWfieldref<T, safe_vector<U>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, safe_vector<U>, COWfieldref<T, safe_vector<U>, field>>::operator=;
    const safe_vector<U> &get() const { return info->get()->*field; }
    const safe_vector<U> &getOrig() const { return info->getOrig()->*field; }
    safe_vector<U> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get(), "safe_vector", v, name);
        if (res != get()) modify() = std::move(res);
    }
};

/* specializations for std::vector */
template <class T, typename U, std::vector<U> T::*field>
struct COWfieldref<T, std::vector<U>, field>
    : public COW_iterableref_base<
          T, std::vector<U>, COWfieldref<T, std::vector<U>, field>,
          COW_iterator<T, std::vector<U>, U, COWfieldref<T, std::vector<U>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, std::vector<U>, COWfieldref<T, std::vector<U>, field>>::operator=;
    const std::vector<U> &get() const { return info->get()->*field; }
    const std::vector<U> &getOrig() const { return info->getOrig()->*field; }
    std::vector<U> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get(), "vector", v, name);
        if (res != get()) modify() = std::move(res);
    }
};

template <class T, class U, class COW, class X, std::vector<U> X::*field>
struct COWref_subfield<T, std::vector<U>, COW, X, field>
    : public COW_iterableref_base<
          T, std::vector<U>, COWref_subfield<T, std::vector<U>, COW, X, field>,
          COW_iterator<T, std::vector<U>, U, COWref_subfield<T, std::vector<U>, COW, X, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, std::vector<U>,
                      COWref_subfield<T, std::vector<U>, COW, X, field>>::operator=;
    const std::vector<U> &get() const {
        return (reinterpret_cast<const COW *>(this)->get()).*field;
    }
    const std::vector<U> &getOrig() const {
        return (reinterpret_cast<const COW *>(this)->getOrig()).*field;
    }
    std::vector<U> &modify() const {
        return (reinterpret_cast<const COW *>(this)->modify()).*field;
    }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get(), "vector", v, name);
        if (res != get()) modify() = std::move(res);
    }
};

/* specializations for IR::Vector */
template <class T, typename U, Vector<U> T::*field>
struct COWfieldref<T, Vector<U>, field>
    : public COW_iterableref_base<
          T, Vector<U>, COWfieldref<T, Vector<U>, field>,
          COW_iterator<T, Vector<U>, const U *, COWfieldref<T, Vector<U>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, Vector<U>, COWfieldref<T, Vector<U>, field>>::operator=;
    const Vector<U> &get() const { return info->get()->*field; }
    const Vector<U> &getOrig() const { return info->getOrig()->*field; }
    Vector<U> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get().get_contents(), "Vector", v, name);
        if (res != get().get_contents()) modify().replace_contents(std::move(res));
    }
};

/* specializations for IR::IndexedVector */
template <class T, typename U, IndexedVector<U> T::*field>
struct COWfieldref<T, IndexedVector<U>, field>
    : public COW_iterableref_base<
          T, IndexedVector<U>, COWfieldref<T, IndexedVector<U>, field>,
          COW_iterator<T, IndexedVector<U>, const U *, COWfieldref<T, IndexedVector<U>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, IndexedVector<U>, COWfieldref<T, IndexedVector<U>, field>>::operator=;
    const IndexedVector<U> &get() const { return info->get()->*field; }
    const IndexedVector<U> &getOrig() const { return info->getOrig()->*field; }
    IndexedVector<U> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get().get_contents(), "IndexedVector", v, name);
        if (res != get().get_contents()) modify().replace_contents(std::move(res));
    }
};

/* specializations for IR::NameMap */
template <class T, typename U, template <class K, class V, class COMP, class ALLOC> class MAP,
          NameMap<U, MAP> T::*field>
struct COWfieldref<T, NameMap<U, MAP>, field>
    : public COW_iterableref_base<
          T, NameMap<U, MAP>, COWfieldref<T, NameMap<U, MAP>, field>,
          COW_iterator<T, NameMap<U, MAP>, const U *, COWfieldref<T, NameMap<U, MAP>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, NameMap<U, MAP>, COWfieldref<T, NameMap<U, MAP>, field>>::operator=;
    const NameMap<U, MAP> &get() const { return info->get()->*field; }
    const NameMap<U, MAP> &getOrig() const { return info->getOrig()->*field; }
    NameMap<U, MAP> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = get().visit_symbols(v, name);
        if (!get().match_contents(res)) modify().replace_contents(std::move(res));
    }
};

// FIXME -- need NodeMap specializations if any backend ever uses that template

template <class T, class C, class K, class V, class COW>
struct COW_map_value_ref : public COW_element_ref<T, C, std::pair<const K, V>, COW> {
    explicit COW_map_value_ref(COW_element_ref<T, C, std::pair<const K, V>, COW> ref)
        : COW_element_ref<T, C, std::pair<const K, V>, COW>(ref) {}
    void clone_fixup() const {
        if (this->is_const) {
            // messy problem -- need to clone (iff not yet cloned) and then find the
            // corresponding iterator in the clone.  If the element has been removed
            // in the clone, we end up returning end(), which may or may not be ok
            this->ni = this->ref.modified().find(this->ci->first);
            this->is_const = false;
        }
    }
    using COW_element_ref<T, C, std::pair<const K, V>, COW>::operator=;
    const V &get() const {
        if (this->is_const && this->isCloned()) clone_fixup();
        return this->ci->second;
    }
    const V &getOrig() const {
        if (this->is_const) return this->ci->second;
        BUG("getOrig on cloned COW_map_value_ref");
    }
    V &modify() const {
        clone_fixup();
        return this->ni->second;
    }
};

/* specializations for std::map */
template <class T, class KEY, class VAL, class COMP, class ALLOC,
          std::map<KEY, VAL, COMP, ALLOC> T::*field>
struct COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>
    : public COW_iterableref_base<
          T, std::map<KEY, VAL, COMP, ALLOC>,
          COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>,
          COW_iterator<T, std::map<KEY, VAL, COMP, ALLOC>, std::pair<const KEY, VAL>,
                       COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, std::map<KEY, VAL, COMP, ALLOC>,
                      COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>>::operator=;
    const std::map<KEY, VAL, COMP, ALLOC> &get() const { return info->get()->*field; }
    const std::map<KEY, VAL, COMP, ALLOC> &getOrig() const { return info->getOrig()->*field; }
    std::map<KEY, VAL, COMP, ALLOC> &modify() const { return info->mkClone()->*field; }

    using iterator = COW_iterator<T, std::map<KEY, VAL, COMP, ALLOC>, std::pair<const KEY, VAL>,
                                  COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>>;
    size_t count(KEY &k) const { return get().count(k); }
    COW_map_value_ref<T, std::map<KEY, VAL, COMP, ALLOC>, KEY, VAL,
                      COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>>
    at(KEY &k) const {
        iterator it(info, get().find(k));
        if (it == get().end()) get().at(k);  // throw exception
        return COW_map_value_ref<T, std::map<KEY, VAL, COMP, ALLOC>, KEY, VAL,
                                 COWfieldref<T, std::map<KEY, VAL, COMP, ALLOC>, field>>(*it);
    }
};

/* specializations for ordered_map */
template <class T, class KEY, class VAL, class COMP, class ALLOC,
          ordered_map<KEY, VAL, COMP, ALLOC> T::*field>
struct COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>
    : public COW_iterableref_base<
          T, ordered_map<KEY, VAL, COMP, ALLOC>,
          COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>,
          COW_iterator<T, ordered_map<KEY, VAL, COMP, ALLOC>, std::pair<const KEY, VAL>,
                       COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>>> {
    COWinfo<T> *info;
    using COWref_base<T, ordered_map<KEY, VAL, COMP, ALLOC>,
                      COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>>::operator=;
    const ordered_map<KEY, VAL, COMP, ALLOC> &get() const { return info->get()->*field; }
    const ordered_map<KEY, VAL, COMP, ALLOC> &getOrig() const { return info->getOrig()->*field; }
    ordered_map<KEY, VAL, COMP, ALLOC> &modify() const { return info->mkClone()->*field; }

    using iterator = COW_iterator<T, ordered_map<KEY, VAL, COMP, ALLOC>, std::pair<const KEY, VAL>,
                                  COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>>;
    size_t count(KEY &k) const { return get().count(k); }
    COW_map_value_ref<T, ordered_map<KEY, VAL, COMP, ALLOC>, KEY, VAL,
                      COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>>
    at(KEY &k) const {
        iterator it(info, get().find(k));
        if (it == get().end()) get().at(k);  // throw exception
        return COW_map_value_ref<T, ordered_map<KEY, VAL, COMP, ALLOC>, KEY, VAL,
                                 COWfieldref<T, ordered_map<KEY, VAL, COMP, ALLOC>, field>>(*it);
    }
};

#if 0
/* specialization for std::variant */
template <class T, class... TYPES, std::variant<TYPES...> T::*field>
struct COWfieldref<T, std::variant<TYPES...>, field> {
    COWinfo<T>        *info;

    size_t index() const { return info->get()->*field.index(); }
    // FIXME -- what do we need here?  Do we need a specialization of std::visit?
};
#endif

/* specialization for pairs */
template <class T, typename U, typename V, std::pair<U, V> T::*field>
struct COWfieldref<T, std::pair<U, V>, field>
    : public COWref_base<T, std::pair<U, V>, COWfieldref<T, std::pair<U, V>, field>> {
    union {
        COWinfo<T> *info;
        COWref_subfield<T, U, COWfieldref<T, std::pair<U, V>, field>, std::pair<U, V>,
                        &std::pair<U, V>::first>
            first;
        COWref_subfield<T, V, COWfieldref<T, std::pair<U, V>, field>, std::pair<U, V>,
                        &std::pair<U, V>::second>
            second;
    };
    using COWref_base<T, std::pair<U, V>, COWfieldref<T, std::pair<U, V>, field>>::operator=;
    const std::pair<U, V> &get() const { return info->get()->*field; }
    const std::pair<U, V> &getOrig() const { return info->getOrig()->*field; }
    std::pair<U, V> &modify() const { return info->mkClone()->*field; }
};

/* specialization for a vector<pair> element */
template <class T, class C, class U, class V, class COW>
struct COW_element_ref<T, C, std::pair<U, V>, COW>
    : public COWref_base<T, std::pair<U, V>, COW_element_ref<T, C, std::pair<U, V>, COW>> {
    union {
        COWinfo<T> *info;
        COW ref;
        COWref_subfield<T, U, COW_element_ref<T, C, std::pair<U, V>, COW>, std::pair<U, V>,
                        &std::pair<U, V>::first>
            first;
        COWref_subfield<T, V, COW_element_ref<T, C, std::pair<U, V>, COW>, std::pair<U, V>,
                        &std::pair<U, V>::second>
            second;
    };
    mutable bool is_const;
    union {
        typename C::const_iterator ci;
        mutable typename C::iterator ni;
    };
    COW_element_ref(COW r, typename C::const_iterator i) : ref(r), is_const(true), ci(i) {}
    COW_element_ref(COW r, typename C::iterator i) : ref(r), is_const(false), ni(i) {}
    void clone_fixup() const {
        if (is_const) {
            // messy problem -- need to clone (iff not yet cloned) and then find the
            // corresponding iterator in the clone
            auto i = ref.modified().begin();
            auto &orig_vec = ref.getOrig();
            for (auto oi = orig_vec.begin(); oi != ci; ++oi, ++i)
                BUG_CHECK(oi != orig_vec.end(), "Invalid iterator in clone_fixup");
            ni = i;
            is_const = false;
        }
    }
    using COWref_base<T, std::pair<U, V>, COW_element_ref<T, C, std::pair<U, V>, COW>>::operator=;
    const std::pair<U, V> &get() const {
        if (is_const && ref.isCloned()) clone_fixup();
        return *ci;
    }
    const std::pair<U, V> &getOrig() const {
        if (is_const) return *ci;
        BUG("getOrig on cloned COW_element_ref");
    }
    bool isCloned() const { return ref.isCloned(); }
    std::pair<U, V> &modify() const {
        clone_fixup();
        return *ni;
    }
};

/* COW reference to an element from an array.  Used by COWfieldref specializations for arrays */
// FIXME -- figure out how to combine/factor this with COW_element_ref
template <class T, typename U, size_t N, U (T::*field)[N]>
struct COW_array_element_ref : public COWref_base<T, U, COW_array_element_ref<T, U, N, field>> {
    COWinfo<T> *info;
    mutable bool is_const;
    union {
        const U *ci;
        mutable U *ni;
    };
    COW_array_element_ref(COWinfo<T> *inf, const U *i) : info(inf), is_const(true), ci(i) {}
    COW_array_element_ref(COWinfo<T> *inf, U *i) : info(inf), is_const(false), ni(i) {}
    void clone_fixup() const {
        if (is_const) {
            // messy problem -- need to clone (iff not yet cloned) and then find the
            // corresponding iterator in the clone
            U *i = info->mkClone()->*field;
            const U *orig_vec = info->getOrig()->*field;
            BUG_CHECK(ci >= orig_vec && ci <= orig_vec + N, "Invalid iterator in clone_fixup");
            ni = i + (ci - orig_vec);
            is_const = false;
        }
    }
    using COWref_base<T, U, COW_array_element_ref<T, U, N, field>>::operator=;
    const U &get() const {
        if (is_const && info->isCloned()) clone_fixup();
        return *ci;
    }
    const U &getOrig() const {
        if (is_const) return *ci;
        BUG("getOrig on cloned COW_array_element_ref");
    }
    U &modify() const {
        clone_fixup();
        return *ni;
    }
};

template <class T, typename U, size_t N, U (T::*field)[N]>
struct COW_array_iterator {
    COW_array_element_ref<T, U, N, field> ref;
    COW_array_iterator(COWinfo<T> *inf, const U *i) : ref(inf, i) {}
    COW_array_iterator(COWinfo<T> *inf, U *i) : ref(inf, i) {}
    COW_array_iterator &operator++() {
        ++ref.ci;
        return *this;
    }
    COW_array_iterator operator++(int) {
        COW_array_iterator<T, U, N, field> rv = *this;
        ++ref.ci;
        return rv;
    }
    COW_array_iterator &operator--() {
        --ref.ci;
        return *this;
    }
    COW_array_iterator operator--(int) {
        COW_array_iterator<T, U, N, field> rv = *this;
        --ref.ci;
        return rv;
    }
    bool operator==(COW_array_iterator<T, U, N, field> &i) {
        if (ref.is_const != i.ref.is_const) {
            ref.clone_fixup();
            i.ref.clone_fixup();
        }
        return ref.ci == i.ref.ci;
    }
    bool operator!=(COW_array_iterator<T, U, N, field> &i) {
        if (ref.is_const != i.ref.is_const) {
            ref.clone_fixup();
            i.ref.clone_fixup();
        }
        return ref.ci != i.ref.ci;
    }
    COW_array_element_ref<T, U, N, field> &operator*() { return ref; }
};

/* specialization for array */
template <class T, typename U, size_t N, U (T::*field)[N]>
struct COWfieldref<T, U[N], field>
    : public COW_iterableref_base<T, U[N], COWfieldref<T, U[N], field>,
                                  COW_array_iterator<T, U, N, field>> {
    typedef U array[N];
    COWinfo<T> *info;
    using COWref_base<T, U[N], COWfieldref<T, U[N], field>>::operator=;
    const array &get() const { return info->get()->*field; }
    const array &getOrig() const { return info->getOrig()->*field; }
    array &modify() const { return info->mkClone()->*field; }
};

}  // namespace P4::IR

#endif /* IR_COPY_ON_WRITE_INL_H_ */
