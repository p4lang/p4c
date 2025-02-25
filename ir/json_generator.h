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
#include "lib/string_map.h"

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

    indent_t indent;
    enum output_state_t {
        TOP,
        VEC_START,
        VEC_MID,
        OBJ_START,
        OBJ_AFTERTAG,
        OBJ_MID,
        OBJ_END
    } output_state = TOP;

 public:
    explicit JSONGenerator(std::ostream &out, bool dumpSourceInfo = false)
        : out(out), dumpSourceInfo(dumpSourceInfo) {}

    output_state_t begin_vector() {
        output_state_t rv = output_state;
        if (rv == OBJ_START) rv = OBJ_END;
        BUG_CHECK(rv != VEC_START, "invalid json output state in begin_vector");
        output_state = VEC_START;
        out << '[';
        ++indent;
        return rv;
    }

    void end_vector(output_state_t prev) {
        --indent;
        if (output_state == VEC_MID)
            out << std::endl << indent;
        else if (output_state != VEC_START)
            BUG("invalid json output state in end_vector");
        out << ']';
        if ((output_state = prev) == OBJ_AFTERTAG) output_state = OBJ_MID;
    }

    output_state_t begin_object() {
        output_state_t rv = output_state;
        output_state = OBJ_START;
        return rv;
    }

    void end_object(output_state_t prev) {
        switch (output_state) {
            case OBJ_START:
                out << "{}";
                break;
            case OBJ_MID:
                out << std::endl << --indent << '}';
                break;
            case OBJ_END:
                break;
            default:
                BUG("invalid json output state in end_object");
                break;
        }
        if ((output_state = prev) == OBJ_AFTERTAG) output_state = OBJ_MID;
    }

    template <typename T>
    void emit(const T &val) {
        switch (output_state) {
            case VEC_MID:
                out << ',';
                /* fall through */
            case VEC_START:
                out << std::endl << indent;
                output_state = VEC_MID;
                break;
            case OBJ_AFTERTAG:
                output_state = OBJ_MID;
                break;
            case TOP:
                break;
            case OBJ_START:
                output_state = OBJ_END;
                break;
            case OBJ_MID:
            case OBJ_END:
                BUG("invalid json output state for emit(obj)");
        }
        generate(val);
        if (output_state == TOP) out << std::endl;
    }

    void emit_tag(const char *tag) {
        switch (output_state) {
            case OBJ_START:
                out << '{' << std::endl << ++indent;
                break;
            case OBJ_MID:
                out << ',' << std::endl << indent;
                break;
            case TOP:
            case VEC_START:
            case VEC_MID:
            case OBJ_AFTERTAG:
            case OBJ_END:
                BUG("invalid json output state for emit_tag");
        }
        out << '\"' << cstring(tag).escapeJson() << "\" : ";
        output_state = OBJ_AFTERTAG;
    }

    template <typename T>
    void emit(const char *tag, const T &val) {
        emit_tag(tag);
        output_state = OBJ_MID;
        generate(val);
    }

    void emit_tag(cstring tag) { emit_tag(tag.c_str()); }
    void emit_tag(std::string tag) { emit_tag(tag.c_str()); }
    template <typename T>
    void emit(cstring tag, const T &val) {
        emit(tag.c_str(), val);
    }
    template <typename T>
    void emit(std::string tag, const T &val) {
        emit(tag.c_str(), val);
    }

 private:
    template <typename T>
    void generate(const safe_vector<T> &v) {
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }

    template <typename T>
    void generate(const std::vector<T> &v) {
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }

    template <typename T, typename U>
    void generate(const std::pair<T, U> &v) {
        auto t = begin_object();
        toJSON(v);
        end_object(t);
    }

 public:
    template <typename T, typename U>
    void toJSON(const std::pair<T, U> &v) {
        emit("first", v.first);
        emit("second", v.second);
    }

 private:
    template <typename T>
    void generate(const std::optional<T> &v) {
        auto t = begin_object();
        emit("valid", !!v);
        if (v) emit("value", *v);
        end_object(t);
    }

    template <typename T>
    void generate(const std::set<T> &v) {
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }

    template <typename T>
    void generate(const ordered_set<T> &v) {
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }

    template <typename K, typename V>
    void generate(const std::map<K, V> &v) {
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }

    template <typename K, typename V>
    void generate(const ordered_map<K, V> &v) {
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }

    template <typename V>
    void generate(const string_map<V> &v) {
        auto t = begin_object();
        for (auto &el : v) emit(el.first, el.second);
        end_object(t);
    }

    template <class... Types>
    void generate(const std::variant<Types...> &v) {
        auto t = begin_object();
        emit("variant_index", v.index());
        std::visit([this](auto &value) { this->emit("value", value); }, v);
        end_object(t);
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
        auto t = begin_object();
        emit("word0", v.word0);
        emit("word1", v.word1);
        end_object(t);
    }

    template <typename T>
    std::enable_if_t<has_toJSON<T>::value && !std::is_base_of_v<IR::Node, T>> generate(const T &v) {
        auto t = begin_object();
        v.toJSON(*this);
        end_object(t);
    }

    void generate(const IR::Node &v) {
        auto t = begin_object();
        if (node_refs.find(v.id) != node_refs.end()) {
            emit("Node_ID", v.id);
        } else {
            node_refs.insert(v.id);
            v.toJSON(*this);
            if (dumpSourceInfo) {
                v.sourceInfoToJSON(*this);
            }
        }
        end_object(t);
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
        auto t = begin_vector();
        for (auto &el : v) emit(el);
        end_vector(t);
    }
};

}  // namespace P4

#endif /* IR_JSON_GENERATOR_H_ */
