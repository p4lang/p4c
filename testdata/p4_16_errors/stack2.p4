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
        h[5] stack1;
        h[3] stack2;

        stack1 = stack2;  // assignment between different stacks
        stack1.lastIndex = 3;  // not an l-value
        stack2.size = 4;  // not an l-value
    }
}
