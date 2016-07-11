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

#ifndef _FRONTENDS_COMMON_PREPROCESSOR_H_
#define _FRONTENDS_COMMON_PREPROCESSOR_H_

#include <stdio.h>
#include <unordered_set>
#include <vector>

class cstring;

// Preprocess a P4 source file using the native P4-16 preprocessor
cstring preprocessP4File(FILE* input, cstring filename,
                         const std::unordered_set<cstring>& definitions,
                         const std::vector<cstring>& importPaths);

#endif /* _FRONTENDS_COMMON_PREPROCESSOR_H_ */
