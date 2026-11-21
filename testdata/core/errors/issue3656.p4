/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action a() {}
control c() {
    apply {
        a<bit>();
    }
}
