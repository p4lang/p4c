/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header h<t> {
  t f;
}
extern void e<t>(in h<t> p);
void func<t>(t a)
{
  h<t> v;
  e(v);
}