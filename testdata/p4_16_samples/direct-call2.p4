/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser p() {
    state start {
        transition accept;
    }
}

parser q() {
    state start {
        p.apply();
        p.apply();
        transition accept;
    }
}
