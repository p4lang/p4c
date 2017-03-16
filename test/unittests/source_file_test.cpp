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

#include "../../lib/cstring.h"
#include "../../lib/exceptions.h"
#include "../../lib/source_file.h"
#include "test.h"

using namespace Util;

namespace Test {
class TestSourceFile : public TestBase {
    int testSourcePosition() {
        SourcePosition invalid;
        ASSERT_EQ(invalid.isValid(), false);

        SourcePosition position(3, 3);
        ASSERT_EQ(position.toString(), "3:3");

        cstring str = ErrorReporter::instance.format_message("%1% - %2%", position, position);
        ASSERT_EQ(str, "3:3 - 3:3\n");

        SourcePosition next(3, 4);
        ASSERT_EQ(position < next, true);
        ASSERT_EQ(position <= position, true);
        ASSERT_EQ(position == position, true);

        return SUCCESS;
    }

    int testInputSources() {
        InputSources sources;
        sources.appendToLastLine("First line");
        {
            SourcePosition position = sources.getCurrentPosition();
            ASSERT_EQ(position.getLineNumber(), 1);
            ASSERT_EQ(position.getColumnNumber(), 10);
        }

        sources.appendNewline("\n");
        sources.mapLine("fakesource.p4", 5);

        {
            SourcePosition position = sources.getCurrentPosition();
            ASSERT_EQ(position.getLineNumber(), 2);
            ASSERT_EQ(position.getColumnNumber(), 0);
        }

        sources.appendToLastLine("Second line");
        sources.appendNewline("\n");
        sources.appendToLastLine("Third line");
        sources.appendNewline("\n");
        sources.seal();

        ASSERT_EQ(sources.lineCount(), 3);

        cstring fl = sources.getLine(1);
        ASSERT_EQ(fl, "First line\n");

        cstring sl = sources.getLine(2);
        ASSERT_EQ(sl, "Second line\n");

        SourceFileLine original = sources.getSourceLine(3);
        ASSERT_EQ(original.fileName, "fakesource.p4");
        ASSERT_EQ(original.sourceLine, 5);

        return SUCCESS;
    }

    int testSourceInfo() {
        SourcePosition t1_s(1, 1);
        SourcePosition t1_e(1, 5);

        SourceInfo t1 = SourceInfo(t1_s, t1_e);

        SourcePosition t2_s(1, 8);
        SourcePosition t2_e(2, 2);

        SourceInfo t2 = SourceInfo(t2_s, t2_e);

        SourceInfo span = t1 + t2;
        cstring str = span.toDebugString();
        ASSERT_EQ(str, "(1:1)-(2:2)");

        SourceInfo invalid;
        ASSERT_EQ(invalid.isValid(), false);

        return SUCCESS;
    }

 public:
    int run() {
        RUNTEST(testSourcePosition);
        RUNTEST(testSourceInfo);
        RUNTEST(testInputSources);
        return SUCCESS;
    }
};
}  // namespace Test

int main(int, char* []) {
    Test::TestSourceFile test;
    return test.run();
}

