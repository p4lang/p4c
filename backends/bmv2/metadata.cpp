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
// #include <lib/cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include "metadata.h"

namespace Arch {
/**
 * Reads a file that consists of pairs of names: arch meta -> target meta
 * ignores empty lines and lines starting with '#'
 * @param filename the file containing the remapping information
 * @returns the remap map
 *
 * \TODO: likely we'll want to use json to represent these maps since
 * there is other target specific information that we want to map
 */
MetadataRemapT *readMap(const char *filename) {
    std::ifstream input;
    input.open(filename);

    MetadataRemapT *remap = new MetadataRemapT;
    while (input.good()) {
        std::string a, sep, b;
        int c = input.peek();
        if (c == EOF) return remap;
        if (c == '#' || c == '\n') {
            // a comment line. read and discard
            input.ignore(256, '\n');
            continue;
        }
        input >> a >> sep >> b;
        if (a.length() > 0) {
            remap->emplace(cstring(a), cstring(b));
        }
    }
    input.close();
    return remap;
}

}  // end namespace Arch

namespace BMV2 {

}  // end namespace BMV2

