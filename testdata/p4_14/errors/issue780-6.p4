/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type standard_metadata {
    fields {
        counterrevolution : 32;
    }
}
header standard_metadata heartlands;
parser start {
    return ingress;
}
action add_heartlands() {
    add_header(heartlands);
}
control ingress { }
