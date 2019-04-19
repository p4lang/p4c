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
#include "lib/path.h"
#include "lib/exceptions.h"
#include "lib/stringref.h"

namespace Util {

TEST(Util, PathName) {
#if _WIN32
    FAIL() << "PathName tests for WIN32 not yet written";
#endif

    {
        PathName path = "/usr/local/bin/file.exe";
        StringRef ext = path.getExtension();
        EXPECT_EQ(StringRef("exe"), ext);

        PathName file = path.getFilename();
        EXPECT_EQ("file.exe", file.toString());

        StringRef base = path.getBasename();
        EXPECT_EQ(StringRef("file"), base);

        PathName folder = path.getFolder();
        EXPECT_EQ("/usr/local/bin", folder.toString());
    }

    {
        PathName path = "/usr/local/bin/";
        StringRef ext = path.getExtension();
        EXPECT_EQ("", ext);

        PathName file = path.getFilename();
        EXPECT_EQ("", file.toString());

        StringRef base = path.getBasename();
        EXPECT_EQ("", base);

        PathName folder = path.getFolder();
        EXPECT_EQ("/usr/local/bin", folder.toString());
    }

    {
        PathName path = "file.exe";
        StringRef ext = path.getExtension();
        EXPECT_EQ(StringRef("exe"), ext);

        PathName file = path.getFilename();
        EXPECT_EQ("file.exe", file.toString());

        StringRef base = path.getBasename();
        EXPECT_EQ(StringRef("file"), base);

        PathName folder = path.getFolder();
        EXPECT_EQ("", folder.toString());
    }

    {
        PathName path = "";
        EXPECT_TRUE(path.isNullOrEmpty());
        PathName grow = path.join("x");
        EXPECT_EQ("x", grow.toString());
        EXPECT_FALSE(grow.isNullOrEmpty());

        grow = grow.join("y");
        EXPECT_EQ("x/y", grow.toString());

        path = PathName("x/");
        grow = path.join("y");
        EXPECT_EQ("x/y", grow.toString());
    }
}

}  // namespace Util
