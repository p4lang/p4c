/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser start {
    return ingress;
}

table empty {
    actions {}
}

control ingress {
    apply(empty);
}