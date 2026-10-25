/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern X {
    X();
}

control p()
{
    X() x;

    apply {
        x.f();
    }
}
