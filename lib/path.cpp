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

#include <stdexcept>
#include "path.h"
#include "algorithm.h"

namespace Util {

PathName PathName::empty = PathName("");
const char PathName::pathSeparators[2] = { '/', '\\' };

const char* PathName::findLastSeparator() const {
    const char* found = nullptr;
    // Not the most efficient, but should be fine for short strings
    for (char c : pathSeparators) {
        const char* f = str.findlast(c);
        if (f == nullptr) continue;
        if (found == nullptr)
            found = f;
        else if (f < found)
            found = f;
    }
    return found;
}

PathName PathName::getFilename() const {
    if (str.isNullOrEmpty()) return *this;

    const char* lastSeparator = findLastSeparator();
    if (lastSeparator == 0)
        return *this;

    return PathName(lastSeparator+1);
}

cstring PathName::getExtension() const {
    PathName filename = getFilename();
    if (filename.isNullOrEmpty())
        return filename.str;

    const char* dot = filename.str.findlast('.');
    if (dot == nullptr)
        return cstring::empty;
    return dot+1;
}

PathName PathName::getFolder() const {
    if (str.isNullOrEmpty()) return *this;

    const char* lastSeparator = findLastSeparator();
    if (lastSeparator == 0)
        return PathName::empty;

    return PathName(str.before(lastSeparator));
}

cstring PathName::getBasename() const {
    PathName file = getFilename();
    const char* dot = file.str.findlast('.');
    if (dot == nullptr)
        return file.str;
    return file.str.before(dot);
}

PathName PathName::join(cstring component) const {
    if (component.isNullOrEmpty())
        throw std::logic_error("Empty string for pathname component");
    if (str.isNullOrEmpty())
        return PathName(component);
    char last = str[str.size() - 1];
    for (char c : pathSeparators) {
        if (c == last) {
            auto result = str + component;
            return PathName(result);
        }
    }
    auto result = str + separator() + component;
    return PathName(result);
}

}  // namespace Util
