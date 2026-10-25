/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control p()
{
    apply {
        bit<32> b;

        b = 32w0 & _; // does not typecheck
        b = 32w0 + _;
        b = true ? 32w0 : _;
    }
}
