/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4TEST_P4TEST_H_
#define BACKENDS_P4TEST_P4TEST_H_

#include "frontends/common/options.h"

using namespace P4;

class P4TestOptions : public CompilerOptions {
 public:
    bool parseOnly = false;
    bool validateOnly = false;
    bool loadIRFromJson = false;
    bool preferSwitch = false;
    bool keepTuples = false;  // keep tuples but flatten assignments of them
    P4TestOptions();
};

#endif /* BACKENDS_P4TEST_P4TEST_H_ */
