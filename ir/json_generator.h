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
//    static cstring generate(int v) { return std::to_string(v); }
//    static cstring generate(unsigned v) { return std::to_string(v); }
//    static cstring generate(long int v) { return std::to_string(v); }
//    static cstring generate(unsigned long v) { return std::to_string(v); }
//    static cstring generate(float v) { return std::to_string(v); }
//    static cstring generate(double v) { return std::to_string(v); } 
//    static cstring generate(const cstring &s) { return "\"" + s + "\""; }
//    static cstring generate(const match_t &m) {}
//
    template<typename>
    struct is_vector : std::false_type {};
    template<typename>
    struct is_ordered_map : std::false_type {};

    template<typename T, typename A>
    struct is_vector<std::vector<T, A>> : std::true_type {};

    template<typename K, typename V>
    struct is_ordered_map<ordered_map<K, V>> : std::true_type {};
    
    template<typename T> 
    static cstring generate(const typename std::enable_if<
                                        std::is_integral<T>::value ||
                                        std::is_same<mpz_class, T>::value ||
                                       std::is_floating_point<T>::value, T>::type
                            &v, cstring)
    {   
        std::stringstream ss;
        ss << v;
        return ss.str();
    }

    template<typename T>
    static cstring generate(const typename std::enable_if<
                    !std::is_integral<T>::value &&
                    !std::is_same<mpz_class, T>::value &&
                    !std::is_floating_point<T>::value, T>::type 
                    & v, cstring type_name)
    {
        (void)v;
        std::stringstream ss;
        ss  << "{ \"Node_Type\" : \"" << type_name << "\", \"Data\" : \"";
        ss << v; 
        ss << "\" }";
        return ss.str();
    }


    template<typename T, size_t N>
    static cstring generate(const typename std::enable_if<
                                    !std::is_integral<T>::value &&
                                    !std::is_floating_point<T>::value, T>::type (&v)[N], cstring type_name)
    {
        (void)v;
        std::stringstream ss;
        ss  << "{ \"Node_Type\" : \"" << type_name << "\", \"Data\" : \"" << type_name << "\" }";
        return ss.str();
    }
};

#endif /* _IR_JSON_GENERATOR_H_ */
