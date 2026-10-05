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

#ifndef LIB_FDSTREAM_H_
#define LIB_FDSTREAM_H_

#include <array>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <streambuf>
#include <variant>

namespace P4 {

/// Buffered C++ input and output for a file descriptor or FILE* owned by the calling context.
/// Keep the handle open until this object is destroyed; closing it remains the  calling context's
/// responsibility. This stream can read ahead, avoid reading or writing through another stream
/// using the same handle while it is in use.
///
/// This stream reads several characters at once to reduce the cost of reading them individually.
/// For FILE*, it also reads any unread characters already held in the FILE* buffer.
/// On a blocking pipe opened as FILE*, a read can wait for a whole chunk of data or for the writer
/// to close the pipe, even when some characters are already available. This can delay responses
/// when the writer expects a reply before sending more data.
///
/// Calling seekg() or seekp() on this stream fails: it cannot change the current file position.
/// Do not reposition the underlying handle with fseek() or lseek() while this stream is in use,
/// since its buffers would no longer match the handle's position.
/// For regular files, use each instance only for reading or only for writing. Reading ahead
/// advances the file position, so switching to writing could write at an unexpected location.
/// Socket descriptors can support both through one instance because sending and receiving
/// do not share a file position.
///
/// Read and write errors set std::ios::badbit. By default, they do not throw: check stream.bad()
/// to distinguish an I/O error from the end of input. To receive exceptions with the underlying
/// error code, call stream.exceptions(std::ios::badbit) before I/O, then catch
/// std::ios_base::failure and inspect its code().
///
/// Reads interrupted by a signal are retried automatically, as are writes to file descriptors.
/// Writes through FILE* are not retried after an error: stdio may have discarded some output,
/// so retrying cannot guarantee that all data was written.
/// With nonblocking handles, unavailable input or output space is reported as an error
/// (EAGAIN or EWOULDBLOCK). This stream does not wait for the handle to become ready.
///
/// To verify that all output was written, call stream.flush() and check that stream.fail() is
/// false, or handle the exception if enabled. Do this before destroying the stream: its destructor
/// also tries to flush, but suppresses errors and cannot tell you whether the output was written.
class FdStream final : public std::iostream {
    class Buffer final : public std::streambuf {
        static constexpr std::size_t BUFFER_SIZE = 8192;
        std::variant<int, FILE *> source;
        std::array<char, BUFFER_SIZE> inputBuffer{};
        std::array<char, BUFFER_SIZE> outputBuffer{};
        int stdioReadError = 0;
        bool stdioOutputPending = false;

        std::size_t readStdio(FILE *file);

     protected:
        int_type underflow() override;
        int_type overflow(int_type c) override;
        int sync() override;

     public:
        explicit Buffer(int fd);
        explicit Buffer(FILE *file);
    } buffer;

 public:
    explicit FdStream(int fd);
    explicit FdStream(FILE *file);
    ~FdStream() override;

    FdStream(const FdStream &) = delete;
    FdStream &operator=(const FdStream &) = delete;
    FdStream(FdStream &&) = delete;
    FdStream &operator=(FdStream &&) = delete;
};

}  // namespace P4

#endif  // LIB_FDSTREAM_H_
