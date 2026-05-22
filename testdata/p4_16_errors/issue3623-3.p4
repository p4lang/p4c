/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum bit<4> e
{
  a = 1
}
control c(in e v)
{
  apply{
    switch(v) {
      1:
      default: {}
    }
  }
}