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

/* COW reference to an element from a container.  Used by COWfieldref specializations
 * of safe_vector, Vector, IndexedVector, and NameMap */
template <class T, class C, class U, C T::*field>
struct COW_element_ref
    : public COWref_base<T, U, COW_element_ref<T, C, U, field>>
{
    COWinfo<T> *info;
    mutable bool is_const;
    union {
        typename C::const_iterator ci;
        mutable typename C::iterator ni;
    };
    COW_element_ref(COWinfo<T> *inf, typename C::const_iterator i)
        : info(inf), is_const(true) {
        ci = i;
    }
    COW_element_ref(COWinfo<T> *inf, typename C::iterator i) : info(inf), is_const(false) {
        ni = i;
    }
    void clone_fixup() const {
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
    using COWref_base<T, U, COW_element_ref<T, C, U, field>>::operator=;
    const U &get() const {
        if (is_const && info->isCloned()) clone_fixup();
        return *ci;
    }
    U &modify() const {
        clone_fixup();
        return *ni;
    }
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

template <class T, class U, class REF, class ITER>
struct COW_iterableref_base : public COWref_base<T, U, REF> {
    using iterator = ITER;
    iterator begin() {
        REF *self = static_cast<REF *>(this);
        if (self->info->isCloned())
            return iterator(self->info, self->modify().begin());
        else
            return iterator(self->info, self->get().begin());
    }
    iterator end() {
        REF *self = static_cast<REF *>(this);
        if (self->info->isCloned())
            return iterator(self->info, self->modify().end());
        else
            return iterator(self->info, self->get().end());
    }
    iterator erase(iterator i) {
        REF *self = static_cast<REF *>(this);
        i.ref.clone_fixup();
        U &vec = self->modify();
        return iterator(self->info, vec.erase(i.ref.ni));
    }
    // FIXME should add insert/append/prepend/emplace_back specializations
};

/* specialization for safe_vector */
template <class T, typename U, safe_vector<U> T::*field>
struct COWfieldref<T, safe_vector<U>, field>
    : public COW_iterableref_base<T, safe_vector<U>, COWfieldref<T, safe_vector<U>, field>,
                                  COW_iterator<T, safe_vector<U>, U, field>> {
    COWinfo<T> *info;
    using COWref_base<T, safe_vector<U>, COWfieldref<T, safe_vector<U>, field>>::operator=;
    const safe_vector<U> &get() const { return info->get()->*field; }
    safe_vector<U> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get(), "safe_vector", v, name);
        if (res != get()) modify() = std::move(res);
    }
};

/* specializations for IR::Vector */
template <class T, typename U, Vector<U> T::*field>
struct COWfieldref<T, Vector<U>, field>
    : public COW_iterableref_base<T, Vector<U>, COWfieldref<T, Vector<U>, field>,
                                  COW_iterator<T, Vector<U>, const U *, field>> {
    COWinfo<T> *info;
    using COWref_base<T, Vector<U>, COWfieldref<T, Vector<U>, field>>::operator=;
    const Vector<U> &get() const { return info->get()->*field; }
    Vector<U> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = visit_safe_vector(get().get_contents(), "Vector", v, name);
        if (res != get().get_contents()) modify().replace_contents(std::move(res));
    }
};

/* specializations for IR::IndexedVector */
template <class T, typename U, IndexedVector<U> T::*field>
struct COWfieldref<T, IndexedVector<U>, field>
    : public COW_iterableref_base<T, IndexedVector<U>, COWfieldref<T, IndexedVector<U>, field>,
                                  COW_iterator<T, IndexedVector<U>, const U *, field>> {
    COWinfo<T> *info;
    using COWref_base<T, IndexedVector<U>, COWfieldref<T, IndexedVector<U>, field>>::operator=;
    const IndexedVector<U> &get() const { return info->get()->*field; }
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
    : public COW_iterableref_base<T, NameMap<U, MAP>, COWfieldref<T, NameMap<U, MAP>, field>,
                                  COW_iterator<T, NameMap<U, MAP>, const U *, field>> {
    COWinfo<T> *info;
    using COWref_base<T, NameMap<U, MAP>, COWfieldref<T, NameMap<U, MAP>, field>>::operator=;
    const NameMap<U, MAP> &get() const { return info->get()->*field; }
    NameMap<U, MAP> &modify() const { return info->mkClone()->*field; }

    void visit_children(Visitor &v, const char *name = nullptr) {
        auto res = get().visit_symbols(v, name);
        if (!get().match_contents(res)) modify().replace_contents(std::move(res));
    }
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

/* wrapper to allow modifications of a COWref.  Generally used to access fields of
 * a COWref of some composite */
template <class T, class U, class COW, class X, U X::*field>
//requires COWref<COW>
struct COWref_subfield : public COWref_base<T, U, COWref_subfield<T, U, COW, X, field>> {
    COWinfo<T> *info;
    using COWref_base<T, U, COWref_subfield<T, U, COW, X, field>>::operator=;
    const U &get() const { return (reinterpret_cast<const COW *>(this)->get()).*field; }
    U &modify() const { return (reinterpret_cast<const COW *>(this)->modify()).*field; }
};

/* specialization for pairs */
template <class T, typename U, typename V, std::pair<U, V> T::*field>
struct COWfieldref<T, std::pair<U, V>, field>
    : public COWref_base<T, std::pair<U, V>, COWfieldref<T, std::pair<U, V>, field>>
{
    union {
        COWinfo<T> *info;
        COWref_subfield<T, U, COWfieldref<T, std::pair<U, V>, field>, std::pair<U, V>, &std::pair<U, V>::first> first;
        COWref_subfield<T, V, COWfieldref<T, std::pair<U, V>, field>, std::pair<U, V>, &std::pair<U, V>::second> second;
    };
    using COWref_base<T, std::pair<U, V>, COWfieldref<T, std::pair<U, V>, field>>::operator=;
    const std::pair<U, V> &get() const { return info->get()->*field; }
    std::pair<U, V> &modify() const { return info->mkClone()->*field; }
};

/* specialization for a vector<pair> element */
template <class T, class C, class U, class V, C T::*field>
struct COW_element_ref<T, C, std::pair<U, V>, field>
    : public COWref_base<T, std::pair<U, V>, COW_element_ref<T, C, std::pair<U, V>, field>>
{
    union {
        COWinfo<T> *info;
        COWref_subfield<T, U, COW_element_ref<T, C, std::pair<U, V>, field>, std::pair<U, V>, &std::pair<U, V>::first> first;
        COWref_subfield<T, V, COW_element_ref<T, C, std::pair<U, V>, field>, std::pair<U, V>, &std::pair<U, V>::second> second;
    };
    mutable bool is_const;
    union {
        typename C::const_iterator ci;
        mutable typename C::iterator ni;
    };
    COW_element_ref(COWinfo<T> *inf, typename C::const_iterator i)
        : info(inf), is_const(true) {
        ci = i;
    }
    COW_element_ref(COWinfo<T> *inf, typename C::iterator i) : info(inf), is_const(false) {
        ni = i;
    }
    void clone_fixup() const {
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
    using COWref_base<T, std::pair<U, V>, COW_element_ref<T, C, std::pair<U, V>, field>>::operator=;
    const std::pair<U, V> &get() const {
        if (is_const && info->isCloned()) clone_fixup();
        return *ci;
    }
    std::pair<U, V> &modify() const {
        clone_fixup();
        return *ni;
    }
};

}  // namespace P4::IR

#endif /* IR_COPY_ON_WRITE_INL_H_ */
