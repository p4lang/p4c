/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser f() {
    state start {
        // implicit transition to reject: warning
    }
}

parser nothing();
package switch0(nothing _p);

switch0(f()) main;
