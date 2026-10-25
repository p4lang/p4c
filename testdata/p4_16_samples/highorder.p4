/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser parse<H>(out H headers);

package ebpfFilter<H>(parse<H> prs);

struct Headers_t {}

parser prs(out Headers_t headers) {
    state start { transition accept; }
}

ebpfFilter(prs()) main;
