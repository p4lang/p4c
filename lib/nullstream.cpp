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

#include "nullstream.h"

#include <fstream>  // IWYU pragma: keep

#include "lib/error.h"

namespace p4c {

std::ostream *openFile(const std::filesystem::path &name, bool nullOnError) {
    if (name.empty()) {
        if (nullOnError) return new nullstream();
        ::p4c::error(ErrorType::ERR_INVALID, "Empty name for openFile");
        return nullptr;
    }
    std::ofstream *file = new std::ofstream(name);
    if (!file->good()) {
        ::p4c::error(ErrorType::ERR_IO, "Error writing output to file %1%: %2%", name, strerror(errno));
        if (nullOnError) return new nullstream();
        return nullptr;
    }
    return file;
}

}  // namespace p4c
