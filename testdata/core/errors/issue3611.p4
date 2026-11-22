/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action NoAction() {}
action a() {}
action b() {}
action d() {}
control c()
{
  table t {
    actions = {a; b;}
  }
  apply {
   switch(t.apply().action_run) {
      1: {} // { dg-error "" }
      a: {}
      default: {}
    }
  }
}
