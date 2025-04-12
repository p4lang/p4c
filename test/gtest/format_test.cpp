// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/format.h"

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>

#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/stringify.h"

namespace P4::Util {

TEST(Util, Format) {
    auto &context = BaseCompileContext::get();
    cstring message = context.errorReporter().format_message("%1%", 5u);
    EXPECT_EQ("5\n", message);

    message = context.errorReporter().format_message("Number=%1%", 5);
    EXPECT_EQ("Number=5\n", message);

    message = context.errorReporter().format_message("Double=%1% String=%2%", 2.3, "short");
    EXPECT_EQ("Double=2.3 String=short\n", message);

    struct NiceFormat {
        int a, b, c;

        cstring toString() const {
            return "("_cs + Util::toString(this->a) + ","_cs + Util::toString(this->b) + ","_cs +
                   Util::toString(this->c) + ")"_cs;
        }
    };

    NiceFormat nf{1, 2, 3};
    message = context.errorReporter().format_message("Nice=%1%", nf);
    EXPECT_EQ("Nice=(1,2,3)\n", message);

    cstring x = "x"_cs;
    cstring y = "y"_cs;
    message = context.errorReporter().format_message("%1% %2%", x, y);
    EXPECT_EQ("x y\n", message);

    message = context.errorReporter().format_message("%1% %2%", x, 5);
    EXPECT_EQ("x 5\n", message);
}

TEST(Util, FormatWithoutArguments) {
    EXPECT_EQ("", createFormattedMessage(""));
    EXPECT_EQ("literal", createFormattedMessage("literal"));
    EXPECT_EQ("100%", createFormattedMessage("100%%"));
    EXPECT_EQ("100%\n", createBugMessage("100%%"));
    EXPECT_EQ("100%\n", error_helper("100%%").toString());
    EXPECT_THROW(createFormattedMessage("%1%"), std::invalid_argument);
    EXPECT_THROW(createFormattedMessage("%s"), std::invalid_argument);
    EXPECT_THROW(createFormattedMessage("%"), std::invalid_argument);
}

TEST(Util, FormatLegacyAndPrintf) {
    EXPECT_EQ("%7%", createFormattedMessage("%%%1%%%", 7));
    EXPECT_EQ("two one two", createFormattedMessage("%2% %1% %2%", "one", "two"));
    EXPECT_EQ("2.3 A 1", createFormattedMessage("%1% %2% %3%", 2.3, 'A', true));
    EXPECT_EQ("42 2.3", createFormattedMessage("%s %s", 42, 2.3));
    EXPECT_EQ("00042", createFormattedMessage("%05s", 42));
    EXPECT_EQ("-0042", createFormattedMessage("%05s", -42));
    EXPECT_EQ("00abc", createFormattedMessage("%05s", "abc"));
    EXPECT_EQ("a", createFormattedMessage("%c", "abc"));
    EXPECT_EQ("0x002a 3.14", createFormattedMessage("%#06x %.2f", 42, 3.14159));
    EXPECT_EQ("2a 42", createFormattedMessage("%1$x %1%", 42));
    EXPECT_EQ("   2a", createFormattedMessage("%|1$5x|", 42));
    EXPECT_EQ("  abc", createFormattedMessage("%|1$5|", "abc"));
    EXPECT_EQ("abc  ", createFormattedMessage("%|-5s|", "abc"));
    EXPECT_EQ("   3.14", createFormattedMessage("%*.*f", 7, 2, 3.14159));
    EXPECT_EQ("   3.14", createFormattedMessage("%3$*1$.*2$f", 7, 2, 3.14159));
    EXPECT_EQ("42", createFormattedMessage("%llu", 42ULL));
    EXPECT_EQ("*", createFormattedMessage("%c", 42));
    EXPECT_EQ("12", createFormattedMessage("%12%", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
    std::string runtimeFormat = "%2% %1$x";
    EXPECT_EQ("answer 2a", createFormattedMessage(runtimeFormat.c_str(), 42, "answer"));
}

TEST(Util, FormatInvalid) {
    for (auto format : {"%", "%0%", "%2%", "%9999999999999999999999999%", "%q", "%1", "%|1$5x",
                        "%|5t|", "%hhhhd", "%1% %s", "%s %1%", "literal", "%%"}) {
        SCOPED_TRACE(format);
        EXPECT_THROW(createFormattedMessage(format, 1), std::invalid_argument);
    }
    EXPECT_THROW(createFormattedMessage("%1%"), std::invalid_argument);
    EXPECT_THROW(createFormattedMessage("%1%", 1, 2), std::invalid_argument);
    EXPECT_THROW(createFormattedMessage("%d", 3.14), std::invalid_argument);
    EXPECT_THROW(createFormattedMessage("%n", 1), std::invalid_argument);
}

namespace {
struct FormatObject {
    cstring toString() const { return "user"_cs; }
    void dbprint(std::ostream &out) const { out << "debug"; }
};

struct NoncopyableFormatObject {
    NoncopyableFormatObject() = default;
    NoncopyableFormatObject(const NoncopyableFormatObject &) = delete;
    cstring toString() const { return "noncopyable"_cs; }
};

struct StreamAndStringObject {
    cstring toString() const { return "user"_cs; }
    friend std::ostream &operator<<(std::ostream &out, const StreamAndStringObject &) {
        return out << "stream";
    }
};
}  // namespace

TEST(Util, FormatObjectsAndPointers) {
    FormatObject object;
    EXPECT_EQ("user user", createFormattedMessage("%1% %2%", object, &object));
    EXPECT_EQ("user", createFormattedMessage("%v", object));
    EXPECT_EQ("42", createFormattedMessage("%v", 42));
    EXPECT_EQ("debug debug", createFormattedMessage<true>("%1% %2%", object, &object));
    EXPECT_EQ("debug\n", createBugMessage("%1%", object));
    StreamAndStringObject streamAndString;
    EXPECT_EQ("user", createFormattedMessage("%1%", streamAndString));
    EXPECT_EQ("stream", createFormattedMessage<true>("%1%", streamAndString));
    NoncopyableFormatObject noncopyable;
    EXPECT_EQ("noncopyable", createFormattedMessage("%1%", noncopyable));
    EXPECT_EQ("noncopyable", createFormattedMessage("%s", NoncopyableFormatObject{}));
    EXPECT_EQ("<nullptr>", createFormattedMessage("%1%", static_cast<FormatObject *>(nullptr)));
    EXPECT_EQ("<nullptr>", createFormattedMessage("%s", static_cast<const char *>(nullptr)));
    EXPECT_EQ("<nullptr>", createFormattedMessage("%1%", nullptr));
    int value = 42;
    EXPECT_EQ("42", createFormattedMessage("%1%", &value));
    std::ostringstream address;
    address << &value;
    EXPECT_EQ(address.str(), createFormattedMessage<true>("%1%", &value));
    EXPECT_EQ(address.str(), createFormattedMessage("%p", &value));
    EXPECT_EQ(address.str(), createFormattedMessage("%1%", static_cast<void *>(&value)));
    // Stream formatting must not leak across arguments or repeated placeholders.
    big_int large = 42;
    EXPECT_EQ("2a 42", createFormattedMessage("%1$x %1%", large));
    EXPECT_EQ("00000x2a", createFormattedMessage("%#08x", large));
    EXPECT_EQ(" 42", createFormattedMessage("% d", large));
}

TEST(Util, FormatSourceLocations) {
    InputSources sources;
    sources.mapLine("format.p4", 1);
    sources.appendText("first\nsecond\n");
    sources.seal();
    SourceInfo first(&sources, SourcePosition(1, 0), SourcePosition(1, 5));
    SourceInfo second(&sources, SourcePosition(2, 0), SourcePosition(2, 6));
    const auto expected =
        absl::StrCat(first.toPositionString(), ": message \n", first.toSourceFragment(),
                     second.toPositionString(), "\n", second.toSourceFragment());
    EXPECT_EQ(expected, createBugMessage("%1%message %2%%3%%4%", first, "", second, SourceInfo{}));
    EXPECT_EQ(expected,
              error_helper("%1%message %2%%3%%4%", &first, "", &second, SourceInfo{}).toString());
    EXPECT_EQ("x", createFormattedMessage("%1%x", SourceInfo{}));
    EXPECT_EQ("x", createFormattedMessage("%sx", SourceInfo{}));
    EXPECT_EQ(expected,
              createBugMessage("%1%message %2%%3%%4%", &first, "", &second, SourceInfo{}));
    std::string prefix = "prefix: ", suffix = "suffix";
    auto message = error_helper("%1%", prefix, first, suffix, "body");
    EXPECT_EQ("prefix: ", message.prefix);
    EXPECT_EQ("body", message.message);
    EXPECT_EQ("suffix", message.suffix);
    ASSERT_EQ(1U, message.locations.size());
    EXPECT_EQ(first, message.locations.front());
}

}  // namespace P4::Util
