/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

match_kind { exact }

typedef bit<48> EthernetAddress;

extern tbl { tbl(); }

control c(bit x)
{
    action Set_dmac(EthernetAddress dmac)
    {}

    action drop() {}

    table unit {
        key = { x : exact; }

#if 0
        entries = {
            32w0x0A_00_00_01 => drop();
            32w0x0A_00_00_02 => Set_dmac(dmac = (EthernetAddress)48w0x11_22_33_44_55_66);
            32w0x0B_00_00_03 => Set_dmac(dmac = (EthernetAddress)48w0x11_22_33_44_55_77);
            32w0x0B_00_00_00 &&& 32w0xFF_00_00_00 => drop();
        }
#endif

        actions = {
            Set_dmac;
            drop;
        }

        default_action = Set_dmac((EthernetAddress)48w0xAA_BB_CC_DD_EE_FF);

        implementation = tbl();
    }

    apply {}
}
