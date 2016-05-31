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

#ifndef _FRONTENDS_COMMON_PARSEINPUT_H_
#define _FRONTENDS_COMMON_PARSEINPUT_H_

#include "ir/ir.h"
#include "options.h"

// Parse a P4-14 or P4-16 file (as specified by the options)
// and return it in a P4-16 representation
const IR::P4Program* parseP4File(CompilerOptions& options);

#endif /* _FRONTENDS_COMMON_PARSEINPUT_H_ */
