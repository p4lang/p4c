/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
header h {}
struct s {}

control p()
{
    apply {
        h[5] stack;
        s[5] stack1; // 'normal' array can't use stack operations

        // out of range indexes
        h b = stack[1231092310293];
        h c = stack[-2];
        h d = stack[6];
        s e = stack1.next;
    }
}
