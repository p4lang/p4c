/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser start {
  return select(current(0,1)) {
    default: loop;
  }
}

parser loop {
  return start;
}

control ingress {
}
