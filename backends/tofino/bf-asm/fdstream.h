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

#ifndef BACKENDS_TOFINO_BF_ASM_FDSTREAM_H_
#define BACKENDS_TOFINO_BF_ASM_FDSTREAM_H_

#include <sys/socket.h>
#include <sys/types.h>

#include <functional>
#include <iostream>
#include <streambuf>

class fdstream : public std::iostream {
    struct buffer_t : public std::basic_streambuf<char> {
        int fd;

     public:
        explicit buffer_t(int _fd) : fd(_fd) {}
        ~buffer_t() {
            delete[] eback();
            delete[] pbase();
        }
        int sync();
        int_type underflow();
        int_type overflow(int_type c = traits_type::eof());
        void reset() {
            setg(eback(), eback(), eback());
            setp(pbase(), epptr());
        }
    } buffer;
    std::function<void()> closefn;

 public:
    explicit fdstream(int fd = -1) : std::iostream(&buffer), buffer(fd) { init(&buffer); }
    ~fdstream() {
        if (closefn) closefn();
    }
    void connect(int fd) {
        flush();
        buffer.reset();
        buffer.fd = fd;
    }
    void setclose(std::function<void()> fn) { closefn = fn; }
};

#endif /* BACKENDS_TOFINO_BF_ASM_FDSTREAM_H_ */
