/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
struct S { }

control p()
{
    S() s;   // structs have no constructors

    apply{}
}
