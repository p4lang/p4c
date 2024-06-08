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

#include "lib/path.h"

#include <gtest/gtest.h>

namespace Util {

TEST(Util, PathName) {
#if _WIN32
    FAIL() << "PathName tests for WIN32 not yet written";
#endif

    {
        PathName path = "/usr/local/bin/file.exe";
        auto ext = path.extension();
        EXPECT_EQ(".exe", ext);

        PathName file = path.filename();
        EXPECT_EQ("file.exe", file);

        auto base = path.stem();
        EXPECT_EQ("file", base);

        PathName folder = path.parent_path();
        EXPECT_EQ("/usr/local/bin", folder);
    }

    {
        PathName path = "/usr/local/bin/";
        auto ext = path.extension();
        EXPECT_EQ("", ext);

        PathName file = path.filename();
        EXPECT_EQ("", file);

        auto base = path.stem();
        EXPECT_EQ("", base);

        PathName folder = path.parent_path();
        EXPECT_EQ("/usr/local/bin", folder);
    }

    {
        PathName path = "file.exe";
        auto ext = path.extension();
        EXPECT_EQ(".exe", ext);

        PathName file = path.filename();
        EXPECT_EQ("file.exe", file);

        auto base = path.stem();
        EXPECT_EQ("file", base);

        PathName folder = path.parent_path();
        EXPECT_EQ("", folder);
    }

    {
        PathName path = "";
        EXPECT_TRUE(path.empty());
        PathName grow = path / "x";
        EXPECT_EQ("x", grow);
        EXPECT_FALSE(grow.empty());

        grow = grow / "y";
        EXPECT_EQ("x/y", grow);

        path = PathName("x/");
        grow = path / "y";
        EXPECT_EQ("x/y", grow);
    }
}

}  // namespace Util
