/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control C<X>();
package S<X>(C<X> x1, C<X> x2);
control MyC()() { apply {} }
S<bool>(MyC(), MyC()) main;
