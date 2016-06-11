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

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type llc_header_t {
    fields {
        dsap : 8;
        ssap : 8;
        control_ : 8;
    }
}

header_type snap_header_t {
    fields {
        oui : 24;
        type_ : 16;
    }
}

header_type roce_header_t {
    fields {
        ib_grh : 320;
        ib_bth : 96;
    }
}

header_type roce_v2_header_t {
    fields {
        ib_bth : 96;
    }
}

header_type fcoe_header_t {
    fields {
        version : 4;
        type_ : 4;
        sof : 8;
        rsvd1 : 32;
        ts_upper : 32;
        ts_lower : 32;
        size_ : 32;
        eof : 8;
        rsvd2 : 24;
    }
}

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
        etherType : 16;
    }
}

header_type ieee802_1ah_t {
    fields {
        pcp : 3;
        dei : 1;
        uca : 1;
        reserved : 3;
        i_sid : 24;
    }
}

header_type mpls_t {
    fields {
        label : 20;
        exp : 3;
        bos : 1;
        ttl : 8;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type ipv6_t {
    fields {
        version : 4;
        trafficClass : 8;
        flowLabel : 20;
        payloadLen : 16;
        nextHdr : 8;
        hopLimit : 8;
        srcAddr : 128;
        dstAddr : 128;
    }
}

header_type icmp_t {
    fields {
        typeCode : 16;
        hdrChecksum : 16;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

header_type sctp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        verifTag : 32;
        checksum : 32;
    }
}

header_type gre_t {
    fields {
        C : 1;
        R : 1;
        K : 1;
        S : 1;
        s : 1;
        recurse : 3;
        flags : 5;
        ver : 3;
        proto : 16;
    }
}

header_type nvgre_t {
    fields {
        tni : 24;
        flow_id : 8;
    }
}

/* erspan III header - 12 bytes */
header_type erspan_header_t3_t {
    fields {
        version : 4;
        vlan : 12;
        priority : 6;
        span_id : 10;
        timestamp : 32;
        sgt_other : 32;
    }
}

header_type ipsec_esp_t {
    fields {
        spi : 32;
        seqNo : 32;
    }
}

header_type ipsec_ah_t {
    fields {
        nextHdr : 8;
        length_ : 8;
        zero : 16;
        spi : 32;
        seqNo : 32;
    }
}

header_type arp_rarp_t {
    fields {
        hwType : 16;
        protoType : 16;
        hwAddrLen : 8;
        protoAddrLen : 8;
        opcode : 16;
    }
}

header_type arp_rarp_ipv4_t {
    fields {
        srcHwAddr : 48;
        srcProtoAddr : 32;
        dstHwAddr : 48;
        dstProtoAddr : 32;
    }
}

header_type eompls_t {
    fields {
        zero : 4;
        reserved : 12;
        seqNo : 16;
    }
}

header_type vxlan_t {
    fields {
        flags : 8;
        reserved : 24;
        vni : 24;
        reserved2 : 8;
    }
}

header_type vxlan_gpe_t {
    fields {
        flags : 8;
        reserved : 16;
        next_proto : 8;
        vni : 24;
        reserved2 : 8;
    }
}

header_type nsh_t {
    fields {
        oam : 1;
        context : 1;
        flags : 6;
        reserved : 8;
        protoType: 16;
        spath : 24;
        sindex : 8;
    }
}

header_type nsh_context_t {
    fields {
        network_platform : 32;
        network_shared : 32;
        service_platform : 32;
        service_shared : 32;
    }
}

header_type vxlan_gpe_int_header_t {
    fields {
        int_type    : 8;
        rsvd        : 8;
        len         : 8;
        next_proto  : 8;
    }
}

/* GENEVE HEADERS
   3 possible options with known type, known length */

header_type genv_t {
    fields {
        ver : 2;
        optLen : 6;
        oam : 1;
        critical : 1;
        reserved : 6;
        protoType : 16;
        vni : 24;
        reserved2 : 8;
    }
}

#define GENV_OPTION_A_TYPE 0x000001
/* TODO: Would it be convenient to have some kind of sizeof macro ? */
#define GENV_OPTION_A_LENGTH 2 /* in bytes */

header_type genv_opt_A_t {
    fields {
        optClass : 16;
        optType : 8;
        reserved : 3;
        optLen : 5;
        data : 32;
    }
}

#define GENV_OPTION_B_TYPE 0x000002
#define GENV_OPTION_B_LENGTH 3 /* in bytes */

header_type genv_opt_B_t {
    fields {
        optClass : 16;
        optType : 8;
        reserved : 3;
        optLen : 5;
        data : 64;
    }
}

#define GENV_OPTION_C_TYPE 0x000003
#define GENV_OPTION_C_LENGTH 2 /* in bytes */

header_type genv_opt_C_t {
    fields {
        optClass : 16;
        optType : 8;
        reserved : 3;
        optLen : 5;
        data : 32;
    }
}

header_type trill_t {
    fields {
        version : 2;
        reserved : 2;
        multiDestination : 1;
        optLength : 5;
        hopCount : 6;
        egressRbridge : 16;
        ingressRbridge : 16;
    }
}

header_type lisp_t {
    fields {
        flags : 8;
        nonce : 24;
        lsbsInstanceId : 32;
    }
}

header_type vntag_t {
    fields {
        direction : 1;
        pointer : 1;
        destVif : 14;
        looped : 1;
        reserved : 1;
        version : 2;
        srcVif : 12;
    }
}

header_type bfd_t {
    fields {
        version : 3;
        diag : 5;
        state : 2;
        p : 1;
        f : 1;
        c : 1;
        a : 1;
        d : 1;
        m : 1;
        detectMult : 8;
        len : 8;
        myDiscriminator : 32;
        yourDiscriminator : 32;
        desiredMinTxInterval : 32;
        requiredMinRxInterval : 32;
        requiredMinEchoRxInterval : 32;
    }
}

header_type sflow_t {
    fields {
        version : 32;
        ipVersion : 32;
        ipAddress : 32;
        subAgentId : 32;
        seqNumber : 32;
        uptime : 32;
        numSamples : 32;
    }
}

header_type sflow_internal_ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type sflow_sample_t {
    fields {
        enterprise : 20;
        format : 12;
        sampleLength : 32;
        seqNumer : 32;
        srcIdClass : 8;
        srcIdIndex : 24;
        samplingRate : 32;
        samplePool : 32;
        numDrops : 32;
        inputIfindex : 32;
        outputIfindex : 32;
        numFlowRecords : 32;
    }
}

header_type sflow_record_t {
    fields {
        enterprise : 20;
        format : 12;
        flowDataLength : 32;
        headerProtocol : 32;
        frameLength : 32;
        bytesRemoved : 32;
        headerSize : 32;
    }
}

#define FABRIC_HEADER_TYPE_NONE        0
#define FABRIC_HEADER_TYPE_UNICAST     1
#define FABRIC_HEADER_TYPE_MULTICAST   2
#define FABRIC_HEADER_TYPE_MIRROR      3
#define FABRIC_HEADER_TYPE_CONTROL     4
#define FABRIC_HEADER_TYPE_CPU         5

header_type fabric_header_t {
    fields {
        packetType : 3;
        headerVersion : 2;
        packetVersion : 2;
        pad1 : 1;

        fabricColor : 3;
        fabricQos : 5;

        dstDevice : 8;
        dstPortOrGroup : 16;
    }
}

header_type fabric_header_unicast_t {
    fields {
        routed : 1;
        outerRouted : 1;
        tunnelTerminate : 1;
        ingressTunnelType : 5;

        nexthopIndex : 16;
    }
}

header_type fabric_header_multicast_t {
    fields {
        routed : 1;
        outerRouted : 1;
        tunnelTerminate : 1;
        ingressTunnelType : 5;

        ingressIfindex : 16;
        ingressBd : 16;

        mcastGrp : 16;
    }
}

header_type fabric_header_mirror_t {
    fields {
        rewriteIndex : 16;
        egressPort : 10;
        egressQueue : 5;
        pad : 1;
    }
}

header_type fabric_header_cpu_t {
    fields {
        egressQueue : 5;
        txBypass : 1;
        reserved : 2;

        ingressPort: 16;
        ingressIfindex : 16;
        ingressBd : 16;

        reasonCode : 16;
    }
}

header_type fabric_payload_header_t {
    fields {
        etherType : 16;
    }
}

/* INT headers */
header_type int_header_t {
    fields {
        ver                     : 2;
        rep                     : 2;
        c                       : 1;
        e                       : 1;
        rsvd1                   : 5;
        ins_cnt                 : 5;
        max_hop_cnt             : 8;
        total_hop_cnt           : 8;
        instruction_mask_0003   : 4;   /* split the bits for lookup */
        instruction_mask_0407   : 4;
        instruction_mask_0811   : 4;
        instruction_mask_1215   : 4;
        rsvd2                   : 16;
    }
}

/* INT meta-value headers - different header for each value type */
header_type int_switch_id_header_t {
    fields {
        bos                 : 1;
        switch_id           : 31;
    }
}
header_type int_ingress_port_id_header_t {
    fields {
        bos                 : 1;
        ingress_port_id     : 31;
    }
}
header_type int_hop_latency_header_t {
    fields {
        bos                 : 1;
        hop_latency         : 31;
    }
}
header_type int_q_occupancy_header_t {
    fields {
        bos                 : 1;
        q_occupancy         : 31;
    }
}
header_type int_ingress_tstamp_header_t {
    fields {
        bos                 : 1;
        ingress_tstamp      : 31;
    }
}
header_type int_egress_port_id_header_t {
    fields {
        bos                 : 1;
        egress_port_id      : 31;
    }
}
header_type int_q_congestion_header_t {
    fields {
        bos                 : 1;
        q_congestion        : 31;
    }
}
header_type int_egress_port_tx_utilization_header_t {
    fields {
        bos                         : 1;
        egress_port_tx_utilization  : 31;
    }
}

/* generic int value (info) header for extraction */
header_type int_value_t {
    fields {
        bos         : 1;
        val         : 31;
    }
}
