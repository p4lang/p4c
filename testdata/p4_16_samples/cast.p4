/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control p()
{
    apply {
        bit<2>  a;
        int<2>  b;
    
        a = (bit<2>)b;
        b = (int<2>)a;
    }
}