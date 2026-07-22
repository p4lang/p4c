/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
action a() {
    bit<8> a = 8w0;
    int<8> b = 8w0;
    a = a - b;
}
