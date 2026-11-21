/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p00()
{
    state start {
        bit<1> z = (1w0 & 1w1);
        transition accept;
    }
}

parser nothing();
package m(nothing n);

m(p00()) main;
