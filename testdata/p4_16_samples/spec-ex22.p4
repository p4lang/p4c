/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c()
{
    action Forward_a(out bit<9> outputPort, bit<9> port)
    {
        outputPort = port;
    }

    apply {}
}
