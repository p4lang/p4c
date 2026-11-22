/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser p() {
    state start {
        {
            bit<16> retval = 0;
        }
        {
            bit<16> retval = 1;
        }
        transition accept;
    }
}
