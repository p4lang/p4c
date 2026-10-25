/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control p()
{
    apply {
        int<32> a = 32s1;
        int<32> b = a > > 1;
    }
}
