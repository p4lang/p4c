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

#include <string>

/* preprocessing by prepending the content of core.p4 to test program */
std::string with_core_p4 (const std::string& pgm) {
    std::ifstream input("p4include/core.p4");
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    return sstr.str() + pgm;
}
