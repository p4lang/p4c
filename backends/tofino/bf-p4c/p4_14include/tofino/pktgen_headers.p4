/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/********************************************************************************
 *                   Packet Generator Header Definition for Tofino              *
 *******************************************************************************/

#ifndef TOFINO_LIB_PKTGEN_HEADERS
#define TOFINO_LIB_PKTGEN_HEADERS 1

header_type pktgen_generic_header_t {
    fields {
        _pad0     :  3;
        pipe_id   :  2;
        app_id    :  3;
        key_msb   :  8; // Only valid for recirc triggers.
        batch_id  : 16; // Overloaded to port# or lsbs of key for port down and
                        // recirc triggers.
        packet_id : 16;
    }
}
header pktgen_generic_header_t pktgen_generic;

header_type pktgen_timer_header_t {
    fields {
        _pad0     :  3;
        pipe_id   :  2;
        app_id    :  3;
        _pad1     :  8;
        batch_id  : 16;
        packet_id : 16;
    }
}
header pktgen_timer_header_t pktgen_timer;

header_type pktgen_port_down_header_t {
    fields {
        _pad0     :  3;
        pipe_id   :  2;
        app_id    :  3;
        _pad1     : 15;
        port_num  :  9;
        packet_id : 16;
    }
}
header pktgen_port_down_header_t pktgen_port_down;

header_type pktgen_recirc_header_t {
    fields {
        _pad0     :  3;
        pipe_id   :  2;
        app_id    :  3;
        key       : 24;
        packet_id : 16;
    }
}
header pktgen_recirc_header_t pktgen_recirc;

#endif
