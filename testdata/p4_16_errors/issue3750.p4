/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control ct();
void f(tuple<ct> t)
{
  t[0].apply();
}