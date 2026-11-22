/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type X {
    fields {
        x: 32;
    }
}

header X standard_metadata;

parser start {
    return ingress;
}

control ingress {}
