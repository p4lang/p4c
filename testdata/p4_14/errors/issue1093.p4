/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser start {
  return ingress;
}
control ingress {
  d();
}
control d {
    if(0 == 1) { }
