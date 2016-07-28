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

#ifndef _IR_JSON_GENERATOR_H_
#define _IR_JSON_GENERATOR_H_

#include <assert.h>
#include <string>
#include "lib/cstring.h"
#include "lib/indent.h"
#include "lib/match.h"
#include <gmpxx.h>

#include "ir.h"
class JSONGenerator {
    std::unordered_set<int> node_refs;

    template<typename T>
    class has_toJSON
    {
        typedef char small;
        typedef struct { char c[2]; } big;

        template<typename C> static small test(decltype(&C::toJSON));
        template<typename C> static big test(...);
    public:
        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

 public:
    indent_t indent;

    template<typename T>
    cstring generate(const vector<T> &v) {
        std::stringstream ss;
        ss << "[";
        if (v.size() > 0) {
            ss << indent++;
            ss << generate(v[0]);
            for (size_t i = 1; i < v.size(); i++) {
                ss  << "," << std::endl
                    << (indent-1) << generate(v[i]); }
            ss << std::endl << --indent; }
        ss << "]";
        return ss.str();
    }

    template<typename T, typename U>
    cstring generate(const std::pair<T, U> &v)
    {
        std::stringstream ss;
        ss << "{" << std::endl << indent++ << "\"first\" : ";
        ss << generate(v.first) << "," << std::endl << (indent-1)
           << "\"second\" : " << generate(v.second) << std::endl;
        ss << --indent << "}";
        return ss.str();
    }

    template<typename K, typename V>
    cstring generate(const ordered_map<K, V> &v)
    {
        std::stringstream ss;
        ss  << "[" << std::endl;
        if (v.size() > 0) {
            ++indent;
            auto it = v.begin();
            ss << (indent-1) << generate(*it);
            for (it++; it != v.end(); ++it) {
                ss << "," << std::endl
                   << (indent-1) << generate(*it); }
            --indent;
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, cstring>::type
    generate(T v) {
        return std::to_string(v);
    }
    cstring generate(double v) {
        return std::to_string(v);
    }
    cstring generate(const mpz_class &v) {
        std::stringstream ss;
        ss << v;
        return ss.str();
    }

    template<typename T>
    cstring generate(const IR::Vector<T>&v)
    {
        return generate(v.get_data());
    }

    cstring generate(cstring v) {
        return "\"" + v + "\"";
    }
    template<typename T>
    typename std::enable_if<
                std::is_same<T, LTBitMatrix>::value ||
                std::is_enum<T>::value, cstring>::type
    generate(T v) {
        std::stringstream ss;
        ss << "\"";
        ss << v;
        ss << "\"";
        return ss.str();
    }

    cstring generate(const match_t &v) {
        std::stringstream ss;
        ss << "{" << std::endl
           << (indent + 1) << "\"word0\" : " << v.word0 << "," << std::endl
           << (indent + 1) << "\"word1\" : " << v.word1 << std::endl
           << indent << "}";
        return ss.str();
    }

    template<typename T>
    typename std::enable_if<
                    has_toJSON<T>::value &&
                    !std::is_base_of<IR::Node, T>::value, cstring>::type
    generate(const T &v) {
        std::stringstream ss;
        ++indent;
        ss << "{" << std::endl
           << v.toJSON(this) << std::endl
           << --indent << "}";
        return ss.str();
    }

    cstring generate(const IR::Node &v) {
        std::stringstream ss;
        ss << "{" << std::endl;
        ++indent;
        if (node_refs.find(v.id) != node_refs.end()) {
            ss << indent << "\"Node_ID\" : " << v.id << std::endl;
        } else {
            node_refs.insert(v.id);
            ss << v.toJSON(this) << std::endl; }
        ss << --indent << "}";
        return ss.str();
    }

    template<typename T>
    typename std::enable_if<
                    std::is_pointer<T>::value &&
                    has_toJSON<typename std::remove_pointer<T>::type>::value, cstring>::type
    generate(T v) {
        if (v == nullptr)
            return "null";
        return generate(*v);
    }

    template<typename T, size_t N>
    cstring generate(const T (&v)[N]) {
        std::stringstream ss;
        ss << "[";
        if (N > 0) {
            ++indent;
            ss << (indent-1) << generate(v[0]);
            for (size_t i = 1; i < N; i++) {
                ss  << "," << std::endl
                    << (indent-1) << generate(v[i]);
            }
            --indent;
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }
};

#endif /* _IR_JSON_GENERATOR_H_ */
