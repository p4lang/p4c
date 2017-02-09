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

#ifndef _P4_P4_PARSE_H_
#define _P4_P4_PARSE_H_

#include <memory>
#include "ir/ir.h"

namespace IR { class Global; }

const IR::P4Program *parse_P4_16_file(const char *name, FILE *in);

#endif /* _P4_P4_PARSE_H_ */
