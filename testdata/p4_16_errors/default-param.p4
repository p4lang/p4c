/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f(out bit<32> x = 0,   // illegal: out parameter with default value
             @optional bit<32> y = 0); // illegal: optional parameter with default value
