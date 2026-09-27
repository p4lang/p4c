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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <ostream>
#include <utility>

#include "lib/hash.h"

namespace P4::IR {

template <class T>
class shared_ptr;

class shared_ptr_base {
    template <class T>
    friend class shared_ptr;
    mutable std::atomic<uint32_t> refcount;
    bool not_on_heap;
    static void recordAllocation(void *address, size_t size);
    static void forgetAllocation(void *address) noexcept;
    static bool consumeAllocation(const void *base);

 public:
    static void *operator new(size_t size);
    static void *operator new(size_t size, std::align_val_t alignment);
    static void *operator new(size_t, void *ptr) noexcept { return ptr; }
    static void operator delete(void *ptr) noexcept;
    static void operator delete(void *ptr, std::align_val_t alignment) noexcept;
    static void operator delete(void *, void *) noexcept {}

    shared_ptr_base() : refcount(0), not_on_heap(!consumeAllocation(this)) {}
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
    shared_ptr(std::nullptr_t) : ptr(nullptr) {}  // NOLINT(runtime/explicit)
    shared_ptr(const shared_ptr &a) {
        if ((ptr = a.ptr)) ptr->refcount++;
    }
    shared_ptr(shared_ptr &&a) noexcept {
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
        // Retain first: a can be a field of the object currently owned by *this.
        shared_ptr{a}.swap(*this);
        return *this;
    }
    shared_ptr<T> &operator=(shared_ptr<T> &&a) noexcept {
        shared_ptr{std::move(a)}.swap(*this);
        return *this;
    }
    template <class U,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    shared_ptr<T> &operator=(const shared_ptr<U> &a) {
        shared_ptr{a}.swap(*this);
        return *this;
    }
    shared_ptr<T> &operator=(T *a) {
        shared_ptr{a}.swap(*this);
        return *this;
    }
    template <class U,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    shared_ptr<T> &operator=(U *a) {
        shared_ptr{a}.swap(*this);
        return *this;
    }
    shared_ptr<T> &operator=(std::nullptr_t) {
        shared_ptr().swap(*this);
        return *this;
    }
    ~shared_ptr() {
        if (ptr && --ptr->refcount == 0 && !ptr->not_on_heap) delete ptr;
    }

    shared_ptr<T> &swap(shared_ptr<T> &a) noexcept {
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
    bool operator==(std::nullptr_t) const { return ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr != nullptr; }
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
