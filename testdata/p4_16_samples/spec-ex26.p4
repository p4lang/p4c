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

#include "very_simple_model.p4"

struct Parsed_packet {}

typedef bit<32>  IPv4Address;

extern tbl {}

control Pipe(inout Parsed_packet headers,
             in error parseError, // parser error
             in InControl inCtrl, // input port
             out OutControl outCtrl)
{
    action Drop_action(out PortId_t port)
    {
        port = DROP_PORT;
    }

    action drop() {}

    table IPv4_match(in IPv4Address a)
    {
        actions = {
            drop;
        }
        key = { inCtrl.inputPort : exact; }
        implementation = tbl();
        default_action = drop;
    }

    apply {
        if (parseError != NoError)
        {
            // invoke Drop_action directly
            Drop_action(outCtrl.outputPort);
            return;
        }

        IPv4Address nextHop; // temporary variable
        if (!IPv4_match.apply(nextHop).hit)
            return;
    }
}
