/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern X { X(); bit<8> get(); }

control q() {
    X() x;
    apply {}
}

control r() {
    apply {
        bit<8> y = .q.x.get(); // should be unreachable
    }
}
