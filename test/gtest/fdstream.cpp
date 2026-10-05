// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/fdstream.h"

#include <fcntl.h>
// POSIX and GNU stdio extensions use the C header.
// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stdio.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <system_error>

namespace P4::Test {
namespace {

// GoogleTest assertions add branches that inflate the complexity of these tests.
// NOLINTBEGIN(readability-function-cognitive-complexity)

constexpr int STREAM_BUFFER_SIZE = 8192;
constexpr std::size_t LARGE_INPUT_SIZE = std::size_t{4} * STREAM_BUFFER_SIZE;
constexpr int PEEK_COUNT = 10;

using File = std::unique_ptr<FILE, int (*)(FILE *)>;

class FdStreamTest : public testing::TestWithParam<bool> {
    File fileOwner{std::tmpfile(), &std::fclose};

 protected:
    FILE *file() { return fileOwner.get(); }

    void SetUp() override { ASSERT_NE(file(), nullptr); }

    FdStream stream() { return GetParam() ? FdStream(file()) : FdStream(fileno(file())); }
};

TEST_P(FdStreamTest, FormattedInputAcrossBufferBoundaries) {
    const std::string word(LARGE_INPUT_SIZE, 'a');
    const std::string text = std::string(STREAM_BUFFER_SIZE - 1, ' ') + "12345 " + word + " 67";
    ASSERT_EQ(std::fwrite(text.data(), 1, text.size(), file()), text.size());
    ASSERT_EQ(std::fseek(file(), 0, SEEK_SET), 0);
    auto input = stream();
    for (int i = 0; i < PEEK_COUNT; ++i) {
        EXPECT_EQ(input.peek(), ' ');
    }
    int first = 0;
    int last = 0;
    std::string parsed;
    input >> first >> parsed >> last;
    EXPECT_FALSE(input.fail());
    EXPECT_EQ(first, 12345);
    EXPECT_EQ(parsed, word);
    EXPECT_EQ(last, 67);
    EXPECT_TRUE(input.eof());
    EXPECT_FALSE(input.bad());
}

TEST_P(FdStreamTest, BinaryInputAndPutback) {
    std::string text;
    for (std::size_t i = 0; i < LARGE_INPUT_SIZE; ++i) {
        text.push_back(static_cast<char>(i));
    }
    ASSERT_EQ(std::fwrite(text.data(), 1, text.size(), file()), text.size());
    ASSERT_EQ(std::fseek(file(), 0, SEEK_SET), 0);
    auto input = stream();
    EXPECT_EQ(input.get(), 0);
    input.unget();
    ASSERT_TRUE(input);
    std::string actual(text.size(), '\0');
    input.read(actual.data(), static_cast<std::streamsize>(actual.size()));
    EXPECT_EQ(input.gcount(), static_cast<std::streamsize>(text.size()));
    EXPECT_EQ(actual, text);
    input.unget();
    EXPECT_EQ(input.get(), 255);
    EXPECT_EQ(input.peek(), std::char_traits<char>::eof());
    EXPECT_FALSE(input.bad());
}

TEST_P(FdStreamTest, EmptyInputAndBorrowedOwnership) {
    {
        auto input = stream();
        EXPECT_EQ(input.peek(), std::char_traits<char>::eof());
        EXPECT_TRUE(input.eof());
        EXPECT_FALSE(input.bad());
    }
    // fcntl uses a variadic POSIX interface for descriptor operations.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    EXPECT_NE(fcntl(fileno(file()), F_GETFD), -1);
    ASSERT_EQ(std::fseek(file(), 0, SEEK_SET), 0);
    EXPECT_EQ(std::fputc('x', file()), 'x');
}

TEST_P(FdStreamTest, ShortBlockReadAtEof) {
    ASSERT_GE(std::fputs("abc", file()), 0);
    ASSERT_EQ(std::fseek(file(), 0, SEEK_SET), 0);
    auto input = stream();
    EXPECT_EQ(input.peek(), 'a');
    std::array<char, sizeof("abc")> actual{};
    input.read(actual.data(), static_cast<std::streamsize>(actual.size()));
    EXPECT_EQ(input.gcount(), 3);
    EXPECT_EQ(std::string(actual.data(), 3), "abc");
    EXPECT_TRUE(input.eof());
    EXPECT_TRUE(input.fail());
    EXPECT_FALSE(input.bad());
}

TEST_P(FdStreamTest, OutputAcrossBufferBoundariesAndDestructorFlush) {
    std::string text;
    for (std::size_t i = 0; i < LARGE_INPUT_SIZE + 3; ++i) {
        text.push_back(static_cast<char>(i));
    }
    {
        auto output = stream();
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        ASSERT_TRUE(output);
        // An earlier failure must not cause pending output to be lost at cleanup.
        output.setstate(std::ios::failbit);
    }
    ASSERT_EQ(std::fseek(file(), 0, SEEK_SET), 0);
    std::string actual(text.size() + 1, '\0');
    EXPECT_EQ(std::fread(actual.data(), 1, actual.size(), file()), text.size());
    actual.resize(text.size());
    EXPECT_EQ(actual, text);
}

TEST_P(FdStreamTest, ExplicitFlush) {
    {
        auto output = stream();
        output << "hello" << std::flush;
        ASSERT_TRUE(output);
        // Check that flush made the output visible before the stream is destroyed.
        std::array<char, sizeof("hello") - 1> actual{};
        ASSERT_EQ(pread(fileno(file()), actual.data(), actual.size(), 0), 5);
        EXPECT_EQ(std::string(actual.data(), actual.size()), "hello");
    }
}

INSTANTIATE_TEST_SUITE_P(Handles, FdStreamTest, testing::Bool(),
                         [](const auto &info) { return info.param ? "Stdio" : "Descriptor"; });

TEST(FdStream, PreservesStdioBufferAndPushback) {
    File file(std::tmpfile(), &std::fclose);
    ASSERT_NE(file, nullptr);
    ASSERT_GE(std::fputs("prefix 123 456", file.get()), 0);
    ASSERT_EQ(std::fseek(file.get(), 0, SEEK_SET), 0);
    ASSERT_EQ(std::fgetc(file.get()), 'p');
    // The next read must honor the caller's replacement character, even though
    // the file itself still contains the original text.
    ASSERT_EQ(std::ungetc('s', file.get()), 's');
    FdStream input(file.get());
    std::string word;
    int first = 0;
    int last = 0;
    input >> word >> first >> last;
    EXPECT_FALSE(input.fail());
    EXPECT_EQ(word, "srefix");
    EXPECT_EQ(first, 123);
    EXPECT_EQ(last, 456);
}

TEST(FdStream, ReadErrorsAreNotEof) {
    FdStream input(-1);
    EXPECT_EQ(input.peek(), std::char_traits<char>::eof());
    EXPECT_TRUE(input.bad());
    EXPECT_FALSE(input.eof());
    input.clear();
    input.exceptions(std::ios::badbit);
    try {
        input.peek();
        FAIL() << "Expected an I/O exception";
    } catch (const std::ios_base::failure &error) {
        EXPECT_EQ(error.code(), std::error_code(EBADF, std::generic_category()));
    }
}

TEST(FdStream, WriteErrorsAndNonthrowingDestructor) {
    FdStream output(-1);
    output << "pending output" << std::flush;
    EXPECT_TRUE(output.bad());
    output.clear();
    output.exceptions(std::ios::badbit);
    EXPECT_THROW(output.flush(), std::ios_base::failure);
    // Leaving scope must be safe even when writing the remaining output fails.
}

class Pipe {
    std::array<int, 2> fds{-1, -1};

 public:
    Pipe() = default;
    ~Pipe() {
        for (int fd : fds) {
            if (fd >= 0) {
                close(fd);
            }
        }
    }

    Pipe(const Pipe &) = delete;
    Pipe &operator=(const Pipe &) = delete;
    Pipe(Pipe &&) = delete;
    Pipe &operator=(Pipe &&) = delete;

    int open() { return ::pipe(fds.data()); }
    [[nodiscard]] int readFd() const { return fds[0]; }
    [[nodiscard]] int writeFd() const { return fds[1]; }
    void releaseReadFd() { fds[0] = -1; }
};

TEST(FdStream, StdioPipePreservesPartialReadBeforeError) {
    Pipe pipe;
    ASSERT_EQ(pipe.open(), 0);
    // fcntl uses a variadic POSIX interface for descriptor operations.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    ASSERT_NE(fcntl(pipe.readFd(), F_SETFL, O_NONBLOCK), -1);
    File file(fdopen(pipe.readFd(), "r"), &std::fclose);
    ASSERT_NE(file, nullptr);
    pipe.releaseReadFd();
    ASSERT_EQ(write(pipe.writeFd(), "x", 1), 1);
    FdStream input(file.get());
    EXPECT_EQ(input.peek(), 'x');
    EXPECT_EQ(input.get(), 'x');
    // With the writer still open, more input would require waiting. The stream
    // must return 'x' before reporting that it cannot read further without waiting.
    EXPECT_NE(std::ferror(file.get()), 0);
    EXPECT_FALSE(input.bad());
    input.exceptions(std::ios::badbit);
    errno = EINVAL;
    try {
        input.peek();
        FAIL() << "Expected an I/O exception";
    } catch (const std::ios_base::failure &error) {
        EXPECT_EQ(error.code(), std::error_code(EAGAIN, std::generic_category()));
    }
    EXPECT_TRUE(input.bad());
    EXPECT_FALSE(input.eof());
}

#ifdef F_SETPIPE_SZ
TEST(FdStream, PartialWriteRetainsOnlyUnwrittenBytes) {
    constexpr int pageSize = 4096;
    if (sysconf(_SC_PAGESIZE) != pageSize) {
        GTEST_SKIP() << "Requires 4096-byte pipe pages";
    }
    Pipe pipe;
    ASSERT_EQ(pipe.open(), 0);
    // Leave room for only half the output. After draining the pipe, retrying
    // must write exactly the remaining half, without losing or repeating bytes.
    // fcntl uses a variadic POSIX interface for descriptor operations.
    // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    ASSERT_EQ(fcntl(pipe.writeFd(), F_SETPIPE_SZ, STREAM_BUFFER_SIZE), STREAM_BUFFER_SIZE);
    ASSERT_NE(fcntl(pipe.writeFd(), F_SETFL, O_NONBLOCK), -1);
    ASSERT_NE(fcntl(pipe.readFd(), F_SETFL, O_NONBLOCK), -1);
    // NOLINTEND(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    const std::string prefix(pageSize, 'p');
    ASSERT_EQ(write(pipe.writeFd(), prefix.data(), prefix.size()), pageSize);
    const std::string text = std::string(pageSize, 'a') + std::string(pageSize, 'b');
    FdStream output(pipe.writeFd());
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    output.flush();
    ASSERT_TRUE(output.bad());
    std::string first(STREAM_BUFFER_SIZE, '\0');
    ASSERT_EQ(read(pipe.readFd(), first.data(), first.size()), STREAM_BUFFER_SIZE);
    EXPECT_EQ(first, prefix + text.substr(0, pageSize));
    output.clear();
    output.flush();
    ASSERT_TRUE(output);
    std::string rest(STREAM_BUFFER_SIZE, '\0');
    ASSERT_EQ(read(pipe.readFd(), rest.data(), rest.size()), pageSize);
    rest.resize(pageSize);
    EXPECT_EQ(rest, text.substr(pageSize));
}
#endif

#ifdef __GLIBC__
// stdio.h provides this type through a private glibc header.
// NOLINTNEXTLINE(misc-include-cleaner)
using CookieFunctions = cookie_io_functions_t;

TEST(FdStream, StdioFlushErrorsAreReported) {
    for (int error : {ENOSPC, EINTR}) {
        SCOPED_TRACE(error);
        CookieFunctions functions{};
        functions.write = [](void *cookie, const char *, std::size_t) -> ssize_t {
            errno = *static_cast<int *>(cookie);
            return -1;
        };
        File file(fopencookie(&error, "w", functions), &std::fclose);
        ASSERT_NE(file, nullptr);
        FdStream output(file.get());
        output.exceptions(std::ios::badbit);
        try {
            output << "pending output" << std::flush;
            FAIL() << "Expected an I/O exception";
        } catch (const std::ios_base::failure &failure) {
            EXPECT_EQ(failure.code(), std::error_code(error, std::generic_category()));
        }
        EXPECT_TRUE(output.bad());
    }
}

TEST(FdStream, RetriesInterruptedStdioReadAndReportsRealError) {
    int calls = 0;
    CookieFunctions functions{};
    functions.read = [](void *cookie, char *buffer, std::size_t) -> ssize_t {
        auto &calls = *static_cast<int *>(cookie);
        switch (++calls) {
            case 1:
                errno = EINTR;
                return -1;
            case 2:
                *buffer = 'x';
                return 1;
            default:
                errno = EIO;
                return -1;
        }
    };
    File file(fopencookie(&calls, "r", functions), &std::fclose);
    ASSERT_NE(file, nullptr);
    FdStream input(file.get());
    input.exceptions(std::ios::badbit);
    EXPECT_EQ(input.get(), 'x');
    EXPECT_EQ(calls, 3);
    errno = EINVAL;
    try {
        input.get();
        FAIL() << "Expected an I/O exception";
    } catch (const std::ios_base::failure &error) {
        EXPECT_EQ(error.code(), std::error_code(EIO, std::generic_category()));
    }
    EXPECT_FALSE(input.eof());
}

TEST(FdStream, InterruptedStdioReadPreservesPartialInput) {
    int calls = 0;
    CookieFunctions functions{};
    functions.read = [](void *cookie, char *buffer, std::size_t) -> ssize_t {
        auto &calls = *static_cast<int *>(cookie);
        switch (++calls) {
            case 1:
                *buffer = 'x';
                return 1;
            case 2:
                errno = EINTR;
                return -1;
            case 3:
                *buffer = 'y';
                return 1;
            default:
                return 0;
        }
    };
    File file(fopencookie(&calls, "r", functions), &std::fclose);
    ASSERT_NE(file, nullptr);
    FdStream input(file.get());
    input.exceptions(std::ios::badbit);
    std::string text;
    input >> text;
    EXPECT_EQ(text, "xy");
    EXPECT_TRUE(input.eof());
    EXPECT_FALSE(input.fail());
}
#endif

// NOLINTEND(readability-function-cognitive-complexity)

}  // namespace
}  // namespace P4::Test
