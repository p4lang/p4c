/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
action act() {
    bit<8> a;
    a = a / 0;
    a = a % 0;
    a = a / 8w0;
    a = a % 8w0;
}
