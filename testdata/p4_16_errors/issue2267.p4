/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

control MyC(bit<8> t) {
  table t {
    key = { t : exact; }
    actions = {}
  }
  apply {}
}
