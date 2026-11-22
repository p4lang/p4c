/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum bit<4> e
{
  a = 1,
  b = 2,
  c = 3
}
void f()
{
  bit<8> good1;
  good1 = e.a ++ e.b;
}

const bit<8> good = ((bit<4>)e.a) ++ ((bit<4>)e.b);
const bit<4> bad = e.a + e.b;
const bit<8> bad1 = e.a ++ e.b;
const bit<4> bad2 = e.a & e.b;
