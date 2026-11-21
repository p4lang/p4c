/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser p() {
    state start {
        transition accept;
    }
}

void func()
{
    p() t;
    t();
}