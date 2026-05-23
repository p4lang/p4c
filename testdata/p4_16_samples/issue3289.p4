/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

bit<5> f1(in int<3> a)
{
  return (5w1) << 5;
}

bit<5> f2(in int<3> a)
{
  return (5w1) << 6;
}