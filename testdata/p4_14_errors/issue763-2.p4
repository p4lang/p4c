/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type standard_metadata_t {
    fields {
        x: 32;
    }
}

parser start {
    return ingress;
}

control ingress {}
