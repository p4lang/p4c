#include <fstream>
#include "nullstream.h"

std::ostream* openFile(cstring name, bool nullOnError) {
    if (name.isNullOrEmpty()) {
        if (nullOnError)
            return new nullstream();
        ::error("Empty name for openFile");
        return nullptr;
    }
    std::ofstream *file = new std::ofstream(name);
    if (!file->good()) {
        ::error("Error writing output to file %1%", name);
        if (nullOnError)
            return new nullstream();
        return nullptr;
    }
    return file;
}
