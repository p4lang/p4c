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

#ifndef P4C_LIB_NULLSTREAM_H_
#define P4C_LIB_NULLSTREAM_H_

#include <streambuf>
#include <ostream>
#include <iostream>
#include "cstring.h"
#include "error.h"

template <class cT, class traits = std::char_traits<cT> >
class basic_nullbuf final: public std::basic_streambuf<cT, traits> {
    typename traits::int_type overflow(typename traits::int_type c) {
        return traits::not_eof(c);  // indicate success
    }
};

template <class cT, class traits = std::char_traits<cT> >
class onullstream final: public std::basic_ostream<cT, traits> {
 public:
    onullstream():
        std::basic_ios<cT, traits>(&m_sbuf),
        std::basic_ostream<cT, traits>(&m_sbuf)
    { this->init(&m_sbuf); }

 private:
    basic_nullbuf<cT, traits> m_sbuf;
};

typedef onullstream<char> nullstream;

// If nullOnError is 'true', on error a nullstream is returned
// otherwise a nullptr is returned
std::ostream* openFile(cstring name, bool nullOnError);

#endif /* P4C_LIB_NULLSTREAM_H_ */
