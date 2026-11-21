/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type intrinsic_metadata_t {
    fields {
        ingress_global_timestamp : 48;
        mcast_grp : 16;
        egress_rid : 16;
    }
}

metadata intrinsic_metadata_t intrinsic_metadata;
