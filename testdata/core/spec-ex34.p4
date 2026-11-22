/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern Checksum16 
{
    // prepare unit
    void reset();
    // append data to be checksummed
    void append<D>(in D d); // same as { append(true, d); }
    // conditionally append data to be checksummed
    void append<D>(in bool condition, in D d);
    // get the checksum of all data appended since the last reset
    bit<16> get();
}
