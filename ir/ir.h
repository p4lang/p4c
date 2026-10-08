/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_IR_H_
#define IR_IR_H_

// Compatibility umbrella for all enabled IR extensions. Core compiler code
// should include ir/core.h to avoid dependencies on backend class definitions.
#include "ir/ir-generated.h"  // IWYU pragma: export
#include "ir/ir-inline.h"     // IWYU pragma: export

#endif /* IR_IR_H_ */
