/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser p() {
    state start {
        transition select(32w0) {
            5 : reject;
            default : reject;
        }
    }
}
