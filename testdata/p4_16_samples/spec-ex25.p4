/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

match_kind { exact }

typedef bit<48> EthernetAddress;

extern tbl { tbl(); }

control c(bit<32> x)
{
    action Set_dmac(EthernetAddress dmac)
    {}

    action drop() {}

    table unit {
        key = { x : exact; }
        actions = {
            Set_dmac;
            drop;
        }

        const entries = {
            32w0x0A_00_00_01 : drop();
            32w0x0A_00_00_02 : Set_dmac(dmac = (EthernetAddress)48w0x11_22_33_44_55_66);
            32w0x0B_00_00_03 : Set_dmac(dmac = (EthernetAddress)48w0x11_22_33_44_55_77);
            32w0x0B_00_00_00 &&& 32w0xFF_00_00_00 : drop();
        }

        default_action = Set_dmac((EthernetAddress)48w0xAA_BB_CC_DD_EE_FF);

        implementation = tbl();
    }

    apply {}
}
