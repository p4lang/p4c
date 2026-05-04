/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header A_h {}
header B_h {}

struct Alternate {
    A_h a0;
    B_h b;
    A_h a1;
}
