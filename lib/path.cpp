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
