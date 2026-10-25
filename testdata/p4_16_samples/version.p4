/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct Version
{
    bit<8> major;
    bit<8> minor;
}
    
const .Version version = { 8w0, 8w1 };
