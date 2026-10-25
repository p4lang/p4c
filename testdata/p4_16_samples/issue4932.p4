/*
 * Copyright 2024 Intel Corporation
 * SPDX-FileCopyrightText: 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void log(string msg);

parser SimpleParser();
package SimpleArch(SimpleParser p);

parser ParserImpl()
{
    state start {
        log("Log message" ++ " text");
        transition accept;
    }
}

SimpleArch(ParserImpl()) main;
