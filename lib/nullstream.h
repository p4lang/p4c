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
