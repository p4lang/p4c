/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

const bit<8> x = 10;
struct S { bit<8> s; }
action a(in S w, out bit<8> z) 
{
    z = x + w.s;
}
