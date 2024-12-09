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

#ifndef IR_COPY_ON_WRITE_PTR_H_
#define IR_COPY_ON_WRITE_PTR_H_

#include "lib/exceptions.h"

namespace P4 {

class COWModifier;
class COWTransform;
class Visitor;
class Visitor_Context;

namespace IR {

class Node;

template<typename T>
concept COWref = requires(T r) {
    r.set(r.get());
};

class COWNode_info {
 protected:
    const Node *orig;
    Node *clone;
    Visitor_Context *ctxt;

    COWNode_info() = delete;
    COWNode_info(const COWNode_info &) = delete;
    COWNode_info(const Node *n, Visitor_Context *c) : orig(n), clone(nullptr), ctxt(c) {}
};

template<class T> class COWinfo : public COWNode_info {
 public:
    const T *get() const;
    const T *getOrig() const;
    T *mkClone();
    T *getClone() const;
    bool isCloned() const { return clone != nullptr; }
 private:
    friend COWModifier;
    friend COWTransform;
    COWinfo(const T *n, Visitor_Context *c) : COWNode_info(n, c) {}
};

template<class T, typename U, U T::*field> struct COWfieldref {
    COWinfo<T> *info;

    const U &get() const { return *info->get(); }
    U &modify() const { return *info->mkClone(); }
    operator const U&() const { return get(); }
    const U &operator=(const U &val) const {
        if (!info->isCloned() && info->get()->*field == val) return val;
        return info->mkClone()->*field = val;
    }
    const U &operator=(U &&val) const {
        if (!info->isCloned() && info->get()->*field == val) return val;
        return info->mkClone()->*field = std::move(val);
    }
    void set(const U &val) const { *this = val; }
};

template<class T> class COWptr {
    COWinfo<T> *info;

    template<class U> friend class COWptr;
    friend COWModifier;
    friend COWTransform;
    friend T;
    COWptr(COWinfo<T> *p) : info(p) {}
    COWptr(COWNode_info *p) : info(static_cast<COWinfo<T> *>(p)) {
        BUG_CHECK(info->get()->template is<T>(), "incorrect type in COWptr ctor");
    }

public:
    COWptr() = default;
    COWptr(const COWptr &) = default;
    COWptr(COWptr &&) = default;
    template<typename U> requires std::derived_from<U, T>
    COWptr(const COWptr<U> &a) : COWptr(a.info) {}
    ~COWptr() = default;
    COWptr &operator=(const COWptr &) = default;
    COWptr &operator=(COWptr &&) = default;

    operator const T *() const { return info->get(); }
    typename T::COWref operator->() const { return typename T::COWref(info); }
    typename T::COWref operator*() const { return typename T::COWref(info); }
    void visit_children(Visitor &v) const { info->get()->COW_visit_children(info, v); }
    bool apply_visitor_preorder(COWModifier &v) const { 
        return info->get()->apply_visitor_preorder(info, v); }
    void apply_visitor_postorder(COWModifier &v) const { 
        info->get()->apply_visitor_postorder(info, v); }
    const IR::Node *apply_visitor_preorder(COWTransform &v) const { 
        return info->get()->apply_visitor_preorder(info, v); }
    const IR::Node *apply_visitor_postorder(COWTransform &v) const { 
        return info->get()->apply_visitor_postorder(info, v); }
};

}

}

#endif  /* IR_COPY_ON_WRITE_PTR_H_ */
