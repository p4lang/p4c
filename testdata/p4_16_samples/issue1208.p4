/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c();
package p(c _c);
package q(p _p);

control empty() {
    apply {}
}

q(p(empty())) main;
