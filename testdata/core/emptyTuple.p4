/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef tuple<> emptyTuple;

control c(out bool b) {
    apply {
        emptyTuple t = {};
        if (t == {})
           b = true;
        else
           b = false;
    }
}

control e(out bool b);
package top(e _e);

top(c()) main;
