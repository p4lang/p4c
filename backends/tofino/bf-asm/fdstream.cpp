/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fdstream.h"

#include <unistd.h>

#include <cstring>

#define BUFSIZE 1024

fdstream::buffer_t::int_type fdstream::buffer_t::underflow() {
    if (!gptr()) {
        char_type *n = new char_type[BUFSIZE];
        setg(n, n, n);
    } else if (gptr() != egptr()) {
        size_t len = egptr() - gptr();
        if (len > 0) std::memmove(eback(), gptr(), len * sizeof(char_type));
        setg(eback(), eback(), eback() + len);
    } else {
        setg(eback(), eback(), eback());
    }
    int rv = ::read(fd, egptr(), eback() + BUFSIZE - egptr());
    if (rv > 0)
        setg(eback(), eback(), egptr() + rv);
    else if (gptr() == egptr())
        return traits_type::eof();
    return traits_type::to_int_type(*gptr());
}

fdstream::buffer_t::int_type fdstream::buffer_t::overflow(fdstream::buffer_t::int_type c) {
    if (!pptr()) {
        char_type *n = new char_type[BUFSIZE];
        setp(n, n + BUFSIZE);
    }
    if (pptr() != pbase()) {
        int rv = ::write(fd, pbase(), pptr() - pbase());
        if (rv <= 0) return traits_type::eof();
        if (pbase() + rv == pptr())
            setp(pbase(), epptr());
        else {
            size_t len = pptr() - pbase() + rv;
            std::memmove(pbase(), pbase() + rv, len);
            setp(pbase(), epptr());
            pbump(len);
        }
    }
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        *pptr() = c;
        pbump(1);
        return c;
    } else {
        return traits_type::not_eof(c);
    }
}

int fdstream::buffer_t::sync() {
    char *p = pbase(), *e = pptr();
    while (p != e) {
        int rv = ::write(fd, p, e - p);
        if (rv <= 0) {
            if (p != pbase()) std::memmove(pbase(), p, e - p);
            setp(pbase(), epptr());
            pbump(e - p);
            return -1;
        }
        p += rv;
    }
    setp(pbase(), epptr());
    return 0;
}
