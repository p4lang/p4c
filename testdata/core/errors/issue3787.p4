/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern X<T> {}
extern Y {}
control c1(in Y x);
control c2(in X<bit> x);
