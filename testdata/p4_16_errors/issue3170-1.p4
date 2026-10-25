/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header yy {
    bit tt;
}
struct y {
    yy[2] tt;
}
header t<tt> {
    tt y;
}

typedef t<y> tty;
