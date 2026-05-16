/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser p() {
    state next {}
}  // no start state

parser nothing();
package top(nothing _n);
top(p()) main;
