/*
 * SPDX-FileCopyrightText: 2019 Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern T max<T>(T t1, T t2);

extern Register<W> {
    void write(in W max);
}
