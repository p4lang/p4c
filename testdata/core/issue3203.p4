/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct h<t> {
    t f;
}

bit func(h<bit> a) {
    return a.f;
}
