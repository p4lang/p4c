/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header h {
    bit field;
};

struct s {
    h[2] field;
};

action a(in s v) {
    bit<32> t = v.field.lastIndex;
}