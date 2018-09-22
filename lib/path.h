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

#ifndef P4C_LIB_PATH_H_
#define P4C_LIB_PATH_H_

/*
  It's 2015 and C++ still does not have a portable way to manipulate pathnames.
  This code is not portable, but at least the interfaces should be.
*/

#include "cstring.h"

namespace Util {
// Represents a filename path, e.g., /usr/local/bin/file.exe
class PathName final {
 private:
    static const char pathSeparators[2];
    cstring str;

    const char* findLastSeparator() const;

 public:
    static inline cstring separator() {
#ifdef _WIN32
        return "\\";
#else
        return "/";
#endif
    }

    PathName(cstring str) : str(str)  {}             // NOLINT(runtime/explicit)
    PathName(const char* str) : str(str) {}          // NOLINT(runtime/explicit)
    PathName(const std::string &str) : str(str) {}   // NOLINT(runtime/explicit)
    // get the file name extension.  It starts at the last dot.
    // e.g, exe
    cstring getExtension() const;
    // extract just the filename, including the extension
    // e.g., file.exe
    PathName getFilename() const;
    // extract the filename without folder, excluding the extension
    // e.g., file
    cstring getBasename() const;
    // extract the folder
    // e.g., /usr/local/bin
    PathName getFolder() const;
    cstring toString() const { return str; }
    bool isNullOrEmpty() const { return str.isNullOrEmpty(); }
    bool operator==(const PathName &other) const { return str == other.str; }
    bool operator!=(const PathName &other) const { return str != other.str; }
    PathName join(cstring component) const;

    static PathName empty;
};
}  // namespace Util

#endif /* P4C_LIB_PATH_H_ */
