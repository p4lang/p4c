/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header h{}

bool f()
{
  return ((h){}).isValid() || ((h){#}).isValid();
}

control c(out bool b) {
    apply {
        b = f();
    }
}

control C(out bool b);
package top(C _c);

top(c()) main;
