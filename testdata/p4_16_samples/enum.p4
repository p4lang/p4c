/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum X
{
    Field,
    Field1,
    Field2
}

control c()
{
    apply {
        X v = X.Field;
    }
}

