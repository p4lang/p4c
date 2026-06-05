/*
 * Copyright 2018 Barefoot Networks, Inc.
 * SPDX-FileCopyrightText: 2018 Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

extern widget { }

extern widget createWidget<T, U>(U a, T b);

parser P();
parser p1()(widget w) {
    state start { transition accept; } }

package sw0(P p);

sw0(p1(createWidget(16w0, 8w0))) main;
