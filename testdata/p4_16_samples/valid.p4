/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header h {
    bit<32> f;
}

control c(inout h hdr)
{
    action a() {
        hdr.setInvalid();
    }

    apply {}
}
