/*
Copyright 2022-present Barefoot Networks, Inc.

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

#ifndef IR_SHARED_PTR_H_
#define IR_SHARED_PTR_H_

#include <atomic>
#include <memory>

#include "lib/exceptions.h"
#include "lib/hash.h"

namespace P4::IR {

template <class T>
class shared_ptr;

class shared_ptr_base {
    template <class T>
    friend class shared_ptr;
    mutable std::atomic<uint32_t> refcount;
    bool not_on_heap;
    static void *last_alloc;

 public:
    void *operator new(size_t size) {
        BUG_CHECK(last_alloc == nullptr, "Failed to catch IR::Node heap allocation");
        last_alloc = ::operator new(size);
        return last_alloc;
    }
    void *operator new(size_t, void *ptr) {
        BUG_CHECK(last_alloc == nullptr, "Failed to catch IR::Node heap allocation");
        return ptr;
    }
// FIXME: Remove this #ifdefine check once we switch to C++20
#if defined(__cpp_sized_deallocation) && __cpp_sized_deallocation >= 201309L
    void operator delete(void *p, size_t size) { return ::operator delete(p, size); }
#else
    void operator delete(void *p) { return ::operator delete(p); }
#endif

    shared_ptr_base() : refcount(0) {
        not_on_heap = last_alloc == nullptr;
        last_alloc = nullptr;
    }
    shared_ptr_base(const shared_ptr_base &) : shared_ptr_base() {}        // copying ignores refcnt
    shared_ptr_base(shared_ptr_base &&) : shared_ptr_base() {}             // moving ignores refcnt
    shared_ptr_base &operator=(const shared_ptr_base &) { return *this; }  // copying ignores refcnt
    shared_ptr_base &operator=(shared_ptr_base &&) { return *this; }       // moving ignores refcnt

    // to be used only for BUG_CHECKs checking for proper referencing
    bool check_referenced() const { return refcount > 0; }

 protected:
    const char *dbheap() const { return not_on_heap ? "" : " (heap)"; }
};

/* Shared (reference counted) pointer type for IR classes -- not using std::shared_ptr as
 * that works poorly with pointers to objects not allocated on the heap (allocated statically
 * or on the stack, or as part of another object.)  We intercept IR::INode::operator new to know
 * when Nodes are allocated on the heap or elsewhere */

template <class T>
class shared_ptr {
    // FIXME -- static assert fails due to circular uses causing invalid incomplete type errors
    // static_assert(std::is_base_of<shared_ptr_base, T>::value,
    //               "IR::shared_ptr only usable on subclasses of IR::shared_ptr_base");
    T *ptr;
    template <class U>
    friend class shared_ptr;

 public:
    typedef T element_type;
    shared_ptr() : ptr(nullptr) {}
    shared_ptr(nullptr_t) : ptr(nullptr) {}  // NOLINT(runtime/explicit)
    shared_ptr(const shared_ptr &a) {
        if ((ptr = a.ptr)) ptr->refcount++;
    }
    shared_ptr(shared_ptr &&a) {
        ptr = a.ptr;
        a.ptr = nullptr;
    }
    template <class U,
              typename = typename std::enable_if<std::is_constructible<T *, U *>::value>::type>
    shared_ptr(const shared_ptr<U> &a) {
        if ((ptr = a.ptr)) ptr->refcount++;
    }
    template <class U,
              typename = typename std::enable_if<std::is_constructible<T *, U *>::value>::type>
    shared_ptr(U *a) {  // NOLINT(runtime/explicit)
        if ((ptr = a)) a->refcount++;
    }
    shared_ptr<T> &operator=(const shared_ptr<T> &a) {
        if (ptr == a.ptr) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a.ptr)) ptr->refcount++;
        return *this;
    }
    shared_ptr<T> &operator=(shared_ptr<T> &&a) { return swap(a); }
    template <class U,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    shared_ptr<T> &operator=(const shared_ptr<U> &a) {
        if (ptr == a.ptr) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a.ptr)) ptr->refcount++;
        return *this;
    }
    shared_ptr<T> &operator=(T *a) {
        if (ptr == a) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a)) ptr->refcount++;
        return *this;
    }
    template <class U,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    shared_ptr<T> &operator=(U *a) {
        if (ptr == a) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a)) a->refcount++;
        return *this;
    }
    shared_ptr<T> &operator=(nullptr_t) {
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        ptr = nullptr;
        return *this;
    }
    ~shared_ptr() {
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
    }

    shared_ptr<T> &swap(shared_ptr<T> &a) {
        std::swap(ptr, a.ptr);
        return *this;
    }
    T *get() const { return ptr; }
    T *operator->() const { return ptr; }
    T &operator*() const { return *ptr; }
    operator T *() const { return ptr; }
    template <class U,
              typename = typename std::enable_if<std::is_convertible<T *, U *>::value>::type>
    operator U *() const {
        return ptr;
    }
    bool operator==(nullptr_t) const { return ptr == nullptr; }
    bool operator!=(nullptr_t) const { return ptr != nullptr; }
    explicit operator bool() const { return ptr != nullptr; }

    friend std::ostream &operator<<(std::ostream &out, const shared_ptr<T> &v) {
        return out << v.ptr;
    }
};

}  // namespace P4::IR

// FIXME -- if I put this in namespace P4::IR, then ADL fails to find it.  Why?
// it only seems to work if it is in the global scope
template <class T, class U>
inline T dynamic_pointer_cast(const P4::IR::shared_ptr<U> &r) noexcept {
    return T(dynamic_cast<typename T::element_type *>(r.get()));
}

namespace P4::Util {

template <typename T>
struct Hasher<P4::IR::shared_ptr<T>> {
    size_t operator()(const P4::IR::shared_ptr<T> &val) const { return Hash()(val.get()); }
};

template <typename T>
auto toString(const P4::IR::shared_ptr<T> &val) {
    return val->toString();
}

}  // namespace P4::Util

#endif /* IR_SHARED_PTR_H_ */
