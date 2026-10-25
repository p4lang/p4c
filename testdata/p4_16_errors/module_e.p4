/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
parser Filter(out bool filter);

package top(Filter f);

parser g(in bit x) // mismatch in direction
{
    state start { transition accept; }
}

top(g()) main;
