/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern X<T> { }

parser p()
{
    // no type arguments
    X() x;

    state start { transition accept; }
}
