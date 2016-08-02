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
#include "lib/match.h"
#include <gmpxx.h>

#include "ir.h"
class JSONGenerator {

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

    template<typename T>
    static constexpr bool is_vector()
    {
        return std::is_same<T, vector<typename T::value_type,
                                      typename T::allocator_type>>::value 
                || std::is_same<T, std::vector<typename T::value_type,
                                               typename T::allocator_type>>::value;
    }

    template<typename T>
    static cstring vectorToJson(const T &v,
                        cstring indent, std::unordered_set<int> &node_refs)
    {
        static_assert(is_vector<T>(), "vectorToJson requires a vector as first argument");
        std::stringstream ss;
        ss << "[";
        if (v.size() > 0) {
            ss << indent << generate(v[0], indent + "    ", node_refs);
            for (size_t i = 1; i < v.size(); i++) {
                ss  << "," << std::endl
                    << indent << generate(v[i], indent + "    ", node_refs);
            }
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }

public:
    template<typename T>
    static cstring generate(const vector<T>
                &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        return vectorToJson(v, indent, node_refs);
    }

    template<typename T>
    static cstring generate(const std::vector<T>
                &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        return vectorToJson(v, indent, node_refs);
    }


    template<typename T, typename U>
    static cstring generate(const std::pair<T, U>
                &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        ss  << "{" << std::endl
            << indent << "\"first\" : "
            << generate(v.first, indent + "    ", node_refs)
            << "," << std::endl << indent << "\"second\" : "
            << generate(v.second, indent + "    ", node_refs)
            << std::endl << indent << "}";
        return ss.str();
    }

    template<typename K, typename V>
    static cstring generate(const ordered_map<K, V>
                &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        ss  << "[" << std::endl;
        if (v.size() > 0) {
            auto it = v.begin();
            ss << indent << generate(*it, indent + "    ", node_refs);
            for (it++; it != v.end(); ++it) {
                ss  << "," << std::endl
                    << indent << generate(*it, indent + "    ", node_refs);
            }
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }

    template<typename T>
    static typename std::enable_if<
        std::is_integral<T>::value, cstring>::type
    generate(T v, cstring, std::unordered_set<int> &) {
        return std::to_string(v);
    }
    static cstring generate(double v, cstring, std::unordered_set<int> &) {
        return std::to_string(v);
    }
    static cstring generate(const mpz_class &v, cstring, std::unordered_set<int> &) {
        std::stringstream ss;
        ss << v;
        return ss.str();
    }

    template<typename T>
    static cstring generate(const IR::Vector<T>&v, cstring indent,
                            std::unordered_set<int> &node_refs)
    {
        return generate(v.get_data(), indent, node_refs);
    }

    static cstring generate(cstring v, cstring, std::unordered_set<int> &) {
        return "\"" + v + "\"";
    }
    template<typename T>
    static typename std::enable_if<
                std::is_same<T, LTBitMatrix>::value ||
                std::is_enum<T>::value, cstring>::type
    generate(T v, cstring, std::unordered_set<int> &) {
        std::stringstream ss;
        ss << "\"";
        ss << v;
        ss << "\"";
        return ss.str();
    }

    static cstring generate(const struct TableResourceAlloc*, 
            cstring, std::unordered_set<int>&)
    {
        return "\"TODO: Implement TableResourceAlloc to JSON \"";
    }

    static cstring generate(const match_t &v, cstring indent, std::unordered_set<int> &)
    {
        std::stringstream ss;
        ss  << "{" << std::endl
            << indent + "    " << "\"word0\" : " << v.word0 << "," << std::endl
            << indent + "    " << "\"word1\" : " << v.word1 << std::endl
            << indent << "}";
        return ss.str();
    }

    template<typename T>
    static typename std::enable_if<has_toJSON<T>::value, cstring>::type
    generate(const T
            & v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        ss << "{" << std::endl
           << v.toJSON(indent + "    ", node_refs) << std::endl
           << indent << "}";
        return ss.str();
    }

    template<typename T>
    static typename std::enable_if<
                        std::is_pointer<T>::value && 
                        has_toJSON<typename std::remove_pointer<T>::type
                        >::value, cstring>::type
    generate(T v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        if (v == nullptr)
            return "null";
        ss << "{" << std::endl
           << v->toJSON(indent + "    ", node_refs) << std::endl
           << indent << "}";
        return ss.str();
    }

    template<typename T, size_t N>
    static cstring generate(const T
            (&v)[N], cstring indent, std::unordered_set<int> &node_refs)
    {
        (void)v; (void) indent; (void)node_refs;
        std::stringstream ss;
        ss << "[";
        if (N > 0) {
            ss << indent << generate(v[0], indent + "    ", node_refs);
            for (size_t i = 1; i < N; i++) {
                ss  << "," << std::endl
                    << indent << generate(v[i], indent + "    ", node_refs);
            }
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    } 
};


#endif /* _IR_JSON_GENERATOR_H_ */
