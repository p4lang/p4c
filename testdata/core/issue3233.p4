/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct s
{
  bit t;
  bit t1;
}

s func(bit t, bit t1)
{
  return (s)(s)(s){t, t1};
}

s func1(s t1)
{
  return (s)(s)(s)t1;
}
