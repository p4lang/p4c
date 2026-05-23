/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

@deprecated("Please use verify_checksum/update_checksum instead.")
extern Checksum16 {
    Checksum16();
}

@deprecated("Please don't use this function.")
extern bit<6> wrong();

Checksum16() instance;

control c() {
    apply {
        bit<6> x = wrong();
    }
}
