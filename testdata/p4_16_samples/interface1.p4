/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

extern X<T> { X(); }
extern Y    { Y(); }

parser p()
{
    X<int<32>>() x;
    Y()          y;

    state start { transition accept; }
}

parser empty();
package sw(empty e);

sw(p()) main;
