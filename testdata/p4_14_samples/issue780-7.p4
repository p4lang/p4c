/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type exact {
  fields {
    counterrevolution : 32;
  }
}
header exact heartlands;
parser start {
  return ingress;
}
action add_heartlands() {
  add_header(heartlands);
}
control ingress { }
