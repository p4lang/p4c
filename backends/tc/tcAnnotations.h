/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TC_TCANNOTATIONS_H_
#define BACKENDS_TC_TCANNOTATIONS_H_

#include "frontends/p4/parseAnnotations.h"
#include "ir/ir.h"

namespace P4::TC {

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
    static const cstring tc_ControlPath;
    static const cstring tc_key;
    static const cstring tc_data;
    static const cstring tc_data_scalar;
    static const cstring tc_init_val;
    static const cstring tc_numel;
    static const cstring tc_acl;
    ParseTCAnnotations()
        : P4::ParseAnnotations(
              "TC", true,
              {PARSE_EMPTY(defaultHit), PARSE_EMPTY(defaultHitConst), PARSE_STRING_LITERAL(tcType),
               PARSE_CONSTANT_OR_STRING_LITERAL(numMask), PARSE_EMPTY(tcMayOverride),
               PARSE_STRING_LITERAL(tc_acl), PARSE_EMPTY(tc_md_write), PARSE_EMPTY(tc_md_read),
               PARSE_EMPTY(tc_md_write), PARSE_EMPTY(tc_md_exec), PARSE_EMPTY(tc_ControlPath),
               PARSE_EMPTY(tc_key), PARSE_EMPTY(tc_data), PARSE_EMPTY(tc_data_scalar),
               PARSE_EMPTY(tc_init_val), PARSE_EMPTY(tc_numel)}) {}
};

}  // namespace P4::TC

#endif /* BACKENDS_TC_TCANNOTATIONS_H_ */
