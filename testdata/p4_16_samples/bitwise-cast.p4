/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core.p4"

control p() {
    apply {
        bit<1> a;
        bit<8> b = 3;
        bit<8> c = 4;
        a = (bit)(b == 1) & (bit)(c == 2);
    }
}
