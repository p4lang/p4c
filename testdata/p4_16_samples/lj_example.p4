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

#include <core.p4>
#include "very_simple_model.p4"

header ARPA_hdr {
    bit<48> src;
    bit<48> dest;
    bit<16> etype;
}

struct Parsed_rep {
    ARPA_hdr arpa_pak;
}

parser LJparse(packet_in b, out Parsed_rep p) {
    state start {
        b.extract(p.arpa_pak);
        transition accept;
    }
}

control LjPipe(inout Parsed_rep p,
        in error parseError,
        in InControl inCtrl,
        out OutControl outCtrl)
{
    action Drop_action(out PortId port)
    {
        port = DROP_PORT;
    }

    action Drop_1 ()
    {
       outCtrl.outputPort = DROP_PORT;
    }

    action Forward(PortId outPort)
    {
       outCtrl.outputPort = outPort;
    }

    table Enet_lkup {
        key = { p.arpa_pak.dest : exact; }

        actions = {
            Drop_action(outCtrl.outputPort);
            Drop_1;
            Forward;
        }

        default_action = Drop_1;
    }

    apply {
        outCtrl.outputPort = DROP_PORT;
        if (p.arpa_pak.isValid())
            Enet_lkup.apply();
    }
}

control LJdeparse(inout Parsed_rep p, packet_out b)
{
    apply {
        b.emit<ARPA_hdr>(p.arpa_pak);
    }
}

VSS(LJparse(), LjPipe(), LJdeparse()) main;
