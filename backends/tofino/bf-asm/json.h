/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_ASM_JSON_H_  //  NOLINT(build/header_guard)
#define BF_ASM_JSON_H_

#include <assert.h>

#include <cinttypes>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

#include "ordered_map.h"
#include "rvalue_reference_wrapper.h"

using namespace P4;

namespace json {

/* this is std::make_unique, except that is missing in some compilers/versions.  We give
 * it a different name as other compilers complain about ambiguities if we don't... */
template <class T, class... Args>
std::unique_ptr<T> mkuniq(Args &&...args) {
    std::unique_ptr<T> ret(new T(std::forward<Args>(args)...));
    return ret;
}

class number;
class string;
class vector;
class map;

class obj {
 public:
    obj() {}
    obj(const obj &) = default;
    obj(obj &&) = default;
    obj &operator=(const obj &) & = default;
    obj &operator=(obj &&) & = default;
    virtual ~obj() {}
    virtual bool operator<(const obj &a) const = 0;
    bool operator>=(const obj &a) const { return !(*this < a); }
    bool operator>(const obj &a) const { return a < *this; }
    bool operator<=(const obj &a) const { return !(a < *this); }
    virtual bool operator==(const obj &a) const = 0;
    bool operator!=(const obj &a) const { return !(*this == a); }
    virtual bool operator==(const char *str) const { return false; }
    virtual bool operator==(const std::string &str) const { return false; }
    virtual bool operator==(const string &str) const { return false; }
    bool operator!=(const char *str) const { return !(*this == str); }
    virtual bool operator==(int64_t val) const { return false; }
    bool operator!=(int64_t val) const { return !(*this == val); }
    struct ptrless {
        bool operator()(const obj *a, const obj *b) const { return b ? a ? *a < *b : true : false; }
        bool operator()(const std::unique_ptr<obj> &a, const std::unique_ptr<obj> &b) const {
            return b ? a ? *a < *b : true : false;
        }
    };
    virtual void print_on(std::ostream &out, int indent = 0, int width = 80,
                          const char *pfx = "") const = 0;
    virtual bool test_width(int &limit) const = 0;
    virtual number *as_number() { return nullptr; }
    virtual const number *as_number() const { return nullptr; }
    virtual string *as_string() { return nullptr; }
    virtual const string *as_string() const { return nullptr; }
    virtual vector *as_vector() { return nullptr; }
    virtual const vector *as_vector() const { return nullptr; }
    virtual map *as_map() { return nullptr; }
    virtual const map *as_map() const { return nullptr; }
    virtual const char *c_str() const { return nullptr; }
    template <class T>
    bool is() const {
        return dynamic_cast<const T *>(this) != nullptr;
    }
    template <class T>
    T &to() {
        return dynamic_cast<T &>(*this);
    }
    template <class T>
    const T &to() const {
        return dynamic_cast<const T &>(*this);
    }
    virtual std::unique_ptr<obj> copy() && = 0;      // Creates a shallow copy of unique_ptr
    virtual std::unique_ptr<obj> clone() const = 0;  // Creates a deep copy of obj
    static std::unique_ptr<obj> clone_ptr(const std::unique_ptr<obj> &a) {
        return a ? a->clone() : std::unique_ptr<obj>();
    }
    std::string toString() const;
};

class True : public obj {
    bool operator<(const obj &a) const {
        return std::type_index(typeid(*this)) < std::type_index(typeid(a));
    }
    bool operator==(const obj &a) const { return dynamic_cast<const True *>(&a) != 0; }
    void print_on(std::ostream &out, int indent = 0, int width = 80, const char *pfx = "") const {
        out << "true";
    }
    bool test_width(int &limit) const {
        limit -= 4;
        return limit >= 0;
    }
    std::unique_ptr<obj> copy() && { return mkuniq<True>(std::move(*this)); }
    std::unique_ptr<obj> clone() const { return mkuniq<True>(); }
};

class False : public obj {
    bool operator<(const obj &a) const {
        return std::type_index(typeid(*this)) < std::type_index(typeid(a));
    }
    bool operator==(const obj &a) const { return dynamic_cast<const False *>(&a) != 0; }
    void print_on(std::ostream &out, int indent = 0, int width = 80, const char *pfx = "") const {
        out << "false";
    }
    bool test_width(int &limit) const {
        limit -= 5;
        return limit >= 0;
    }
    std::unique_ptr<obj> copy() && { return mkuniq<False>(std::move(*this)); }
    std::unique_ptr<obj> clone() const { return mkuniq<False>(); }
};

class number : public obj {
 public:
    int64_t val;
    explicit number(int64_t l) : val(l) {}
    ~number() {}
    bool operator<(const obj &a) const override {
        if (auto *b = dynamic_cast<const number *>(&a)) return val < b->val;
        return std::type_index(typeid(*this)) < std::type_index(typeid(a));
    }
    bool operator==(const obj &a) const override {
        if (auto *b = dynamic_cast<const number *>(&a)) return val == b->val;
        return false;
    }
    bool operator==(int64_t v) const override { return val == v; }
    void print_on(std::ostream &out, int indent = 0, int width = 80,
                  const char *pfx = "") const override {
        out << val;
    }
    bool test_width(int &limit) const override {
        char buf[32];
        limit -= snprintf(buf, sizeof(buf), "%" PRId64, val);
        return limit >= 0;
    }
    number *as_number() override { return this; }
    const number *as_number() const override { return this; }
    std::unique_ptr<obj> copy() && override { return mkuniq<number>(std::move(*this)); }
    std::unique_ptr<obj> clone() const override { return mkuniq<number>(val); }
};

class string : public obj, public std::string {
 public:
    string() {}
    string(const string &) = default;
    string(const std::string &a) : std::string(a) {}  // NOLINT(runtime/explicit)
    string(const char *a) : std::string(a) {}         // NOLINT(runtime/explicit)
    string(string &&) = default;
    string(std::string &&a) : std::string(a) {}            // NOLINT
    string(int64_t l) : std::string(std::to_string(l)) {}  // NOLINT
    string &operator=(const string &) & = default;
    string &operator=(string &&) & = default;
    ~string() {}
    bool operator<(const obj &a) const override {
        if (const string *b = dynamic_cast<const string *>(&a))
            return static_cast<const std::string &>(*this) < static_cast<const std::string &>(*b);
        return std::type_index(typeid(*this)) < std::type_index(typeid(a));
    }
    bool operator==(const obj &a) const override {
        if (const string *b = dynamic_cast<const string *>(&a))
            return static_cast<const std::string &>(*this) == static_cast<const std::string &>(*b);
        return false;
    }
    bool operator==(const string &a) const override {
        return static_cast<const std::string &>(*this) == static_cast<const std::string &>(a);
    }
    bool operator==(const char *str) const override {
        return static_cast<const std::string &>(*this) == str;
    }
    bool operator==(const std::string &str) const override {
        return static_cast<const std::string &>(*this) == str;
    }
    void print_on(std::ostream &out, int indent = 0, int width = 80,
                  const char *pfx = "") const override {
        out << '"' << *this << '"';
    }
    bool test_width(int &limit) const override {
        limit -= size() + 2;
        return limit >= 0;
    }
    const char *c_str() const override { return std::string::c_str(); }
    string *as_string() override { return this; }
    const string *as_string() const override { return this; }
    std::unique_ptr<obj> copy() && override { return mkuniq<string>(std::move(*this)); }
    std::unique_ptr<obj> clone() const override { return mkuniq<string>(*this); }
};

class map;  // forward decl

typedef std::vector<std::unique_ptr<obj>> vector_base;
class vector : public obj, public vector_base {
 public:
    vector() {}
    vector(const vector &) = delete;
    vector(vector &&) = default;
    vector(const std::initializer_list<rvalue_reference_wrapper<obj>> &init) {
        for (auto o : init) push_back(o.get().copy());
    }
    vector &operator=(const vector &) & = delete;
    vector &operator=(vector &&) & = default;
    ~vector() {}
    bool operator<(const obj &a) const override {
        if (const vector *b = dynamic_cast<const vector *>(&a)) {
            auto p1 = begin(), p2 = b->begin();
            while (p1 != end() && p2 != b->end()) {
                if (**p1 < **p2) return true;
                if (**p1 != **p2) return false;
                p1++;
                p2++;
            }
            return p2 != b->end();
        }
        return std::type_index(typeid(*this)) < std::type_index(typeid(a));
    }
    using obj::operator<=;
    using obj::operator>=;
    using obj::operator>;
    bool operator==(const obj &a) const override {
        if (const vector *b = dynamic_cast<const vector *>(&a)) {
            auto p1 = begin(), p2 = b->begin();
            while (p1 != end() && p2 != b->end()) {
                if (**p1 != **p2) return false;
                p1++;
                p2++;
            }
            return (p1 == end() && p2 == b->end());
        }
        return false;
    }
    using obj::operator!=;
    void print_on(std::ostream &out, int indent = 0, int width = 80,
                  const char *pfx = "") const override;
    bool test_width(int &limit) const override {
        limit -= 2;
        for (auto &e : *this) {
            if (e ? !e->test_width(limit) : (limit -= 4) < 0) return false;
            if ((limit -= 2) < 0) return false;
        }
        return true;
    }
    using vector_base::push_back;
    void push_back(decltype(nullptr)) { push_back(std::unique_ptr<obj>()); }
    void push_back(bool t) {
        if (t)
            push_back(mkuniq<True>(True()));
        else
            push_back(mkuniq<False>(False()));
    }
    void push_back(int64_t n) { push_back(mkuniq<number>(number(n))); }
    void push_back(int n) { push_back((int64_t)n); }
    void push_back(unsigned int n) { push_back((int64_t)n); }
    void push_back(uint64_t n) { push_back((int64_t)n); }
    void push_back(const char *s) { push_back(mkuniq<string>(string(s))); }
    void push_back(std::string s) { push_back(mkuniq<string>(string(s))); }
    void push_back(string s) { push_back(mkuniq<string>(s)); }
    void push_back(vector &&v) { push_back(mkuniq<vector>(std::move(v))); }
    void push_back(json::map &&);  // NOLINT(whitespace/operators)
    vector *as_vector() override { return this; }
    const vector *as_vector() const override { return this; }
    std::unique_ptr<obj> copy() && override { return mkuniq<vector>(std::move(*this)); }
    std::unique_ptr<obj> clone() const override {
        vector *v = new vector();
        for (auto &e : *this) v->push_back(clone_ptr(e));
        return std::unique_ptr<vector>(v);
    }
};

typedef ordered_map<obj *, std::unique_ptr<obj>, obj::ptrless> map_base;
class map : public obj, public map_base {
 public:
    map() {}
    map(const map &) = default;
    map(map &&) = default;
    map(const std::initializer_list<std::pair<std::string, obj &&>> &init) {
        for (auto &pair : init) (*this)[pair.first] = std::move(pair.second).copy();
    }
    map &operator=(const map &) & = default;
    map &operator=(map &&) & = default;
    ~map() {
        for (auto &e : *this) delete e.first;
    }
    bool operator<(const obj &a) const override {
        if (const map *b = dynamic_cast<const map *>(&a)) {
            auto p1 = begin(), p2 = b->begin();
            while (p1 != end() && p2 != b->end()) {
                if (*p1->first < *p2->first) return true;
                if (*p1->first != *p2->first) return false;
                if (*p1->second < *p2->second) return true;
                if (*p1->second != *p2->second) return false;
                p1++;
                p2++;
            }
            return p2 != b->end();
        }
        return std::type_index(typeid(*this)) < std::type_index(typeid(a));
    }
    using obj::operator<=;
    using obj::operator>=;
    using obj::operator>;
    bool operator==(const obj &a) const override {
        if (const map *b = dynamic_cast<const map *>(&a)) {
            auto p1 = begin(), p2 = b->begin();
            while (p1 != end() && p2 != b->end()) {
                if (*p1->first != *p2->first) return false;
                if (*p1->second != *p2->second) return false;
                p1++;
                p2++;
            }
            return (p1 == end() && p2 == b->end());
        }
        return false;
    }
    using obj::operator!=;
    std::unique_ptr<obj> remove(const char *key) {
        string tmp(key);
        auto itr = find(&tmp);
        if (itr != end()) {
            std::unique_ptr<obj> val = std::move(itr->second);
            this->erase(itr);
            return val;
        }
        return std::unique_ptr<obj>();
    }
    void print_on(std::ostream &out, int indent = 0, int width = 80,
                  const char *pfx = "") const override;
    bool test_width(int &limit) const override {
        limit -= 2;
        for (auto &e : *this) {
            if (!e.first->test_width(limit)) return false;
            if (e.second ? !e.second->test_width(limit) : (limit -= 4) < 0) return false;
            if ((limit -= 4) < 0) return false;
        }
        return true;
    }
    using map_base::count;
    map_base::size_type count(const char *str) const {
        string tmp(str);
        return count(&tmp);
    }
    map_base::size_type count(std::string &str) const {
        string tmp(str);
        return count(&tmp);
    }
    map_base::size_type count(int64_t n) const {
        number tmp(n);
        return count(&tmp);
    }
    // using map_base::operator[];
    obj *operator[](const std::unique_ptr<obj> &i) const {
        auto rv = find(i.get());
        if (rv != end()) return rv->second.get();
        return 0;
    }
    obj *operator[](const char *str) const {
        string tmp(str);
        auto rv = find(&tmp);
        if (rv != end()) return rv->second.get();
        return 0;
    }
    obj *operator[](const std::string &str) const {
        string tmp(str);
        auto rv = find(&tmp);
        if (rv != end()) return rv->second.get();
        return 0;
    }
    obj *operator[](int64_t n) const {
        number tmp(n);
        auto rv = find(&tmp);
        if (rv != end()) return rv->second.get();
        return 0;
    }

 private:
    class element_ref {
        map &self;
        std::unique_ptr<obj> key;
        map_base::iterator iter;

     public:
        element_ref(map &s, const char *k) : self(s) {
            string tmp(k);
            iter = self.find(&tmp);
            if (iter == self.end()) key.reset(new string(std::move(tmp)));
        }
        element_ref(map &s, int64_t k) : self(s) {
            number tmp(k);
            iter = self.find(&tmp);
            if (iter == self.end()) key.reset(new number(std::move(tmp)));
        }
        element_ref(map &s, std::unique_ptr<obj> &&k) : self(s) {
            iter = self.find(k.get());
            if (iter == self.end()) key = std::move(k);
        }
        void operator=(decltype(nullptr)) {
            if (key) {
                iter = self.emplace(key.release(), std::unique_ptr<obj>()).first;
            } else {
                assert(iter != self.end());
                iter->second.reset();
            }
        }
        bool operator=(bool t) {
            if (key) {
                iter = self.emplace(key.release(),
                                    std::unique_ptr<obj>(t ? static_cast<obj *>(new True())
                                                           : static_cast<obj *>(new False())))
                           .first;
            } else {
                assert(iter != self.end());
                iter->second.reset(t ? static_cast<obj *>(new True())
                                     : static_cast<obj *>(new False()));
            }
            return t;
        }
        bool operator=(void *);  // not defined to avoid converting pointers to bool
        bool operator==(string &str) {
            if (key) return false;
            assert(iter != self.end());
            return *iter->second == str;
        }
        bool operator!=(string &str) { return !(*this == str); }
        bool operator==(const std::string &str) {
            if (key) return false;
            assert(iter != self.end());
            return *iter->second == str;
        }
        bool operator!=(const std::string &str) { return !(*this == str); }
        bool operator==(int64_t v) {
            if (key) return false;
            assert(iter != self.end());
            return *iter->second == v;
        }
        bool operator!=(int64_t v) { return !(*this == v); }
        const char *operator=(const char *v) {
            if (key) {
                iter = self.emplace(key.release(), std::unique_ptr<obj>(new string(v))).first;
            } else {
                assert(iter != self.end());
                iter->second.reset(new string(v));
            }
            return v;
        }
        const std::string &operator=(const std::string &v) {
            if (key) {
                iter = self.emplace(key.release(), std::unique_ptr<obj>(new string(v))).first;
            } else {
                assert(iter != self.end());
                iter->second.reset(new string(v));
            }
            return v;
        }
        int64_t operator=(int64_t v) {
            if (key) {
                iter = self.emplace(key.release(), std::unique_ptr<obj>(new number(v))).first;
            } else {
                assert(iter != self.end());
                iter->second.reset(new number(v));
            }
            return v;
        }
        int operator=(int v) { return static_cast<int>(*this = static_cast<int64_t>(v)); }
        unsigned int operator=(unsigned int v) { return (unsigned int)(*this = (int64_t)v); }
#if defined(__clang__) && defined(__APPLE__)
        // Clang ang gcc on Mac OS can't agree whether size_t overloads uint64_t or unsigned long
        // or the overload is not defined!
        size_t operator=(size_t v) { return (size_t)(*this = (int64_t)v); }
#endif
        uint64_t operator=(uint64_t v) { return (uint64_t)(*this = (int64_t)v); }
        vector &operator=(vector &&v) {
            if (key) {
                iter = self.emplace(key.release(), mkuniq<vector>(std::move(v))).first;
            } else {
                assert(iter != self.end());
                iter->second = mkuniq<vector>(std::move(v));
            }
            return dynamic_cast<vector &>(*iter->second);
        }
        map &operator=(map &&v) {
            if (key) {
                iter = self.emplace(key.release(), mkuniq<map>(std::move(v))).first;
            } else {
                assert(iter != self.end());
                iter->second = mkuniq<map>(std::move(v));
            }
            return dynamic_cast<map &>(*iter->second);
        }
        const std::unique_ptr<obj> &operator=(std::unique_ptr<obj> &&v) {
            if (key) {
                iter = self.emplace(key.release(), std::move(v)).first;
            } else {
                assert(iter != self.end());
                iter->second = std::move(v);
            }
            return iter->second;
        }
        obj &operator*() {
            assert(!key && iter != self.end());
            return *iter->second;
        }
        explicit operator bool() const { return !key; }
        obj *get() const { return key ? 0 : iter->second.get(); }
        obj *operator->() const { return key ? 0 : iter->second.get(); }
        operator vector &() {
            if (key) iter = self.emplace(key.release(), mkuniq<vector>()).first;
            return dynamic_cast<vector &>(*iter->second);
        }
        operator map &() {
            if (key) iter = self.emplace(key.release(), mkuniq<map>()).first;
            return dynamic_cast<map &>(*iter->second);
        }
        element_ref operator[](const char *str) {
            if (key) iter = self.emplace(key.release(), mkuniq<map>()).first;
            map *m = dynamic_cast<map *>(iter->second.get());
            if (!m) throw std::runtime_error("lookup in non-map json object");
            return element_ref(*m, str);
        }
        element_ref operator[](const std::string &str) {
            if (key) iter = self.emplace(key.release(), mkuniq<map>()).first;
            map *m = dynamic_cast<map *>(iter->second.get());
            if (!m) throw std::runtime_error("lookup in non-map json object");
            return element_ref(*m, str.c_str());
        }
        element_ref operator[](int64_t n) {
            if (key) iter = self.emplace(key.release(), mkuniq<map>()).first;
            map *m = dynamic_cast<map *>(iter->second.get());
            if (!m) throw std::runtime_error("lookup in non-map json object");
            return element_ref(*m, n);
        }
        element_ref operator[](std::unique_ptr<obj> &&i) {
            if (key) iter = self.emplace(key.release(), mkuniq<map>()).first;
            map *m = dynamic_cast<map *>(iter->second.get());
            if (!m) throw std::runtime_error("lookup in non-map json object");
            return element_ref(*m, std::move(i));
        }
        template <class T>
        void push_back(T &&v) {
            vector &vec = *this;
            vec.push_back(std::forward<T>(v));
        }
        template <class T>
        bool is() const {
            return !key && dynamic_cast<T *>(iter->second.get()) != nullptr;
        }
        template <class T>
        T &to() {
            if (key) iter = self.emplace(key.release(), mkuniq<T>()).first;
            return dynamic_cast<T &>(*iter->second);
        }
    };
    friend std::ostream &operator<<(std::ostream &out, const element_ref &el);

 public:
    element_ref operator[](const char *str) { return element_ref(*this, str); }
    element_ref operator[](const std::string &str) { return element_ref(*this, str.c_str()); }
    element_ref operator[](int64_t n) { return element_ref(*this, n); }
    element_ref operator[](std::unique_ptr<obj> &&i) { return element_ref(*this, std::move(i)); }
    using map_base::erase;
    map_base::size_type erase(const char *str) {
        string tmp(str);
        return map_base::erase(&tmp);
    }
    map_base::size_type erase(int64_t n) {
        number tmp(n);
        return map_base::erase(&tmp);
    }
    map *as_map() override { return this; }
    const map *as_map() const override { return this; }
    std::unique_ptr<obj> copy() && override { return mkuniq<map>(std::move(*this)); }
    std::unique_ptr<obj> clone() const override {
        map *m = new map();
        for (auto &e : *this)
            m->emplace(e.first ? e.first->clone().release() : nullptr, clone_ptr(e.second));
        return std::unique_ptr<map>(m);
    }

    /// Merges the given map into this one and returns this map. For any key collisions, if both
    /// have a map, then they are merged recursively; if both have a vector, then the one in the
    /// given map is appended to the one in this map; otherwise, the entry in the given map
    /// replaces the entry in this one.
    map &merge(const map &a);
};

inline void vector::push_back(map &&m) { emplace_back(mkuniq<map>(std::move(m))); }

std::istream &operator>>(std::istream &in, std::unique_ptr<obj> &json);
inline std::istream &operator>>(std::istream &in, obj *&json) {
    std::unique_ptr<obj> p;
    in >> p;
    if (in) json = p.release();
    return in;
}

inline std::ostream &operator<<(std::ostream &out, const obj *json) {
    json->print_on(out);
    return out;
}
inline std::ostream &operator<<(std::ostream &out, const std::unique_ptr<obj> &json) {
    return out << json.get();
}
inline std::ostream &operator<<(std::ostream &out, const map::element_ref &el) {
    el->print_on(out);
    return out;
}

}  // end namespace json

extern void dump(const json::obj *);
extern void dump(const json::obj &);

#endif /* BF_ASM_JSON_H_ */
