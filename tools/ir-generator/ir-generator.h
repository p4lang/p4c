/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TOOLS_IR_GENERATOR_IR_GENERATOR_H_
#define TOOLS_IR_GENERATOR_IR_GENERATOR_H_

#include "irclass.h"

namespace P4 {

IrDefinitions *parse(char **files, int count);

}  // namespace P4

#endif /* TOOLS_IR_GENERATOR_IR_GENERATOR_H_ */
