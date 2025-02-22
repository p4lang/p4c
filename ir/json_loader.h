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

#ifndef IR_JSON_LOADER_H_
#define IR_JSON_LOADER_H_

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "ir.h"
#include "json_parser.h"
#include "lib/bitvec.h"
#include "lib/cstring.h"
#include "lib/ltbitmatrix.h"
#include "lib/match.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/safe_vector.h"

namespace P4 {

class JSONLoader {
    template <typename T>
    class has_fromJSON {
        typedef char small;
        typedef struct {
            char c[2];
        } big;

        template <typename C>
        static small test(decltype(&C::fromJSON));
        template <typename C>
        static big test(...);

     public:
        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    std::unordered_map<int, IR::Node *> &node_refs;
    std::unique_ptr<JsonData> json_root;
    const JsonData *json = nullptr;

 public:
    explicit JSONLoader(std::istream &in)
        : node_refs(*(new std::unordered_map<int, IR::Node *>())) {
        in >> json_root;
        json = json_root.get();
    }

 private:
    JSONLoader(JsonData *json, std::unordered_map<int, IR::Node *> &refs)
        : node_refs(refs), json(json) {}

 public:
    JSONLoader(const JSONLoader &unpacker, const std::string &field)
        : node_refs(unpacker.node_refs), json(nullptr) {
        if (!unpacker) return;
        if (auto *obj = unpacker.json->to<JsonObject>()) {
            if (auto it = obj->find(field); it != obj->end()) {
                json = it->second.get();
            }
        }
    }

    explicit operator bool() const { return json != nullptr; }
    template <typename T>
    [[nodiscard]] bool is() const {
        return json && json->is<T>();
    }
    template <typename T>
    [[nodiscard]] const T &as() const {
        return json->as<T>();
    }

 private:
    const IR::Node *get_node() {
        if (!json || !json->is<JsonObject>()) return nullptr;  // invalid json exception?
        int id;
        load("Node_ID", id);
        if (id >= 0) {
            if (node_refs.find(id) == node_refs.end()) {
                cstring type;
                load("Node_Type", type);
                if (auto fn = get(IR::unpacker_table, type)) {
                    node_refs[id] = fn(*this);
                    // Creating JsonObject from source_info read from jsonFile
                    // and setting SourceInfo for each node
                    // when "--fromJSON" flag is used
                    node_refs[id]->sourceInfoFromJSON(*this);
                } else {
                    return nullptr;
                }  // invalid json exception?
            }
            return node_refs[id];
        }
        return nullptr;  // invalid json exception?
    }

    template <typename T>
    void unpack_json(safe_vector<T> &v) {
        T temp;
        v.clear();
        for (auto &e : as<JsonVector>()) {
            load(e.get(), temp);
            v.push_back(temp);
        }
    }

    template <typename T>
    void unpack_json(std::set<T> &v) {
        T temp;
        v.clear();
        for (auto &e : as<JsonVector>()) {
            load(e.get(), temp);
            v.insert(temp);
        }
    }

    template <typename T>
    void unpack_json(ordered_set<T> &v) {
        T temp;
        v.clear();
        for (auto &e : as<JsonVector>()) {
            load(e.get(), temp);
            v.insert(temp);
        }
    }

    template <typename T>
    void unpack_json(IR::Vector<T> &v) {
        v = *IR::Vector<T>::fromJSON(*this);
    }
    template <typename T>
    void unpack_json(const IR::Vector<T> *&v) {
        v = IR::Vector<T>::fromJSON(*this);
    }
    template <typename T>
    void unpack_json(IR::IndexedVector<T> &v) {
        v = *IR::IndexedVector<T>::fromJSON(*this);
    }
    template <typename T>
    void unpack_json(const IR::IndexedVector<T> *&v) {
        v = IR::IndexedVector<T>::fromJSON(*this);
    }
    template <class T, template <class K, class V, class COMP, class ALLOC> class MAP, class COMP,
              class ALLOC>
    void unpack_json(IR::NameMap<T, MAP, COMP, ALLOC> &m) {
        m = *IR::NameMap<T, MAP, COMP, ALLOC>::fromJSON(*this);
    }
    template <class T, template <class K, class V, class COMP, class ALLOC> class MAP, class COMP,
              class ALLOC>
    void unpack_json(const IR::NameMap<T, MAP, COMP, ALLOC> *&m) {
        m = IR::NameMap<T, MAP, COMP, ALLOC>::fromJSON(*this);
    }

    template <typename K, typename V>
    void unpack_json(std::map<K, V> &v) {
        std::pair<K, V> temp;
        v.clear();
        if (is<JsonVector>()) {
            for (auto &e : as<JsonVector>()) {
                load(e.get(), temp);
                v.insert(temp);
            }
        } else {
            for (auto &e : as<JsonObject>()) {
                JsonString *k = new JsonString(e.first);
                load(k, temp.first);
                load(e.second.get(), temp.second);
                v.insert(temp);
            }
        }
    }
    template <typename K, typename V>
    void unpack_json(ordered_map<K, V> &v) {
        std::pair<K, V> temp;
        v.clear();
        if (is<JsonVector>()) {
            for (auto &e : as<JsonVector>()) {
                load(e.get(), temp);
                v.insert(temp);
            }
        } else {
            for (auto &e : as<JsonObject>()) {
                JsonString *k = new JsonString(e.first);
                load(k, temp.first);
                load(e.second.get(), temp.second);
                v.insert(temp);
            }
        }
    }
    template <typename V>
    void unpack_json(string_map<V> &v) {
        std::pair<cstring, V> temp;
        v.clear();
        for (auto &e : as<JsonObject>()) {
            temp.first = e.first;
            load(e.second.get(), temp.second);
            v.insert(temp);
        }
    }

    template <typename K, typename V>
    void unpack_json(std::multimap<K, V> &v) {
        std::pair<K, V> temp;
        v.clear();
        if (is<JsonVector>()) {
            for (auto &e : as<JsonVector>()) {
                load(e.get(), temp);
                v.insert(temp);
            }
        } else {
            for (auto &e : as<JsonObject>()) {
                JsonString *k = new JsonString(e.first);
                load(k, temp.first);
                load(e.second.get(), temp.second);
                v.insert(temp);
            }
        }
    }

    template <typename T>
    void unpack_json(std::vector<T> &v) {
        T temp;
        v.clear();
        for (auto &e : as<JsonVector>()) {
            load(e.get(), temp);
            v.push_back(temp);
        }
    }

    template <typename T, typename U>
    void unpack_json(std::pair<T, U> &v) {
        load("first", v.first);
        load("second", v.second);
    }

    template <typename T>
    void unpack_json(std::optional<T> &v) {
        bool isValid = false;
        load("valid", isValid);
        if (!isValid) {
            v = std::nullopt;
            return;
        }
        T value;
        load("value", value);
        v = std::move(value);
    }

    template <int N, class Variant>
    std::enable_if_t<N == std::variant_size_v<Variant>> unpack_variant(int /*target*/,
                                                                       Variant & /*variant*/) {
        BUG("Error traversing variant during load");
    }

    template <int N, class Variant>
    std::enable_if_t<(N < std::variant_size_v<Variant>)> unpack_variant(int target,
                                                                        Variant &variant) {
        if (N == target) {
            variant.template emplace<N>();
            load("value", std::get<N>(variant));
        } else
            unpack_variant<N + 1>(target, variant);
    }

    template <class... Types>
    void unpack_json(std::variant<Types...> &v) {
        int index = -1;
        load("variant_index", index);
        unpack_variant<0>(index, v);
    }

    void unpack_json(bool &v) { v = as<JsonBoolean>(); }

    template <typename T>
    std::enable_if_t<std::is_integral_v<T>> unpack_json(T &v) {
        v = as<JsonNumber>();
    }
    void unpack_json(big_int &v) { v = as<JsonNumber>().val; }
    void unpack_json(std::string &v) {
        if (is<JsonString>()) v = as<JsonString>();
    }
    void unpack_json(cstring &v) {
        std::string tmp = as<JsonString>();
        std::string::size_type p = 0;
        while ((p = tmp.find('\\', p)) != std::string::npos) {
            tmp.erase(p, 1);
            switch (tmp[p]) {
                case 'n':
                    tmp[p] = '\n';
                    break;
                case 'r':
                    tmp[p] = '\r';
                    break;
                case 't':
                    tmp[p] = '\t';
                    break;
            }
            p++;
        }
        if (!json->is<JsonNull>()) v = tmp;
    }
    void unpack_json(IR::ID &v) {
        if (!json->is<JsonNull>()) v.name = as<JsonString>();
    }

    void unpack_json(LTBitMatrix &m) {
        if (auto *s = json->to<JsonString>()) s->c_str() >> m;
    }

    void unpack_json(bitvec &v) {
        if (auto *s = json->to<JsonString>()) s->c_str() >> v;
    }

    template <typename T>
    std::enable_if_t<std::is_enum_v<T>> unpack_json(T &v) {
        if (auto *s = json->to<JsonString>()) *s >> v;
    }

    void unpack_json(match_t &v) {
        if (auto *s = json->to<JsonString>()) s->c_str() >> v;
    }

    void unpack_json(UnparsedConstant *&v) {
        cstring text("");
        unsigned skip = 0;
        unsigned base = 0;
        bool hasWidth = false;

        load("text", text);
        load("skip", skip);
        load("base", base);
        load("hasWidth", hasWidth);

        v = new UnparsedConstant({text, skip, base, hasWidth});
    }

    template <typename T>
    std::enable_if_t<has_fromJSON<T>::value && !std::is_base_of_v<IR::Node, T> &&
                     std::is_pointer_v<decltype(T::fromJSON(std::declval<JSONLoader &>()))>>
    unpack_json(T *&v) {
        v = T::fromJSON(*this);
    }

    template <typename T>
    std::enable_if_t<has_fromJSON<T>::value && !std::is_base_of_v<IR::Node, T> &&
                     std::is_pointer_v<decltype(T::fromJSON(std::declval<JSONLoader &>()))>>
    unpack_json(T &v) {
        v = *(T::fromJSON(*this));
    }

    template <typename T>
    std::enable_if_t<has_fromJSON<T>::value && !std::is_base_of<IR::Node, T>::value &&
                     !std::is_pointer_v<decltype(T::fromJSON(std::declval<JSONLoader &>()))>>
    unpack_json(T &v) {
        v = T::fromJSON(*this);
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<IR::INode, T>> unpack_json(T &v) {
        v = get_node()->as<T>();
    }
    template <typename T>
    std::enable_if_t<std::is_base_of_v<IR::INode, T>> unpack_json(const T *&v) {
        v = get_node()->checkedTo<T>();
    }

    template <typename T, size_t N>
    void unpack_json(T (&v)[N]) {
        if (auto *j = json->to<JsonVector>()) {
            for (size_t i = 0; i < N && i < j->size(); ++i) {
                load((*j)[i].get(), v[i]);
            }
        }
    }

    template <typename T>
    void load(JsonData *json, T &v) {
        JSONLoader(json, node_refs).unpack_json(v);
    }

 public:
    template <typename T>
    bool load(const std::string field, T *&v) {
        if (auto loader = JSONLoader(*this, field)) {
            loader.unpack_json(v);
            return true;
        } else {
            v = nullptr;
            return false;
        }
    }

    template <typename T>
    bool load(const std::string field, T &v) {
        if (auto loader = JSONLoader(*this, field)) {
            loader.unpack_json(v);
            return true;
        } else {
            return false;
        }
    }

    template <typename T>
    JSONLoader &operator>>(T &v) {
        unpack_json(v);
        return *this;
    }
};

template <class T>
IR::Vector<T>::Vector(JSONLoader &json) : VectorBase(json) {
    json.load("vec", vec);
}
template <class T>
IR::Vector<T> *IR::Vector<T>::fromJSON(JSONLoader &json) {
    return new Vector<T>(json);
}
template <class T>
IR::IndexedVector<T>::IndexedVector(JSONLoader &json) : Vector<T>(json) {
    json.load("declarations", declarations);
}
template <class T>
IR::IndexedVector<T> *IR::IndexedVector<T>::fromJSON(JSONLoader &json) {
    return new IndexedVector<T>(json);
}
template <class T, template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
IR::NameMap<T, MAP, COMP, ALLOC>::NameMap(JSONLoader &json) : Node(json) {
    json.load("symbols", symbols);
}
template <class T, template <class K, class V, class COMP, class ALLOC> class MAP /*= std::map */,
          class COMP /*= std::less<cstring>*/,
          class ALLOC /*= std::allocator<std::pair<cstring, const T*>>*/>
IR::NameMap<T, MAP, COMP, ALLOC> *IR::NameMap<T, MAP, COMP, ALLOC>::fromJSON(JSONLoader &json) {
    return new IR::NameMap<T, MAP, COMP, ALLOC>(json);
}

}  // namespace P4

#endif /* IR_JSON_LOADER_H_ */
