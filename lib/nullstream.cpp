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
