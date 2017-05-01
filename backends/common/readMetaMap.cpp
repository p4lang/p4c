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
#include <map>
#include <string>

typedef std::map<std::string, std::string> metadataRemapT;

metadataRemapT *readMap(const char *filename) {
    std::ifstream input;
    input.open(filename);

    metadataRemapT *remap = new metadataRemapT;
    while(input.good()) {
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
            std::cout << "from " << a << " to " << b << std::endl;
            remap->emplace(a,b);
        }
    }
    input.close();
    return remap;
}

#if DEBUG
int main(int argc, char **argv) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <map_file>" << std::endl;
        exit(1);
    }
    metadataRemapT *r = readMap(argv[1]);
    assert(r);
    std::cout << "remapping " << r->size() << " metadata fields" << std::endl;
    delete r;
    return 0;
}
#endif
