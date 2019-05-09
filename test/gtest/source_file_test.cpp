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

#include "gtest/gtest.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"
#include "lib/compile_context.h"

namespace Util {

TEST(UtilSourceFile, SourcePosition) {
    SourcePosition invalid;
    EXPECT_FALSE(invalid.isValid());

    SourcePosition position(3, 3);
    EXPECT_EQ("3:3", position.toString());

    auto& context = BaseCompileContext::get();
    cstring str = context.errorReporter().format_message("%1% - %2%", position, position);
    EXPECT_EQ("3:3 - 3:3\n", str);

    SourcePosition next(3, 4);
    EXPECT_LT(position, next);
    EXPECT_LE(position, position);
    EXPECT_EQ(position, position);
}

TEST(UtilSourceFile, InputSources) {
    Util::InputSources sources;
    sources.appendToLastLine("First line");
    {
        SourcePosition position = sources.getCurrentPosition();
        EXPECT_EQ(1u, position.getLineNumber());
        EXPECT_EQ(10u, position.getColumnNumber());
    }

    sources.appendNewline("\n");
    sources.mapLine("fakesource.p4", 5);

    {
        SourcePosition position = sources.getCurrentPosition();
        EXPECT_EQ(2u, position.getLineNumber());
        EXPECT_EQ(0u, position.getColumnNumber());
    }

    sources.appendToLastLine("Second line");
    sources.appendNewline("\n");
    sources.appendToLastLine("Third line");
    sources.appendNewline("\n");
    sources.seal();

    EXPECT_EQ(3u, sources.lineCount());

    cstring fl = sources.getLine(1);
    EXPECT_EQ("First line\n", fl);

    cstring sl = sources.getLine(2);
    EXPECT_EQ("Second line\n", sl);

    SourceFileLine original = sources.getSourceLine(3);
    EXPECT_EQ("fakesource.p4", original.fileName);
    EXPECT_EQ(5u, original.sourceLine);
}

TEST(UtilSourceFile, SourceInfo) {
    Util::InputSources sources;

    SourcePosition t1_s(1, 1);
    SourcePosition t1_e(1, 5);

    SourceInfo t1 = SourceInfo(&sources, t1_s, t1_e);

    SourcePosition t2_s(1, 8);
    SourcePosition t2_e(2, 2);

    SourceInfo t2 = SourceInfo(&sources, t2_s, t2_e);

    SourceInfo span = t1 + t2;
    cstring str = span.toDebugString();
    EXPECT_EQ("(1:1)-(2:2)", str);

    SourceInfo invalid;
    EXPECT_FALSE(invalid.isValid());
}

}  // namespace Util
