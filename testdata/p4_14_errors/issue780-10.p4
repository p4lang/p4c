/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control standard_metadata_t { }

parser start {
  return ingress1;
}
control ingress1 { }
