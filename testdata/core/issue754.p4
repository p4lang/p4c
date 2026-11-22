/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(out bit<3> x)(bit<3> v) { apply { x = v; } }

control ctrl(out bit<3> _x);
package top(ctrl c);

top(c(12345)) main;
