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
    static const cstring defaultHit;
    static const cstring defaultHitConst;
    static const cstring tcType;
    static const cstring numMask;
    static const cstring tcMayOverride;
    static const cstring tc_md_write;
    static const cstring tc_md_read;
    static const cstring tc_md_exec;
    static const cstring tc_ContrlPath;
    static const cstring tc_key;
    static const cstring tc_data;
    static const cstring tc_data_scalar;
    static const cstring tc_init_val;
    static const cstring tc_numel;
    static const cstring tc_acl;
    ParseTCAnnotations()
        : P4::ParseAnnotations(
              "TC", true,
              {PARSE_EMPTY(defaultHit), PARSE_EMPTY(defaultHitConst),
               PARSE_CONSTANT_OR_STRING_LITERAL(tcType), PARSE_CONSTANT_OR_STRING_LITERAL(numMask),
               PARSE_EMPTY(tcMayOverride), PARSE_CONSTANT_OR_STRING_LITERAL(tc_acl),
               PARSE_EMPTY(tc_md_write), PARSE_EMPTY(tc_md_read), PARSE_EMPTY(tc_md_write),
               PARSE_EMPTY(tc_md_exec), PARSE_EMPTY(tc_ContrlPath), PARSE_EMPTY(tc_key),
               PARSE_EMPTY(tc_data), PARSE_EMPTY(tc_data_scalar), PARSE_EMPTY(tc_init_val),
               PARSE_EMPTY(tc_numel)}) {}
};

}  // namespace TC

#endif /* BACKENDS_TC_TCANNOTATIONS_H_ */
