/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_TCANNOTATIONS_H_
#define BACKENDS_TC_TCANNOTATIONS_H_

#include "frontends/p4/parseAnnotations.h"
#include "ir/ir.h"

namespace TC {

class ParseTCAnnotations : public P4::ParseAnnotations {
 public:
    // TC specific annotations
    static const cstring default_hit;
    static const cstring default_hit_const;
    static const cstring tcType;
    static const cstring numMask;
    ParseTCAnnotations()
        : P4::ParseAnnotations("TC", true,
                               {PARSE_EMPTY(default_hit), PARSE_EMPTY(default_hit_const),
                                PARSE_CONSTANT_OR_STRING_LITERAL(tcType),
                                PARSE_CONSTANT_OR_STRING_LITERAL(numMask)}) {}
};

}  // namespace TC

#endif /* BACKENDS_TC_TCANNOTATIONS_H_ */
