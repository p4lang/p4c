/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum bit<8> E {
   a = 5
}

bit<16> func(in bit<4> l)
{
  return 16w11 << E.a;
}