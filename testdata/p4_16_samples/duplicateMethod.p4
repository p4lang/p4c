/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
struct packet_in {}

extern Checksum<T>
{
    void reset();
    void append<D>(in D d);
    void append<D>(in bool condition, in D d);
    void append(in packet_in b);   // duplicate method
    T get();
}
