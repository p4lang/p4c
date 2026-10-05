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

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ios>
#include <iostream>
#include <span>
#include <stdexcept>
#include <system_error>
#include <variant>

namespace P4 {
namespace {

std::error_code ioError(int error) { return {error != 0 ? error : EIO, std::generic_category()}; }

}  // namespace

FdStream::Buffer::Buffer(int fd) : source(fd) {
    setp(outputBuffer.data(), outputBuffer.data() + outputBuffer.size());
}

FdStream::Buffer::Buffer(FILE *file) : source(file) {
    if (file == nullptr) {
        throw std::invalid_argument("FdStream requires a non-null FILE*");
    }
    setp(outputBuffer.data(), outputBuffer.data() + outputBuffer.size());
}

FdStream::FdStream(int fd) : std::iostream(nullptr), buffer(fd) { init(&buffer); }

FdStream::FdStream(FILE *file) : std::iostream(nullptr), buffer(file) { init(&buffer); }

FdStream::~FdStream() {
    try {
        // Try to write remaining output even if reading has ended or failed.
        buffer.pubsync();
    } catch (...) {  // NOLINT(bugprone-empty-catch)
        // Cleanup must not throw, even while handling another exception.
    }
}

std::size_t FdStream::Buffer::readStdio(FILE *file) {
    std::size_t count = 0;
    while (stdioReadError == 0 && count == 0) {
        errno = 0;
        count = std::fread(inputBuffer.data(), 1, inputBuffer.size(), file);
        if (std::ferror(file) == 0) {
            break;
        }
        if (errno != EINTR) {
            // Keep data read before the failure, and report the original
            // error once that data has been consumed.
            stdioReadError = errno != 0 ? errno : EIO;
            break;
        }
        std::clearerr(file);
    }
    if (count == 0 && stdioReadError != 0) {
        throw std::ios_base::failure("Error reading stdio stream", ioError(stdioReadError));
    }
    return count;
}

FdStream::Buffer::int_type FdStream::Buffer::underflow() {
    // Peeking must leave the next character available for a later read.
    if (gptr() != egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    std::size_t count = 0;
    if (const auto *fd = std::get_if<int>(&source)) {
        auto result = ::read(*fd, inputBuffer.data(), inputBuffer.size());
        while (result < 0 && errno == EINTR) {
            result = ::read(*fd, inputBuffer.data(), inputBuffer.size());
        }
        if (result < 0) {
            throw std::ios_base::failure("Error reading file descriptor", ioError(errno));
        }
        count = static_cast<std::size_t>(result);
    } else {
        count = readStdio(std::get<FILE *>(source));
    }
    if (count == 0) {
        return traits_type::eof();
    }
    setg(inputBuffer.data(), inputBuffer.data(), inputBuffer.data() + count);
    return traits_type::to_int_type(*gptr());
}

FdStream::Buffer::int_type FdStream::Buffer::overflow(int_type c) {
    sync();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        *pptr() = traits_type::to_char_type(c);
        pbump(1);
    }
    return traits_type::not_eof(c);
}

int FdStream::Buffer::sync() {
    std::span<const char> pending(pbase(), static_cast<std::size_t>(pptr() - pbase()));
    while (!pending.empty()) {
        std::size_t written = 0;
        int error = 0;
        if (const auto *fd = std::get_if<int>(&source)) {
            const auto result = ::write(*fd, pending.data(), pending.size());
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                error = errno;
            } else {
                written = static_cast<std::size_t>(result);
            }
        } else {
            auto *file = std::get<FILE *>(source);
            errno = 0;
            written = std::fwrite(pending.data(), 1, pending.size(), file);
            stdioOutputPending |= written != 0;
            if (std::ferror(file) != 0) {
                error = errno != 0 ? errno : EIO;
            }
        }
        pending = pending.subspan(written);
        if (error != 0 || written == 0) {
            // Save the bytes that were not written.
            // This way a retry can continue without repeating or losing output.
            traits_type::move(outputBuffer.data(), pending.data(), pending.size());
            setp(outputBuffer.data(), outputBuffer.data() + outputBuffer.size());
            pbump(static_cast<int>(pending.size()));
            throw std::ios_base::failure("Error writing stream", ioError(error));
        }
    }
    setp(outputBuffer.data(), outputBuffer.data() + outputBuffer.size());
    if (stdioOutputPending) {
        auto *file = std::get<FILE *>(source);
        // stdio may discard buffered output when flushing fails. Report the
        // failure: a retry could appear to succeed even though data was lost.
        errno = 0;
        if (std::fflush(file) != 0) {
            throw std::ios_base::failure("Error flushing stdio stream", ioError(errno));
        }
        stdioOutputPending = false;
    }
    return 0;
}

}  // namespace P4
