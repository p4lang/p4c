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

#ifndef _IR_JSON_LOADER_H_
#define _IR_JSON_LOADER_H_

#include <assert.h>
#include <boost/optional.hpp>
#include <gmpxx.h>
#include <string>
#include <map>
#include <unordered_map>
#include <utility>
#include "lib/cstring.h"
#include "lib/indent.h"
#include "lib/match.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/safe_vector.h"
#include "ir.h"
#include "json_parser.h"

class JSONLoader {
    template<typename T> class has_fromJSON {
        typedef char small;
        typedef struct { char c[2]; } big;

        template<typename C> static small test(decltype(&C::fromJSON));
        template<typename C> static big test(...);
     public:
        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

 public:
    std::unordered_map<int, IR::Node*> &node_refs;
    JsonData *json;

    explicit JSONLoader(std::istream &in) : node_refs(*(new std::unordered_map<int, IR::Node*>()))
    { in >> json; }

    explicit JSONLoader(JsonData *json)
    : node_refs(*(new std::unordered_map<int, IR::Node*>())), json(json) {}

    JSONLoader(JsonData *json, std::unordered_map<int, IR::Node*> &refs)
    : node_refs(refs), json(json) {}

    JSONLoader(const JSONLoader &unpacker, const std::string &field)
    : node_refs(unpacker.node_refs), json(nullptr) {
        if (auto obj = dynamic_cast<JsonObject *>(unpacker.json))
            json = get(obj, field); }

 private:
    const IR::Node* get_node() {
        if (!json || !json->is<JsonObject>()) return nullptr;  // invalid json exception?
        int id = json->to<JsonObject>()->get_id();
        if (id >= 0) {
            if (node_refs.find(id) == node_refs.end()) {
                if (auto fn = get(IR::unpacker_table, json->to<JsonObject>()->get_type()))
                    node_refs[id] = fn(*this);
                else
                    return nullptr; }  // invalid json exception?
            return node_refs[id]; }
        return nullptr;  // invalid json exception?
    }

    template<typename T>
    void unpack_json(safe_vector<T> &v) {
        T temp;
        for (auto e : *json->to<JsonVector>()) {
            load(e, temp);
            v.push_back(temp);
        }
    }

    template<typename T>
    void unpack_json(std::set<T> &v) {
        T temp;
        for (auto e : *json->to<JsonVector>()) {
            load(e, temp);
            v.insert(temp);
        }
    }

    template<typename T>
    void unpack_json(ordered_set<T> &v) {
        T temp;
        for (auto e : *json->to<JsonVector>()) {
            load(e, temp);
            v.insert(temp);
        }
    }

    template<typename T> void unpack_json(IR::Vector<T> &v) {
        v = *IR::Vector<T>::fromJSON(*this); }
    template<typename T> void unpack_json(const IR::Vector<T> *&v) {
        v = IR::Vector<T>::fromJSON(*this); }
    template<typename T> void unpack_json(IR::IndexedVector<T> &v) {
        v = *IR::IndexedVector<T>::fromJSON(*this); }
    template<typename T> void unpack_json(const IR::IndexedVector<T> *&v) {
        v = IR::IndexedVector<T>::fromJSON(*this); }
    template<class T, template<class K, class V, class COMP, class ALLOC> class MAP,
             class COMP, class ALLOC>
    void unpack_json(IR::NameMap<T, MAP, COMP, ALLOC> &m) {
        m = *IR::NameMap<T, MAP, COMP, ALLOC>::fromJSON(*this); }
    template<class T, template<class K, class V, class COMP, class ALLOC> class MAP,
             class COMP, class ALLOC>
    void unpack_json(const IR::NameMap<T, MAP, COMP, ALLOC> *&m) {
        m = IR::NameMap<T, MAP, COMP, ALLOC>::fromJSON(*this); }

    template<typename K, typename V>
    void unpack_json(std::map<K, V> &v) {
        std::pair<K, V> temp;
        for (auto e : *json->to<JsonObject>()) {
            JsonString* k = new JsonString(e.first);
            load(k, temp.first);
            load(e.second, temp.second);
            v.insert(temp);
        }
    }
    template<typename K, typename V>
    void unpack_json(ordered_map<K, V> &v) {
        std::pair<K, V> temp;
        for (auto e : *json->to<JsonObject>()) {
            JsonString* k = new JsonString(e.first);
            load(k, temp.first);
            load(e.second, temp.second);
            v.insert(temp);
        }
    }
    template<typename K, typename V>
    void unpack_json(std::multimap<K, V> &v) {
        std::pair<K, V> temp;
        for (auto e : *json->to<JsonObject>()) {
            JsonString* k = new JsonString(e.first);
            load(k, temp.first);
            load(e.second, temp.second);
            v.insert(temp);
        }
    }

    template<typename T>
    void unpack_json(std::vector<T> &v) {
        T temp;
        for (auto e : *json->to<JsonVector>()) {
            load(e, temp);
            v.push_back(temp);
        }
    }

    template<typename T, typename U>
    void unpack_json(std::pair<T, U> &v) {
        const JsonObject* obj = json->to<JsonObject>();
        load(::get(obj, "first"), v.first);
        load(::get(obj, "second"), v.second);
    }

    template<typename T>
    void unpack_json(boost::optional<T> &v) {
        const JsonObject* obj = json->to<JsonObject>();
        bool isValid = false;
        load(::get(obj, "valid"), isValid);
        if (!isValid) {
            v = boost::none;
            return;
        }
        T value;
        load(::get(obj, "value"), value),
        v = std::move(value);
    }

    void unpack_json(bool &v) { v = *json->to<JsonBoolean>(); }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value>::type
    unpack_json(T &v) { v = *json->to<JsonNumber>(); }
    void unpack_json(mpz_class &v) { v = json->to<JsonNumber>()->val; }
    void unpack_json(cstring &v) { if (!json->is<JsonNull>()) v = *json->to<std::string>(); }
    void unpack_json(IR::ID &v) { if (!json->is<JsonNull>()) v.name = *json->to<std::string>(); }

    void unpack_json(LTBitMatrix &m) {
        if (auto *s = json->to<std::string>())
            s->c_str() >> m; }

    template<typename T> typename std::enable_if<std::is_enum<T>::value>::type
    unpack_json(T &v) {
        if (auto *s = json->to<std::string>())
            *s >> v; }

    void unpack_json(match_t &v) {
        if (auto *s = json->to<std::string>())
            s->c_str() >> v; }

    void unpack_json(UnparsedConstant*& v) {
        cstring text("");
        unsigned skip = 0;
        unsigned base = 0;
        bool hasWidth = false;

        load("text", text);
        load("skip", skip);
        load("base", base);
        load("hasWidth", hasWidth);

        UnparsedConstant result {text, skip, base, hasWidth};
        v = &result;
    }

    template<typename T>
    typename std::enable_if<
        has_fromJSON<T>::value &&
        !std::is_base_of<IR::Node, T>::value &&
        std::is_pointer<decltype(T::fromJSON(std::declval<JSONLoader&>()))>::value
    >::type
    unpack_json(T *&v) { v = T::fromJSON(*this); }

    template<typename T>
    typename std::enable_if<
        has_fromJSON<T>::value &&
        !std::is_base_of<IR::Node, T>::value &&
        std::is_pointer<decltype(T::fromJSON(std::declval<JSONLoader&>()))>::value
    >::type
    unpack_json(T &v) { v = *(T::fromJSON(*this)); }

    template<typename T>
    typename std::enable_if<
        has_fromJSON<T>::value &&
        !std::is_base_of<IR::Node, T>::value &&
        !std::is_pointer<decltype(T::fromJSON(std::declval<JSONLoader&>()))>::value
    >::type
    unpack_json(T &v) { v = T::fromJSON(*this); }

    template<typename T> typename std::enable_if<std::is_base_of<IR::INode, T>::value>::type
    unpack_json(T &v) { v = *(get_node()->to<T>()); }
    template<typename T> typename std::enable_if<std::is_base_of<IR::INode, T>::value>::type
    unpack_json(const T *&v) { v = get_node()->to<T>(); }

    template<typename T, size_t N>
    void unpack_json(T (&v)[N]) {
        if (auto *j = json->to<JsonVector>()) {
            for (size_t i = 0; i < N && i < j->size(); ++i) {
                json = (*j)[i];
                unpack_json(v[i]); } } }

 public:
    template<typename T>
    void load(JsonData* json, T &v) {
        JSONLoader(json, node_refs).unpack_json(v); }

    template<typename T>
    void load(const std::string field, T &v) {
        JSONLoader loader(*this, field);
        if (loader.json == nullptr) return;
        loader.unpack_json(v); }

    template<typename T> JSONLoader& operator>>(T &v) {
        unpack_json(v);
        return *this; }
};

template<class T>
IR::Vector<T>::Vector(JSONLoader &json) : VectorBase(json) {
    json.load("vec", vec);
}
template<class T>
IR::Vector<T>* IR::Vector<T>::fromJSON(JSONLoader &json) {
    return new Vector<T>(json);
}
template<class T>
IR::IndexedVector<T>::IndexedVector(JSONLoader &json) : Vector<T>(json) {
    json.load("declarations", declarations);
}
template<class T>
IR::IndexedVector<T>* IR::IndexedVector<T>::fromJSON(JSONLoader &json) {
    return new IndexedVector<T>(json);
}
template<class T, template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
IR::NameMap<T, MAP, COMP, ALLOC>::NameMap(JSONLoader &json) : Node(json) {
    json.load("symbols", symbols);
}
template<class T, template<class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
         class COMP /*= std::less<cstring>*/,
         class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
IR::NameMap<T, MAP, COMP, ALLOC> *IR::NameMap<T, MAP, COMP, ALLOC>::fromJSON(JSONLoader &json) {
    return new IR::NameMap<T, MAP, COMP, ALLOC>(json);
}

#endif /* _IR_JSON_LOADER_H_ */
