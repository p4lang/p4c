/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void log(string s);

control c() {
    apply {
        log("This is a message");
    }
}

control e();
package top(e _e);

top(c()) main;
