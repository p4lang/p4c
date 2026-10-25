/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action NoAction(in bit t) {}
control c() {
    table t {
        actions = {}
    }
    apply {}
}