/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control noargs();

control p() {
    apply {}
}

control q() {
    p() p1;
    
    apply {    
        p1.apply();
    }
}

package m(noargs n);

m(q()) main;
