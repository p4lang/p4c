/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type egress {
    fields {
        counterrevolution : 32;
    }
}
header egress heartlands;
parser start {
    return ingress;
}
action add_heartlands() {
    add_header(heartlands);
}
control ingress { }
