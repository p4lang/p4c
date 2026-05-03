/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser p<T>(in T i);

package m1<D>(p<D> m);
package m2<D>(p<_> m);
