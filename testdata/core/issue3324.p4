/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void f<t>(in t a){ }
void g<t>(in t b)
{
  f(b);
}