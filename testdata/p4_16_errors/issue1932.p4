/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control foo (in bit<8> x, out bit<8> y) { apply { y = x + 7; } }
bool foo() { return true; }
