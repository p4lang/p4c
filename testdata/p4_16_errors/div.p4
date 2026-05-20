/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
action act() {
    bit<8> a;
    a = - 8 / -2;
    a = -10 % 2;
    a = 10 % -2;
}
