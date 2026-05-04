/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern Crc16 <T> {
    void hash<U>(in U input_data);
    U id<U>(in U x);
}
