/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Mirror processing
 */

action set_mirror_nhop(nhop_idx) {
    modify_field(l3_metadata.nexthop_index, nhop_idx);
}

action set_mirror_bd(bd) {
    modify_field(egress_metadata.bd, bd);
}

table mirror {
    reads {
        i2e_metadata.mirror_session_id : exact;
    }
    actions {
        nop;
        set_mirror_nhop;
        set_mirror_bd;
#ifdef SFLOW_ENABLE
        sflow_pkt_to_cpu;
#endif
    }
    size : MIRROR_SESSIONS_TABLE_SIZE;
}
