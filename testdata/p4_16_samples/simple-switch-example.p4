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

#include "core.p4"

#include "simple_model.p4"

// This program processes packets composed of an Ethernet and 
// an IPv4 header, performing forwarding based on the
// destination IP address

// Header declarations

typedef bit<48> @ethernetaddress  EthernetAddress;
typedef bit<32> @ipv4address      IPv4Address;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

// IPv4 header without options
header IPv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

// Parser section

error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    IPv4ChecksumError
}

struct Parsed_packet {
    Ethernet_h ethernet;
    IPv4_h     ip;
}

parser TopParser(packet_in b, out Parsed_packet p)
{
    // ck is a variable representing a checksum16 unit from
    // the standard library
    Checksum16() ck;

    state start {
        b.extract(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x0800 : parse_ipv4;
        }
    }

    state parse_ipv4
    {
        b.extract(p.ip);
        verify(p.ip.version == 4w4, IPv4IncorrectVersion);
        verify(p.ip.ihl == 4w5, IPv4OptionsNotSupported);
        ck.clear();
        ck.update(p.ip);
        // Verify that packet checksum is zero
        verify(ck.get() == 16w0, IPv4ChecksumError);
        transition accept;
    }
}

// match-action pipeline section

control Pipe(
    inout Parsed_packet headers,
    in error parseError, // parser error
    in InControl inCtrl, // input port
    out OutControl outCtrl)
{
    /**
     * Indicates that a packet is dropped by setting the 
     * output port to the DROP_PORT
     */
    action Drop_action()
    { outCtrl.outputPort = DROP_PORT; }
    
    /**
     * Set the next hop and the output port
     * @param nextHop   ipv4 address of next nop
     * @param ivp4_dest ipv4 address of next hop (set by the control-plane)
     * @param port output port (set by the control-plane)
     */
    action Set_nhop(out IPv4Address nextHop,
                    IPv4Address ipv4_dest,
                    PortId_t port)
    {
        nextHop = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl - 8w1;
        outCtrl.outputPort = port;
    }
    
    /**
     * Computes address of next IPv4 hop and output port
     * based on the IPv4 destination of the current packet.
     * Decrements packet IPv4 TTL.
     * @param nextHop IPv4 address of next hop
     */
    table ipv4_match(out IPv4Address nextHop)
    {
        key = { headers.ip.dstAddr : lpm; }
        actions = {
            Set_nhop(nextHop);
            Drop_action;
        }

        size = 1024;
        default_action = Drop_action;
    }

    /**
     * Send the packet to the CPU port
     */
    action Send_to_cpu() {
        outCtrl.outputPort=CPU_OUT_PORT;
    }

    /**
     * Check packet TTL and send to CPU if expired.
     */
    table check_ttl()
    {
        key = { headers.ip.ttl : exact; }
        actions =
        {
            Send_to_cpu; NoAction;
        }

        const default_action = NoAction;
    }

    /**
     * Set the destination MAC address of the packet
     * @param dmac destination MAC address.
     */
    action Set_dmac(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    
    /**
     * Set the destination Ethernet address of the packet
     * based on the next hop IP address.
     * @param nextHop IPv4 address of next hop.
     */
    table dmac(in IPv4Address nextHop)
    {
        key = { nextHop : exact; }
        actions = {
            Set_dmac;
            Drop_action;
        }

        size = 1024;
        default_action = Drop_action;
    }

    /**
     * Set the source MAC address.
     * @param sourceMacw source MAC address to use
     */
    action Rewrite_smac(EthernetAddress sourceMac)
    {
        headers.ethernet.srcAddr = sourceMac;
    }
    
    /**
     * Set the source mac address based on the output port.
     */
    table smac()
    {
        key = { outCtrl.outputPort : exact; }
        actions = {
            Drop_action;
            Rewrite_smac;
        }

        size = 16;
        default_action = Drop_action;
    }

    apply {
        // match-action pipeline body

        if (parseError != NoError)
        {
            Drop_action();
            return;
        }

        IPv4Address nextHop; // temporary variable
        ipv4_match.apply(nextHop); // write result in nextHop
        if (outCtrl.outputPort == DROP_PORT)
            return;

        check_ttl.apply();
        if (outCtrl.outputPort == CPU_OUT_PORT)
            return;

        dmac.apply(nextHop);
        if (outCtrl.outputPort == DROP_PORT)
            return;

        smac.apply();
    }
} // end of pipeline

// deparser section
control TopDeparser(inout Parsed_packet p, packet_out b)
{
    // ck is a variable representing a checksum16 unit from
    // the standard library
    Checksum16() ck;

    apply {
        b.emit(p.ethernet);
        if (p.ip.isValid()) {
            ck.clear();       // prepare checksum unit
            p.ip.hdrChecksum = 16w0; // clear checksum
            ck.update(p.ip);  // compute new checksum.
            p.ip.hdrChecksum = ck.get();
            b.emit(p.ip);
        }
    }
}

Simple(TopParser(), Pipe(), TopDeparser()) main;

