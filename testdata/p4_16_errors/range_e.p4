/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
parser p()
{
    state start {
        transition select (2w0) {
            2w2 .. 3w3 : reject;  // different widths
        }
    }
}
