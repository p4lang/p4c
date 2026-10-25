/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p()(bit b, bit c)
{
   state start {
        bit z = b & c;
        transition accept;
   }
}

const bit bv = 1w0;

parser nothing();

package m(nothing n);

m(p(bv, 1w1)) main;
