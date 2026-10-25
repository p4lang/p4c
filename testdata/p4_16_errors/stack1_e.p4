/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
header h {}

control p()
{
    apply {
        int<3> w = 5;
        h[w] stack;
    }
}
