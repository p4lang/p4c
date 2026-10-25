/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bs {}

parser p(in bs b, out bool matches)
{
    state start
    {
        transition next;
    }
    
    state next 
    {
        transition accept;
    }
}