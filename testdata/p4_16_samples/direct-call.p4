/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
control c() {
    apply {}
}

control d() {
    apply { c.apply(); }
}
