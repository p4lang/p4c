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

#ifndef _SIMPLE_MODEL_P4_
#define _SIMPLE_MODEL_P4_

#include "core.p4"
extern Checksum16 
{
    // prepare unit
    void clear();
    // add data to be checksummed
    void update<D>(in D dt);
    // conditionally add data to be checksummed
    void update<D>(in bool condition, in D dt);
    // get the checksum of all data added since the last clear
    bit<16> get();
}

/* Various constants and structure definitions */
/* ports are represented using 4-bit values */
typedef bit<4> PortId_t;
/* only 8 ports are “real” */
const PortId_t REAL_PORT_COUNT = (PortId_t)4w8;
/* metadata accompanying an input packet */
struct InControl {
    PortId_t inputPort;
}
    
/* special input port values */
const PortId_t RECIRCULATE_INPUT_PORT = (PortId_t)4w0xD;
const PortId_t CPU_INPUT_PORT = (PortId_t)4w0xE;
/* metadata that must be computed for outgoing packets */
struct OutControl {
    PortId_t outputPort;
}

/* special output port values for outgoing packet */
const PortId_t DROP_PORT = (PortId_t)4w0xF;
const PortId_t CPU_OUT_PORT = (PortId_t)4w0xE;
const PortId_t RECIRCULATE_OUT_PORT = (PortId_t)4w0xD;
    
/* List of blocks that must be implemented */
parser Parser<H>(packet_in b, 
                 out H parsedHeaders);
control MAP<H>(inout H headers,
               in error parseError, // parser error
               in InControl inCtrl, // input port
               out OutControl outCtrl); // output port
control Deparser<H>(inout H outputHeaders, 
                    packet_out b);
    
/** 
 * Simple switch declaration.
 * H is the user-defined type of the headers processed
 */
package Simple<H>(Parser<H> p, 
                  MAP<H> map, 
                  Deparser<H> d);

#endif
