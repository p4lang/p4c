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

#ifndef _p4_parse_h_
#define _p4_parse_h_

#include <memory>
#include "lib/source_file.h"
#include "frontends/common/options.h"

namespace IR { class V1Program; }

const IR::V1Program *parse_p4v1_file(const CompilerOptions &, FILE *in);

#endif /* _p4_parse_h_ */
