/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_NULLSTREAM_H_
#define LIB_NULLSTREAM_H_

#include <filesystem>
#include <iostream>
#include <memory>
#include <ostream>
#include <streambuf>

namespace P4 {

template <class cT, class traits = std::char_traits<cT>>
class basic_nullbuf final : public std::basic_streambuf<cT, traits> {
    typename traits::int_type overflow(typename traits::int_type c) {
        return traits::not_eof(c);  // indicate success
    }
};

template <class cT, class traits = std::char_traits<cT>>
class onullstream final : public std::basic_ostream<cT, traits> {
 public:
    onullstream() : std::basic_ios<cT, traits>(&m_sbuf), std::basic_ostream<cT, traits>(&m_sbuf) {
        this->init(&m_sbuf);
    }

 private:
    basic_nullbuf<cT, traits> m_sbuf;
};

typedef onullstream<char> nullstream;

// If nullOnError is 'true', on error a nullstream is returned
// otherwise a nullptr is returned
std::unique_ptr<std::ostream> openFile(const std::filesystem::path &name, bool nullOnError);

}  // namespace P4

#endif /* LIB_NULLSTREAM_H_ */
