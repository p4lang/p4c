/*
Copyright 2020-present Barefoot Networks, Inc.

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

#include "exename.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "exceptions.h"

template <size_t N>
static void convertToAbsPath(const char *const relPath, char (&output)[N]) {
    output[0] = '\0';  // Default to the empty string, indicating failure.

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return;
    const size_t cwdLen = strlen(cwd);
    if (cwdLen == 0) return;
    const char *separator = cwd[cwdLen - 1] == '/' ? "" : "/";

    // Construct an absolute path. We're assuming that @relPath is relative to
    // the current working directory.
    int n = snprintf(output, N, "%s%s%s", cwd, separator, relPath);
    BUG_CHECK(n >= 0, "Pathname too long");
}

const char *exename(const char *argv0) {
    // Leave 1 extra char for the \0
    static char buffer[PATH_MAX + 1];
    if (buffer[0]) return buffer;  // done already
    int len;
    /* find the path of the executable.  We use a number of techniques that may fail
     * or work on different systems, and take the first working one we find.  Fall
     * back to not overriding the compiled-in installation path */
    if ((len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1)) > 0 ||
        (len = readlink("/proc/curproc/exe", buffer, sizeof(buffer) - 1)) > 0 ||
        (len = readlink("/proc/curproc/file", buffer, sizeof(buffer) - 1)) > 0 ||
        (len = readlink("/proc/self/path/a.out", buffer, sizeof(buffer) - 1)) > 0) {
        buffer[len] = 0;
    } else if (argv0 && argv0[0] == '/') {
        snprintf(buffer, sizeof(buffer), "%s", argv0);
    } else if (argv0 && strchr(argv0, '/')) {
        convertToAbsPath(argv0, buffer);
    } else if (getenv("_")) {
        strncpy(buffer, getenv("_"), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = 0;
    } else {
        buffer[0] = 0;
    }
    return buffer;
}
