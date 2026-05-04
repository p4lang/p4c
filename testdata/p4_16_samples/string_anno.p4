/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* P4TEST_IGNORE_STDERR */

@name("original") const bit b = 1;
@name("string \" with \" quotes") const bit c = 1;
@name("string with
newline") const bit d = 1;
@name("string with quoted \
newline") const bit e = 1;
@name("8-bit string ⟶") const bit f = 1;
