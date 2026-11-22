/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
action p()
{
    bit<32> x;
    x = 32w5 + 16w3;

    x = 5 / 0;
}
