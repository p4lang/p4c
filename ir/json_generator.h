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

#ifndef IR_JSON_GENERATOR_H_
#define IR_JSON_GENERATOR_H_

#include <optional>
#include <string>
#include <unordered_set>
#include <variant>

#include "ir/node.h"
#include "lib/bitvec.h"
#include "lib/cstring.h"
#include "lib/indent.h"
#include "lib/ltbitmatrix.h"
#include "lib/match.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/safe_vector.h"

namespace P4 {

class JSONGenerator {
    std::unordered_set<int> node_refs;
    std::ostream &out;
    bool dumpSourceInfo;

    template <typename T>
    class has_toJSON {
        typedef char small;
        typedef struct {
            char c[2];
        } big;

        template <typename C>
        static small test(decltype(&C::toJSON));
        template <typename C>
        static big test(...);

     public:
        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    struct variant_generator {
        std::ostream &out;
        JSONGenerator &generator;

        variant_generator(std::ostream &out, JSONGenerator &generator)
            : out(out), generator(generator) {}

        template <class T>
        void operator()(const T &value) const {
            out << R"("value" : )";
            generator.generate(value);
        }
    };

 public:
    indent_t indent;

    explicit JSONGenerator(std::ostream &out, bool dumpSourceInfo = false)
        : out(out), dumpSourceInfo(dumpSourceInfo) {}

    template <typename T>
    void generate(const safe_vector<T> &v) {
        out << "[";
        if (v.size() > 0) {
            out << std::endl << ++indent;
            generate(v[0]);
            for (size_t i = 1; i < v.size(); i++) {
                out << "," << std::endl << indent;
                generate(v[i]);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    template <typename T>
    void generate(const std::vector<T> &v) {
        out << "[";
        if (v.size() > 0) {
            out << std::endl << ++indent;
            generate(v[0]);
            for (size_t i = 1; i < v.size(); i++) {
                out << "," << std::endl << indent;
                generate(v[i]);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    template <typename T, typename U>
    void generate(const std::pair<T, U> &v) {
        ++indent;
        out << "{" << std::endl;
        toJSON(v);
        out << std::endl << --indent << "}";
    }

    template <typename T, typename U>
    void toJSON(const std::pair<T, U> &v) {
        out << indent << "\"first\" : ";
        generate(v.first);
        out << "," << std::endl << indent << "\"second\" : ";
        generate(v.second);
    }

    template <typename T>
    void generate(const std::optional<T> &v) {
        if (!v) {
            out << "{ \"valid\" : false }";
            return;
        }
        out << "{" << std::endl << ++indent;
        out << "\"valid\" : true," << std::endl;
        out << "\"value\" : ";
        generate(*v);
        out << std::endl << --indent << "}";
    }

    template <typename T>
    void generate(const std::set<T> &v) {
        out << "[" << std::endl;
        if (v.size() > 0) {
            auto it = v.begin();
            out << ++indent;
            generate(*it);
            for (it++; it != v.end(); ++it) {
                out << "," << std::endl << indent;
                generate(*it);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    template <typename T>
    void generate(const ordered_set<T> &v) {
        out << "[" << std::endl;
        if (v.size() > 0) {
            auto it = v.begin();
            out << ++indent;
            generate(*it);
            for (it++; it != v.end(); ++it) {
                out << "," << std::endl << indent;
                generate(*it);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    template <typename K, typename V>
    void generate(const std::map<K, V> &v) {
        out << "[" << std::endl;
        if (v.size() > 0) {
            auto it = v.begin();
            out << ++indent;
            generate(*it);
            for (it++; it != v.end(); ++it) {
                out << "," << std::endl << indent;
                generate(*it);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    template <typename K, typename V>
    void generate(const ordered_map<K, V> &v) {
        out << "[" << std::endl;
        if (v.size() > 0) {
            auto it = v.begin();
            out << ++indent;
            generate(*it);
            for (it++; it != v.end(); ++it) {
                out << "," << std::endl << indent;
                generate(*it);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    template <class... Types>
    void generate(const std::variant<Types...> &v) {
        out << "{" << std::endl << ++indent;
        out << R"("variant_index" : )" << v.index() << "," << std::endl << indent;
        variant_generator generator(out, *this);
        std::visit(generator, v);
        out << std::endl << --indent << "}";
    }

    void generate(bool v) { out << (v ? "true" : "false"); }
    template <typename T>
    std::enable_if_t<std::is_integral_v<T>> generate(T v) {
        out << std::to_string(v);
    }
    void generate(double v) { out << std::to_string(v); }
    template <typename T>
    std::enable_if_t<std::is_same_v<T, big_int>> generate(const T &v) {
        out << v;
    }

    void generate(cstring v) {
        if (v) {
            out << "\"" << v.escapeJson() << "\"";
        } else {
            out << "null";
        }
    }
    template <typename T>
    std::enable_if_t<std::is_same_v<T, LTBitMatrix> || std::is_enum_v<T>> generate(T v) {
        out << "\"" << v << "\"";
    }

    void generate(const bitvec &v) { out << "\"" << v << "\""; }

    void generate(const match_t &v) {
        out << "{" << std::endl
            << (indent + 1) << "\"word0\" : " << v.word0 << "," << std::endl
            << (indent + 1) << "\"word1\" : " << v.word1 << std::endl
            << indent << "}";
    }

    template <typename T>
    std::enable_if_t<has_toJSON<T>::value && !std::is_base_of_v<IR::Node, T>> generate(const T &v) {
        ++indent;
        out << "{" << std::endl;
        v.toJSON(*this);
        out << std::endl << --indent << "}";
    }

    void generate(const IR::Node &v) {
        out << "{" << std::endl;
        ++indent;
        if (node_refs.find(v.id) != node_refs.end()) {
            out << indent << "\"Node_ID\" : " << v.id;
        } else {
            node_refs.insert(v.id);
            v.toJSON(*this);
            if (dumpSourceInfo) {
                v.sourceInfoToJSON(*this);
            }
        }
        out << std::endl << --indent << "}";
    }

    template <typename T>
    std::enable_if_t<std::is_pointer_v<T> && has_toJSON<std::remove_pointer_t<T>>::value> generate(
        T v) {
        if (v)
            generate(*v);
        else
            out << "null";
    }

    template <typename T, size_t N>
    void generate(const T (&v)[N]) {
        out << "[";
        if (N > 0) {
            out << std::endl << ++indent;
            generate(v[0]);
            for (size_t i = 1; i < N; i++) {
                out << "," << std::endl << indent;
                generate(v[i]);
            }
            out << std::endl << --indent;
        }
        out << "]";
    }

    JSONGenerator &operator<<(char ch) {
        out << ch;
        return *this;
    }
    JSONGenerator &operator<<(const char *s) {
        out << s;
        return *this;
    }
    JSONGenerator &operator<<(indent_t i) {
        out << i;
        return *this;
    }
    JSONGenerator &operator<<(std::ostream &(*fn)(std::ostream &)) {
        out << fn;
        return *this;
    }
    template <typename T>
    JSONGenerator &operator<<(const T &v) {
        generate(v);
        return *this;
    }
};

}  // namespace P4

#endif /* IR_JSON_GENERATOR_H_ */
