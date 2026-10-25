/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// incorrect arguments passed to constructor
extern E {
    E(bit x);
}

control c() {
    E(true) e;
    apply {}
}
