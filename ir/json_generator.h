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
public:
    template<typename T, typename A = void>
    struct is_numeric {
        static const bool value = false;
    };

    template<typename T>
    struct is_numeric<T, typename std::enable_if<
                                        std::is_integral<T>::value ||
                                        std::is_same<mpz_class, T>::value ||
                                        std::is_floating_point<T>::value
                                  >::type>
    {
        static const bool value = true;
    };

    template<typename T, typename A = void>
    struct is_vector {
        static const bool value = false;
    };

    template<typename T>
    struct is_vector<T, typename std::enable_if<
                                    std::is_same<T, 
                                                 vector<typename T::value_type,
                                                        typename T::allocator_type>
                                    >::value ||
                                    std::is_same<T,
                                                 std::vector<typename T::value_type,
                                                             typename T::allocator_type>
                                    >::value 
                                >::type>
    {
        static const bool value = true;
    };

    template<typename T, typename A = void>
    struct is_std_vector {
        static const bool value = false;
    };

    template<typename T>
    struct is_std_vector<T, typename std::enable_if<
                                        std::is_same<T,
                                                     std::vector<typename T::value_type,
                                                                 typename T::allocator_type>
                                        >::value
                                    >::type>
    {
        static const bool value = true;
    };

    template<typename T, typename COMP = void, typename ALLOC = void>
    struct is_ordered_map {
        static const bool value = false;
    };

    template<typename T>
    struct is_ordered_map<T, typename std::enable_if<
                                            std::is_same<T,
                                                         ordered_map<typename T::key_type,
                                                                     typename T::mapped_type,
                                                                     typename T::key_compare,
                                                                     typename T::allocator_type>
                                            >::value
                                        >::type>
    {
        static const bool value = true;
    };

    template<typename T, typename A = void>
    struct is_std_pair {
        static const bool value = false;
    };

    template<typename T>
    struct is_std_pair<T, typename std::enable_if<
                                        std::is_same<T,
                                                     std::pair<typename T::first_type,
                                                               typename T::second_type>
                                        >::value
                                    >::type>
    {
        static const bool value = true;
    };
    
    template<typename T, typename A = void>
    struct is_ir_vector {
        static const bool value = false;
    };

    template<typename T>
    struct is_ir_vector<T, typename std::enable_if<
                                        std::is_same<T,
                                                     IR::Vector<typename T::value_type>
                                        >::value
                                    >::type>
    {
        static const bool value = true;
    };

     
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
    static cstring generate(const typename 
            std::enable_if<is_vector<T>::value, T>::type 
                &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        (void)v; (void)indent; (void)node_refs;
        std::stringstream ss;
        ss << "[";
        if (v.size() > 0) {
            ss << indent << generate<typename T::value_type>(v[0], indent + "    ", node_refs);
            for (size_t i = 1; i < v.size(); i++) {
                ss  << "," << std::endl 
                    << indent << generate<typename T::value_type>(v[i], indent + "    ", node_refs);
            }
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }

    template<typename T>
    static cstring generate(const typename
            std::enable_if<is_std_pair<T>::value, T>::type
                &v, cstring indent, std::unordered_set<int> &node_refs)
    { 
        std::stringstream ss;
        ss  << "{" << std::endl
            << indent << "\"first\" : " 
            << generate<typename T::first_type>(v.first, indent + "    ", node_refs) 
            << "," << std::endl << indent << "\"second\" : "
            << generate<typename T::second_type>(v.second, indent + "    ", node_refs) 
            << std::endl << indent << "}";
        return ss.str();
    }

    template<typename T>
    static cstring generate(const typename 
            std::enable_if<is_ordered_map<T>::value, T>::type 
                &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        ss  << "[" << std::endl;
        if (v.size() > 0) {
            auto it = v.begin();
            ss << indent << generate<typename T::value_type>(*it, indent + "    ", node_refs);
            for (it++; it != v.end(); ++it) {
                ss  << "," << std::endl
                    << indent << generate<typename T::value_type>(*it, indent + "    ", node_refs);
            }
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }

    template<typename T> 
    static cstring generate(const typename 
            std::enable_if<
                !std::is_base_of<IR::Node, T>::value && 
                is_numeric<T>::value
                , T>::type
                            &v, cstring indent, std::unordered_set<int> &node_refs)
    {   
        (void)indent; (void)node_refs;
        std::stringstream ss;
        ss << v;
        return ss.str();
    }

    template<typename T> 
    static cstring generate(const typename 
            std::enable_if<
                    is_ir_vector<T>::value, T>::type
             &v, cstring indent, std::unordered_set<int> &node_refs)
    {   
        return generate<vector<typename T::value_type>>(v.get_data(), indent, node_refs);
    }

    template<typename T>
    static cstring generate(const typename
            std::enable_if<
                std::is_same<T, cstring>::value ||
                std::is_same<T, IR::ID>::value ||
                std::is_same<T, LTBitMatrix>::value ||
                std::is_enum<T>::value
                , T>::type
            & v, cstring indent, std::unordered_set<int> &node_refs)
    {
        (void)indent; (void)node_refs;
        std::stringstream ss;
        ss << "\"";
        ss << v; 
        ss << "\"";
        return ss.str();
    }

    template<typename T>
    static cstring generate(const typename
                                std::enable_if<
                                    std::is_same<T, match_t>::value
                                , T>::type
                            &v, cstring indent, std::unordered_set<int> &node_refs)
    {
        (void)node_refs;
        std::stringstream ss;
        ss  << "{" << std::endl
            << indent + "    " << "\"word0\" : " << v.word0 << "," << std::endl
            << indent + "    " << "\"word1\" : " << v.word1 << std::endl
            << indent << "}";
        return ss.str();
    }

    template<typename T>
    static cstring generate(const typename
                                std::enable_if<
                                    std::is_same<typename std::remove_cv<
                                                    typename std::remove_pointer<T>::type
                                                 >::type, struct TableResourceAlloc
                                    >::value
                                , T>::type
                            v, cstring indent, std::unordered_set<int> &node_refs)
    {
        (void)v; (void)indent; (void)node_refs;
        return "\"Handle TableResourceAlloc here.\"";
    }

    template<typename T>
    static cstring generate(const typename
                                std::enable_if<
                                    std::is_same<typename std::remove_cv<
                                                    typename std::remove_pointer<T>::type
                                                 >::type,
                                                 IR::CompileTimeValue>::value
                                , T>::type
                            v, cstring indent, std::unordered_set<int> &node_refs)
    {
        (void)v; (void)indent; (void)node_refs;
        return "null";
    }

    template<typename T>
    static cstring generate(const typename 
            std::enable_if<
                    !std::is_pointer<T>::value &&
                    (std::is_base_of<IR::Node, T>::value || has_toJSON<T>::value) && 
                    !is_vector<T>::value &&
                    !is_ir_vector<T>::value, T>::type
            & v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        ss << "{" << std::endl
           << v.toJSON(indent + "    ", node_refs) << std::endl 
           << indent << "}";
        return ss.str();
    }

    template<typename T>
    static cstring generate(const typename 
            std::enable_if<
                    std::is_pointer<T>::value &&
                    (std::is_base_of<IR::Node, typename std::remove_cv<
                                                    typename std::remove_pointer<T>::type
                                               >::type
                    >::value || has_toJSON<T>::value) &&
                    !is_vector<T>::value, T>::type
            v, cstring indent, std::unordered_set<int> &node_refs)
    {
        std::stringstream ss;
        ss << "{" << std::endl
           << v->toJSON(indent + "    ", node_refs) << std::endl 
           << indent << "}";
        return ss.str();

    }

    template<typename T>
    static cstring generate(const typename 
            std::enable_if<
                    !std::is_pointer<T>::value &&
                    (std::is_base_of<IR::Node, T>::value ||
                    has_toJSON<T>::value) &&
                    !is_vector<T>::value, T>::type
            * v, cstring indent, std::unordered_set<int> &node_refs)
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
            ss << indent << generate<T>(v[0], indent + "    ", node_refs);
            for (size_t i = 1; i < N; i++) {
                ss  << "," << std::endl 
                    << indent << generate<T>(v[i], indent + "    ", node_refs);
            }
            ss << std::endl;
        }
        ss << "]";
        return ss.str();
    }

    struct JsonData {
        cstring field_name;
        enum JsonDataType {
            INT_DATA,
            FLOAT_DATA,
            STRING_TYPE,
            BOOLEAN_TYPE,
            ARRAY_TYPE,
            OBJECT_TYPE,
            NULL_TYPE
        }
        enum JsonDataType type;
        union {
            cstring string_data;
            long long int integer_data;
            double float_data;
            bool bool_data;
            JsonData* array_data;
            JsonData* object_data;
        }
    }
    

    cstring parse_string(char * s) {
        assert(*s == '"');
        char* end = strchr(s + 1, "\"");
        *end = '\0';
        cstring ret(s + 1);
        *end = "\"";
        return ret;
    }

    char* shift_to_next(char* s, char c) {
        return strchr(s, c);
    }
    char* shift_to_non_whitespace(char * s)
    {   
        std::string str(s);
        size_t idx = str.find_first_not_of(" \t\n\r");
        return idx != std::string:npos ? s + idx : nullptr;
    }

    JsonData* parseJsonText(cstring json) {
        JsonData* data = new JsonData;
        char* ch = json.find('{'), *ch2;
        if (ch != nullptr 
                && (ch2 = json.find('[')) != nullptr && ch2 > ch)
            ch = shift_to_next(ch, '"');
            data->field_name = parse_string(s + 1);
            ch = shift_to_next(ch, ':');
            ch = shift_to_non_whitespace(ch);
            assert(ch != nullptr);
            switch
            
            
            
        } else if ((start = json.find("[") != nullptr) {
            
        } else
            assert(true);
    }
};


#endif /* _IR_JSON_GENERATOR_H_ */
