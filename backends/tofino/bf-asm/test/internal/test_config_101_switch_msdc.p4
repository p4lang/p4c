# 1 "p4src/switch.p4"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "p4src/switch.p4"

# 1 "/home/john/ws/p4f/submodules/p4c-tofino/p4c_tofino/target/tofino/p4_lib/tofino/intrinsic_metadata.p4" 1
# 11 "/home/john/ws/p4f/submodules/p4c-tofino/p4c_tofino/target/tofino/p4_lib/tofino/intrinsic_metadata.p4"
header_type ingress_parser_control_signals {
    fields {
        priority : 3;
    }
}

@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress ingress ig_prsr_ctrl
header ingress_parser_control_signals ig_prsr_ctrl;



header_type ingress_intrinsic_metadata_t {
    fields {

        resubmit_flag : 1;


        _pad1 : 1;

        _pad2 : 2;

        _pad3 : 3;

        ingress_port : 9;


        ingress_mac_tstamp : 48;

    }
}

@pragma dont_trim
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress ingress ig_intr_md
@pragma pa_no_tagalong ingress ig_intr_md.ingress_port
header ingress_intrinsic_metadata_t ig_intr_md;



header_type generator_metadata_t {
    fields {

        app_id : 16;

        batch_id: 16;

        instance_id: 16;
    }
}

@pragma not_deparsed ingress
@pragma not_deparsed egress
header generator_metadata_t ig_pg_md;




header_type ingress_intrinsic_metadata_from_parser_aux_t {
    fields {
        ingress_global_tstamp : 48;


        ingress_global_ver : 32;


        ingress_parser_err : 16;

    }
}

@pragma pa_fragment ingress ig_intr_md_from_parser_aux.ingress_parser_err
@pragma pa_atomic ingress ig_intr_md_from_parser_aux.ingress_parser_err
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress ingress ig_intr_md_from_parser_aux
header ingress_intrinsic_metadata_from_parser_aux_t ig_intr_md_from_parser_aux;




header_type ingress_intrinsic_metadata_for_tm_t {
    fields {




        _pad2 : 7;
        ucast_egress_port : 9;


        _pad3 : 7;
        bypass_egress : 1;


        _pad4 : 3;
        qid : 5;


        _pad5 : 7;
        deflect_on_drop : 1;



        _pad6 : 5;
        ingress_cos : 3;



        _pad7 : 6;
        packet_color : 2;



        _pad8 : 7;
        copy_to_cpu : 1;


        _pad9 : 5;
        icos_for_copy_to_cpu : 3;



        mcast_grp_a : 16;



        mcast_grp_b : 16;


        _pad10 : 3;
        level1_mcast_hash : 13;





        _pad11 : 3;
        level2_mcast_hash : 13;





        level1_exclusion_id : 16;



        _pad12 : 7;
        level2_exclusion_id : 9;



        rid : 16;


        _pad13 : 7;
        disable_ucast_cutthru : 1;


        _pad14 : 7;
        enable_mcast_cutthru : 1;

    }
}






@pragma pa_no_tagalong ingress ig_intr_md_for_tm.mcast_grp_a
@pragma pa_no_tagalong ingress ig_intr_md_for_tm.mcast_grp_b


@pragma pa_fragment ingress ig_intr_md_for_tm._pad2
@pragma pa_atomic ingress ig_intr_md_for_tm.ucast_egress_port
@pragma pa_fragment ingress ig_intr_md_for_tm._pad2
@pragma pa_fragment ingress ig_intr_md_for_tm._pad3
@pragma pa_fragment ingress ig_intr_md_for_tm._pad4
@pragma pa_fragment ingress ig_intr_md_for_tm._pad5
@pragma pa_fragment ingress ig_intr_md_for_tm._pad6
@pragma pa_fragment ingress ig_intr_md_for_tm._pad7
@pragma pa_fragment ingress ig_intr_md_for_tm._pad8
@pragma pa_fragment ingress ig_intr_md_for_tm._pad9
@pragma pa_atomic ingress ig_intr_md_for_tm.mcast_grp_a
@pragma pa_fragment ingress ig_intr_md_for_tm.mcast_grp_a
@pragma pa_atomic ingress ig_intr_md_for_tm.mcast_grp_b
@pragma pa_fragment ingress ig_intr_md_for_tm.mcast_grp_b
@pragma pa_atomic ingress ig_intr_md_for_tm.level1_mcast_hash
@pragma pa_fragment ingress ig_intr_md_for_tm._pad10
@pragma pa_atomic ingress ig_intr_md_for_tm.level2_mcast_hash
@pragma pa_fragment ingress ig_intr_md_for_tm._pad11
@pragma pa_atomic ingress ig_intr_md_for_tm.level1_exclusion_id
@pragma pa_fragment ingress ig_intr_md_for_tm.level1_exclusion_id
@pragma pa_atomic ingress ig_intr_md_for_tm.level2_exclusion_id
@pragma pa_fragment ingress ig_intr_md_for_tm._pad12
@pragma pa_atomic ingress ig_intr_md_for_tm.rid
@pragma pa_fragment ingress ig_intr_md_for_tm.rid
@pragma pa_fragment ingress ig_intr_md_for_tm._pad13
@pragma pa_fragment ingress ig_intr_md_for_tm._pad14
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress ingress ig_intr_md_for_tm
header ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm;


header_type ingress_intrinsic_metadata_for_mirror_buffer_t {
    fields {
        _pad1 : 6;
        ingress_mirror_id : 10;


    }
}

@pragma dont_trim
@pragma pa_gress ingress ig_intr_md_for_mb
@pragma pa_atomic ingress ig_intr_md_for_mb.ingress_mirror_id
@pragma pa_no_tagalong ingress ig_intr_md_for_mb.ingress_mirror_id
@pragma not_deparsed ingress
@pragma not_deparsed egress
header ingress_intrinsic_metadata_for_mirror_buffer_t ig_intr_md_for_mb;


header_type egress_intrinsic_metadata_t {
    fields {

        egress_port : 16;


        _pad1: 5;
        enq_qdepth : 19;


        _pad2: 6;
        enq_congest_stat : 2;


        enq_tstamp : 32;


        _pad3: 5;
        deq_qdepth : 19;


        _pad4: 6;
        deq_congest_stat : 2;


        app_pool_congest_stat : 8;



        deq_timedelta : 32;


        egress_rid : 16;


        _pad5: 7;
        egress_rid_first : 1;


        _pad6: 3;
        egress_qid : 5;


        _pad7: 5;
        egress_cos : 3;


        _pad8: 7;
        deflection_flag : 1;


        pkt_length : 16;
    }
}

@pragma dont_trim
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress egress eg_intr_md
@pragma pa_no_tagalong egress eg_intr_md.egress_port
@pragma pa_no_tagalong egress eg_intr_md.egress_cos
header egress_intrinsic_metadata_t eg_intr_md;



header_type egress_intrinsic_metadata_from_parser_aux_t {
    fields {
        egress_global_tstamp : 48;


        egress_global_ver : 32;


        egress_parser_err : 16;


    }
}

@pragma pa_fragment egress eg_intr_md_from_parser_aux.egress_parser_err
@pragma pa_atomic egress eg_intr_md_from_parser_aux.egress_parser_err
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress egress eg_intr_md_from_parser_aux
header egress_intrinsic_metadata_from_parser_aux_t eg_intr_md_from_parser_aux;
# 353 "/home/john/ws/p4f/submodules/p4c-tofino/p4c_tofino/target/tofino/p4_lib/tofino/intrinsic_metadata.p4"
header_type egress_intrinsic_metadata_for_mirror_buffer_t {
    fields {
        _pad1 : 6;
        egress_mirror_id : 10;


        coalesce_flush: 1;
        coalesce_length: 7;



    }
}

@pragma dont_trim
@pragma pa_gress egress eg_intr_md_for_mb
@pragma pa_atomic egress eg_intr_md_for_mb.egress_mirror_id
@pragma pa_no_tagalong ingress ig_intr_md.coalesce_length
@pragma pa_no_tagalong ingress ig_intr_md.coalesce_flush
@pragma pa_fragment ingress eg_intr_md_for_mb.coalesce_flush
@pragma not_deparsed ingress
@pragma not_deparsed egress
header egress_intrinsic_metadata_for_mirror_buffer_t eg_intr_md_for_mb;




header_type egress_intrinsic_metadata_for_output_port_t {
    fields {

        _pad1 : 7;
        capture_tstamp_on_tx : 1;



        _pad2 : 7;
        update_delay_on_tx : 1;
# 400 "/home/john/ws/p4f/submodules/p4c-tofino/p4c_tofino/target/tofino/p4_lib/tofino/intrinsic_metadata.p4"
        _pad3 : 7;
        force_tx_error : 1;
    }
}

@pragma pa_fragment egress eg_intr_md_for_oport._pad2
@pragma pa_fragment egress eg_intr_md_for_oport._pad3
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_gress egress eg_intr_md_for_oport
header egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport;
# 3 "p4src/switch.p4" 2




# 1 "p4src/includes/p4features.h" 1
# 8 "p4src/switch.p4" 2
# 1 "p4src/includes/headers.p4" 1
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

header_type vlan_tag_3b_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 4;
        etherType : 16;
    }
}
header_type vlan_tag_5b_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 20;
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
        type_ : 8;
        code : 8;
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
        reserved : 8;
    }
}


header_type erspan_header_v1_t {
    fields {
        version : 4;
        vlan : 12;
        priority : 6;
        span_id : 10;
        direction : 8;
        truncated: 8;
    }
}


header_type erspan_header_v2_t {
    fields {
        version : 4;
        vlan : 12;
        priority : 6;
        span_id : 10;
        unknown7 : 32;
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





header_type genv_opt_A_t {
    fields {
        optClass : 16;
        optType : 8;
        reserved : 3;
        optLen : 5;
        data : 32;
    }
}




header_type genv_opt_B_t {
    fields {
        optClass : 16;
        optType : 8;
        reserved : 3;
        optLen : 5;
        data : 64;
    }
}




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
# 451 "p4src/includes/headers.p4"
header_type fabric_header_internal_t {
    fields {
        packetType : 3;
        pad1 : 5;
        ingress_tunnel_type : 8;
        egress_bd : 16;
        nexthop_index : 16;
        lkp_mac_type : 16;
        routed : 1;
        outer_routed : 1;
        tunnel_terminate : 1;
        header_count : 4;
        pad2 : 1;




    }
}

header_type fabric_header_t {
    fields {
        packetType : 3;
        headerVersion : 2;
        packetVersion : 2;
        pad1 : 1;

        fabricColor : 3;
        fabricQos : 5;

        dstDevice : 8;
        dstPort : 16;
    }
}

header_type fabric_header_unicast_t {
    fields {
        headerCount : 4;
        tunnelTerminate : 1;
        routed : 1;
        outerRouted : 1;
        pad : 1;
        ingressTunnelType : 8;
        egressBd : 16;
        lkpMacType : 16;
        nexthopIndex : 16;
    }
}

header_type fabric_header_multicast_t {
    fields {
        tunnelTerminate : 1;
        routed : 1;
        outerRouted : 1;
        pad1 : 5;
        ingressTunnelType : 8;
        egressBd : 16;
        lkpMacType : 16;

        mcastGrpA : 16;
        mcastGrpB : 16;
        ingressRid : 16;
        l1ExclusionId : 16;
        l2ExclusionId : 9;
        pad2 : 7;
        l1McastHash : 13;
        pad3 : 3;
        l2McastHash : 13;
        pad4 : 3;
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

header_type fabric_header_control_t {
    fields {
        egressPort : 10;
        egressQueue : 5;
        pad : 1;
        type_ : 1;
        dir : 1;
        redirectToCpu : 1;
        forwardingBypass : 1;
        aclBypass : 1;
        reserved : 3;
    }
}

header_type fabric_header_cpu_t {
    fields {
        reserved : 11;
        egressQueue : 5;
        port : 16;
    }
}

header_type fabric_payload_header_t {
    fields {
        etherType : 16;
    }
}
# 9 "p4src/switch.p4" 2
# 1 "p4src/includes/parser.p4" 1



parser start {
    set_metadata(ingress_metadata.drop_0, 0);
    return parse_ethernet;
}
# 71 "p4src/includes/parser.p4"
header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0 mask 0xfe00: parse_llc_header;
        0 mask 0xfa00: parse_llc_header;
        0x9000 : parse_fabric_header;
        0x8100, 0x9100 : parse_vlan; 0x8847 : parse_mpls; 0x0800 : parse_ipv4; 0x86dd : parse_ipv6; 0x0806 : parse_arp_rarp; 0x8035 : parse_arp_rarp; 0x8915 : parse_roce; 0x8906 : parse_fcoe; 0x8926 : parse_vntag; 0x88cc : parse_set_prio_high; 0x8809 : parse_set_prio_high; default: ingress;
    }
}

header llc_header_t llc_header;

parser parse_llc_header {
    extract(llc_header);
    return select(llc_header.dsap, llc_header.ssap) {
        0xAAAA : parse_snap_header;
        0xFEFE : parse_set_prio_med;
        default: ingress;
    }
}

header snap_header_t snap_header;

parser parse_snap_header {
    extract(snap_header);
    return select(latest.type_) {
        0x8100, 0x9100 : parse_vlan; 0x8847 : parse_mpls; 0x0800 : parse_ipv4; 0x86dd : parse_ipv6; 0x0806 : parse_arp_rarp; 0x8035 : parse_arp_rarp; 0x8915 : parse_roce; 0x8906 : parse_fcoe; 0x8926 : parse_vntag; 0x88cc : parse_set_prio_high; 0x8809 : parse_set_prio_high; default: ingress;
    }
}

header roce_header_t roce;

parser parse_roce {
    extract(roce);
    return ingress;
}

header fcoe_header_t fcoe;

parser parse_fcoe {
    extract(fcoe);
    return ingress;
}


header vlan_tag_t vlan_tag_[2];
header vlan_tag_3b_t vlan_tag_3b[2];
header vlan_tag_5b_t vlan_tag_5b[2];

parser parse_vlan {
    extract(vlan_tag_[next]);
    return select(latest.etherType) {
        0x8100, 0x9100 : parse_vlan; 0x8847 : parse_mpls; 0x0800 : parse_ipv4; 0x86dd : parse_ipv6; 0x0806 : parse_arp_rarp; 0x8035 : parse_arp_rarp; 0x8915 : parse_roce; 0x8906 : parse_fcoe; 0x8926 : parse_vntag; 0x88cc : parse_set_prio_high; 0x8809 : parse_set_prio_high; default: ingress;
    }
}



header mpls_t mpls[3];


parser parse_mpls {
    extract(mpls[next]);
    return select(latest.bos) {
        0 : parse_mpls;
        1 : parse_mpls_bos;
        default: ingress;
    }
}

parser parse_mpls_bos {
# 158 "p4src/includes/parser.p4"
    return select(current(0, 4)) {
        0x4 : parse_mpls_inner_ipv4;
        0x6 : parse_mpls_inner_ipv6;
        default: parse_eompls;
    }

}

parser parse_mpls_inner_ipv4 {
    set_metadata(tunnel_metadata.ingress_tunnel_type, 6);
    return parse_inner_ipv4;
}

parser parse_mpls_inner_ipv6 {
    set_metadata(tunnel_metadata.ingress_tunnel_type, 6);
    return parse_inner_ipv6;
}

parser parse_vpls {
    return ingress;
}

parser parse_pw {
    return ingress;
}
# 206 "p4src/includes/parser.p4"
header ipv4_t ipv4;

field_list ipv4_checksum_list {
        ipv4.version;
        ipv4.ihl;
        ipv4.diffserv;
        ipv4.totalLen;
        ipv4.identification;
        ipv4.flags;
        ipv4.fragOffset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.hdrChecksum {

    verify ipv4_checksum;
    update ipv4_checksum;




}

parser parse_ipv4 {
    extract(ipv4);
    set_metadata(ingress_metadata.ipv4_dstaddr_24b, latest.dstAddr);
    return select(latest.fragOffset, latest.ihl, latest.protocol) {
        0x501 : parse_icmp;
        0x506 : parse_tcp;
        0x511 : parse_udp;
        0x52f : parse_gre;
        0x504 : parse_inner_ipv4;
        0x529 : parse_inner_ipv6;
        2 : parse_set_prio_med;
        88 : parse_set_prio_med;
        89 : parse_set_prio_med;
        103 : parse_set_prio_med;
        112 : parse_set_prio_med;
        default: ingress;
    }
}

header ipv6_t ipv6;

parser parse_ipv6 {
    extract(ipv6);
    set_metadata(ipv6_metadata.lkp_ipv6_sa, latest.srcAddr);
    set_metadata(ipv6_metadata.lkp_ipv6_da, latest.dstAddr);
    return select(latest.nextHdr) {
        58 : parse_icmp;
        6 : parse_tcp;
        17 : parse_udp;
        47 : parse_gre;
        4 : parse_inner_ipv4;
        41 : parse_inner_ipv6;

        88 : parse_set_prio_med;
        89 : parse_set_prio_med;
        103 : parse_set_prio_med;
        112 : parse_set_prio_med;

        default: ingress;
    }
}

header icmp_t icmp;

parser parse_icmp {
    extract(icmp);
    set_metadata(ingress_metadata.lkp_icmp_type, latest.type_);
    set_metadata(ingress_metadata.lkp_icmp_code, latest.code);
    return select(latest.type_) {

        0x82 mask 0xfe : parse_set_prio_med;
        0x84 mask 0xfc : parse_set_prio_med;
        0x88 : parse_set_prio_med;
        default: ingress;
    }
}




header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    set_metadata(ingress_metadata.lkp_l4_sport, latest.srcPort);
    set_metadata(ingress_metadata.lkp_l4_dport, latest.dstPort);
    return select(latest.dstPort) {
        179 : parse_set_prio_med;
        639 : parse_set_prio_med;
        default: ingress;
    }
}
# 327 "p4src/includes/parser.p4"
header udp_t udp;

header roce_v2_header_t roce_v2;

parser parse_roce_v2 {
    extract(roce_v2);
    return ingress;
}

parser parse_udp {
    extract(udp);
    set_metadata(ingress_metadata.lkp_l4_sport, latest.srcPort);
    set_metadata(ingress_metadata.lkp_l4_dport, latest.dstPort);
    return select(latest.dstPort) {
        4789 : parse_vxlan;
        6081: parse_geneve;
        4791: parse_roce_v2;





        67 : parse_set_prio_med;
        68 : parse_set_prio_med;
        546 : parse_set_prio_med;
        547 : parse_set_prio_med;
        520 : parse_set_prio_med;
        521 : parse_set_prio_med;
        1985 : parse_set_prio_med;
        default: ingress;
    }
}

header sctp_t sctp;

parser parse_sctp {
    extract(sctp);
    return ingress;
}





header gre_t gre;

parser parse_gre {
    extract(gre);
    return select(latest.C, latest.R, latest.K, latest.S, latest.s,
                  latest.recurse, latest.flags, latest.ver, latest.proto) {
        0x20006558 : parse_nvgre;
        0x0800 : parse_gre_ipv4;
        0x86dd : parse_gre_ipv6;
        0x88BE : parse_erspan_v1;
        0x22EB : parse_erspan_v2;



        default: ingress;
    }
}

parser parse_gre_ipv4 {
    set_metadata(tunnel_metadata.ingress_tunnel_type, 2);
    return parse_inner_ipv4;
}

parser parse_gre_ipv6 {
    set_metadata(tunnel_metadata.ingress_tunnel_type, 2);
    return parse_inner_ipv6;
}

header nvgre_t nvgre;
header ethernet_t inner_ethernet;

header ipv4_t inner_ipv4;
header ipv6_t inner_ipv6;
header ipv4_t outer_ipv4;
header ipv6_t outer_ipv6;

field_list inner_ipv4_checksum_list {
        inner_ipv4.version;
        inner_ipv4.ihl;
        inner_ipv4.diffserv;
        inner_ipv4.totalLen;
        inner_ipv4.identification;
        inner_ipv4.flags;
        inner_ipv4.fragOffset;
        inner_ipv4.ttl;
        inner_ipv4.protocol;
        inner_ipv4.srcAddr;
        inner_ipv4.dstAddr;
}

field_list_calculation inner_ipv4_checksum {
    input {
        inner_ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field inner_ipv4.hdrChecksum {

    verify inner_ipv4_checksum;
    update inner_ipv4_checksum;




}

header udp_t outer_udp;

parser parse_nvgre {
    extract(nvgre);
    set_metadata(tunnel_metadata.ingress_tunnel_type, 4);
    set_metadata(tunnel_metadata.tunnel_vni, latest.tni);
    return parse_inner_ethernet;
}

header erspan_header_v1_t erspan_v1_header;

parser parse_erspan_v1 {
    extract(erspan_v1_header);
    return ingress;
}

header erspan_header_v2_t erspan_v2_header;

parser parse_erspan_v2 {
    extract(erspan_v2_header);
    return ingress;
}



header arp_rarp_t arp_rarp;

parser parse_arp_rarp {
    extract(arp_rarp);
    return select(latest.protoType) {
        0x0800 : parse_arp_rarp_ipv4;
        default: ingress;
    }
}

header arp_rarp_ipv4_t arp_rarp_ipv4;

parser parse_arp_rarp_ipv4 {
    extract(arp_rarp_ipv4);
    return parse_set_prio_med;
}

header eompls_t eompls;

parser parse_eompls {

    set_metadata(tunnel_metadata.ingress_tunnel_type, 5);
    return parse_inner_ethernet;
}

header vxlan_t vxlan;

parser parse_vxlan {
    extract(vxlan);
    set_metadata(tunnel_metadata.ingress_tunnel_type, 1);
    set_metadata(tunnel_metadata.tunnel_vni, latest.vni);
    return parse_inner_ethernet;
}

header genv_t genv;

parser parse_geneve {
    extract(genv);
    set_metadata(tunnel_metadata.tunnel_vni, latest.vni);
    set_metadata(tunnel_metadata.ingress_tunnel_type, 3);
    return select(genv.ver, genv.optLen, genv.protoType) {
        0x6558 : parse_inner_ethernet;
        0x0800 : parse_inner_ipv4;
        0x86dd : parse_inner_ipv6;
        default : ingress;
    }
}

header nsh_t nsh;
header nsh_context_t nsh_context;

parser parse_nsh {
    extract(nsh);
    extract(nsh_context);
    return select(nsh.protoType) {
        0x0800 : parse_inner_ipv4;
        0x86dd : parse_inner_ipv6;
        0x6558 : parse_inner_ethernet;
        default : ingress;
    }
}

header lisp_t lisp;

parser parse_lisp {
    extract(lisp);
    return select(current(0, 4)) {
        0x4 : parse_inner_ipv4;
        0x6 : parse_inner_ipv6;
        default : ingress;
    }
}

parser parse_inner_ipv4 {
    extract(inner_ipv4);
    return select(latest.fragOffset, latest.ihl, latest.protocol) {
        0x501 : parse_inner_icmp;
        0x506 : parse_inner_tcp;
        0x511 : parse_inner_udp;
        default: ingress;
    }
}

header icmp_t inner_icmp;

parser parse_inner_icmp {
    extract(inner_icmp);
    set_metadata(ingress_metadata.lkp_inner_icmp_type, latest.type_);
    set_metadata(ingress_metadata.lkp_inner_icmp_code, latest.code);
    return ingress;
}

header tcp_t inner_tcp;

parser parse_inner_tcp {
    extract(inner_tcp);
    set_metadata(ingress_metadata.lkp_inner_l4_sport, latest.srcPort);
    set_metadata(ingress_metadata.lkp_inner_l4_dport, latest.dstPort);
    return ingress;
}

header udp_t inner_udp;

parser parse_inner_udp {
    extract(inner_udp);
    set_metadata(ingress_metadata.lkp_inner_l4_sport, latest.srcPort);
    set_metadata(ingress_metadata.lkp_inner_l4_dport, latest.dstPort);
    return ingress;
}

header sctp_t inner_sctp;

parser parse_inner_sctp {
    extract(inner_sctp);
    return ingress;
}

parser parse_inner_ipv6 {
    extract(inner_ipv6);
    return select(latest.nextHdr) {
        58 : parse_inner_icmp;
        6 : parse_inner_tcp;
        17 : parse_inner_udp;
        default: ingress;
    }
}

parser parse_inner_ethernet {
    extract(inner_ethernet);
    return select(latest.etherType) {
        0x0800 : parse_inner_ipv4;
        0x86dd : parse_inner_ipv6;
        default: ingress;
    }
}

header trill_t trill;

parser parse_trill {
    extract(trill);
    return parse_inner_ethernet;
}

header vntag_t vntag;

parser parse_vntag {
    extract(vntag);
    return parse_inner_ethernet;
}

header bfd_t bfd;

parser parse_bfd {
    extract(bfd);
    return parse_set_prio_max;
}

header sflow_t sflow;
header sflow_internal_ethernet_t sflow_internal_ethernet;
header sflow_sample_t sflow_sample;
header sflow_record_t sflow_record;

parser parse_sflow {
    extract(sflow);
    return ingress;
}

parser parse_bf_internal_sflow {
    extract(sflow_internal_ethernet);
    extract(sflow_sample);
    extract(sflow_record);
    return ingress;
}

header fabric_header_t fabric_header;
header fabric_header_unicast_t fabric_header_unicast;
header fabric_header_multicast_t fabric_header_multicast;
header fabric_header_mirror_t fabric_header_mirror;
header fabric_header_control_t fabric_header_control;
header fabric_header_cpu_t fabric_header_cpu;
header fabric_payload_header_t fabric_payload_header;

parser parse_fabric_header {
    return select(current(0, 3)) {
        1 : parse_internal_fabric_header;
        default : parse_external_fabric_header;
    }
}

parser parse_internal_fabric_header {




    return ingress;

}

parser parse_external_fabric_header {
    extract(fabric_header);
    return select(latest.packetType) {
        2 : parse_fabric_header_unicast;
        3 : parse_fabric_header_multicast;
        4 : parse_fabric_header_mirror;
        5 : parse_fabric_header_control;
        6 : parse_fabric_header_cpu;
        default : ingress;
    }
}

parser parse_fabric_header_unicast {
    extract(fabric_header_unicast);
    return parse_fabric_payload_header;
}

parser parse_fabric_header_multicast {
    extract(fabric_header_multicast);
    return parse_fabric_payload_header;
}

parser parse_fabric_header_mirror {
    extract(fabric_header_mirror);
    return parse_fabric_payload_header;
}

parser parse_fabric_header_control {
    extract(fabric_header_control);
    return parse_fabric_payload_header;
}

parser parse_fabric_header_cpu {
    extract(fabric_header_cpu);
    set_metadata(ig_intr_md_for_tm.ucast_egress_port, latest.port);
    return parse_fabric_payload_header;
}

parser parse_fabric_payload_header {
    extract(fabric_payload_header);
    return select(latest.etherType) {
        0 mask 0xfe00: parse_llc_header;
        0 mask 0xfa00: parse_llc_header;
        0x8100, 0x9100 : parse_vlan; 0x8847 : parse_mpls; 0x0800 : parse_ipv4; 0x86dd : parse_ipv6; 0x0806 : parse_arp_rarp; 0x8035 : parse_arp_rarp; 0x8915 : parse_roce; 0x8906 : parse_fcoe; 0x8926 : parse_vntag; 0x88cc : parse_set_prio_high; 0x8809 : parse_set_prio_high; default: ingress;
    }
}
# 718 "p4src/includes/parser.p4"
parser parse_set_prio_med {
    set_metadata(ig_prsr_ctrl.priority, 3);
    return ingress;
}

parser parse_set_prio_high {
    set_metadata(ig_prsr_ctrl.priority, 5);
    return ingress;
}

parser parse_set_prio_max {
    set_metadata(ig_prsr_ctrl.priority, 7);
    return ingress;
}
# 10 "p4src/switch.p4" 2
# 1 "p4src/includes/sizes.p4" 1
# 11 "p4src/switch.p4" 2
# 1 "p4src/includes/defines.p4" 1
# 12 "p4src/switch.p4" 2


header_type ingress_metadata_t {
    fields {
        lkp_l4_sport : 16;
        lkp_l4_dport : 16;
        lkp_inner_l4_sport : 16;
        lkp_inner_l4_dport : 16;

        lkp_icmp_type : 8;
        lkp_icmp_code : 8;
        lkp_inner_icmp_type : 8;
        lkp_inner_icmp_code : 8;

        ifindex : 16;
        vrf : 2;

        outer_bd : 16;
        outer_dscp : 8;

        src_is_link_local : 1;
        bd : 16;
        uuc_mc_index : 16;
        umc_mc_index : 16;
        bcast_mc_index : 16;

        if_label : 16;
        bd_label : 16;

        ipsg_check_fail : 1;

        mirror_session_id : 10;

        marked_cos : 3;
        marked_dscp : 8;
        marked_exp : 3;

        egress_ifindex : 16;
        same_bd_check : 16;

        ipv4_dstaddr_24b : 24;
        drop_0 : 1;
        drop_reason : 8;
        control_frame: 1;
    }
}

header_type egress_metadata_t {
    fields {
        payload_length : 16;
        smac_idx : 9;
        bd : 16;
        inner_replica : 1;
        replica : 1;
        mac_da : 48;
        routed : 1;
        same_bd_check : 16;

        header_count: 4;

        drop_reason : 8;
        egress_bypass : 1;
        fabric_bypass : 1;
        drop_exception : 8;
    }
}

metadata ingress_metadata_t ingress_metadata;
metadata egress_metadata_t egress_metadata;

# 1 "p4src/port.p4" 1

action set_valid_outer_unicast_packet_untagged() {
    modify_field(l2_metadata.lkp_pkt_type, 1);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(ethernet.etherType);
}

action set_valid_outer_unicast_packet_single_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 1);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(vlan_tag_[0].etherType);
}

action set_valid_outer_unicast_packet_double_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 1);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(vlan_tag_[1].etherType);
}

action set_valid_outer_unicast_packet_qinq_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 1);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(ethernet.etherType);
}

action set_valid_outer_multicast_packet_untagged() {
    modify_field(l2_metadata.lkp_pkt_type, 2);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(ethernet.etherType);
}

action set_valid_outer_multicast_packet_single_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 2);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(vlan_tag_[0].etherType);
}

action set_valid_outer_multicast_packet_double_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 2);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(vlan_tag_[1].etherType);
}

action set_valid_outer_multicast_packet_qinq_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 2);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(ethernet.etherType);
}

action set_valid_outer_broadcast_packet_untagged() {
    modify_field(l2_metadata.lkp_pkt_type, 4);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(ethernet.etherType);
}

action set_valid_outer_broadcast_packet_single_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 4);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(vlan_tag_[0].etherType);
}

action set_valid_outer_broadcast_packet_double_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 4);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(vlan_tag_[1].etherType);
}

action set_valid_outer_broadcast_packet_qinq_tagged() {
    modify_field(l2_metadata.lkp_pkt_type, 4);
    modify_field(l2_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 511);
    add_i_fabric_header(ethernet.etherType);
}

table validate_outer_ethernet {
    reads {
        ethernet.dstAddr : ternary;
        vlan_tag_[0] : valid;
        vlan_tag_[1] : valid;
    }
    actions {
        set_valid_outer_unicast_packet_untagged;
        set_valid_outer_unicast_packet_single_tagged;
        set_valid_outer_unicast_packet_double_tagged;
        set_valid_outer_unicast_packet_qinq_tagged;
        set_valid_outer_multicast_packet_untagged;
        set_valid_outer_multicast_packet_single_tagged;
        set_valid_outer_multicast_packet_double_tagged;
        set_valid_outer_multicast_packet_qinq_tagged;
        set_valid_outer_broadcast_packet_untagged;
        set_valid_outer_broadcast_packet_single_tagged;
        set_valid_outer_broadcast_packet_double_tagged;
        set_valid_outer_broadcast_packet_qinq_tagged;
    }
    size : 64;
}

control validate_outer_ethernet_header {
    apply(validate_outer_ethernet);
}
action set_ifindex(ifindex, if_label, exclusion_id) {
    modify_field(ingress_metadata.ifindex, ifindex);
    modify_field(ig_intr_md_for_tm.level2_exclusion_id, exclusion_id);
    modify_field(ingress_metadata.if_label, if_label);
}

table port_mapping {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        set_ifindex;
    }
    size : 288;
}

control process_port_mapping {
    apply(port_mapping);
}

action set_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode,
        stp_group, exclusion_id, stats_idx) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(multicast_metadata.outer_ipv4_mcast_key_type, 0);
    modify_field(multicast_metadata.outer_ipv4_mcast_key, bd);
    modify_field(multicast_metadata.outer_ipv6_mcast_key_type, 0);
    modify_field(multicast_metadata.outer_ipv6_mcast_key, bd);

    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ipv6_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(multicast_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(multicast_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(multicast_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(multicast_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ipv4_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ipv6_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(multicast_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(l2_metadata.stp_group, stp_group);
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, exclusion_id);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);
}

action set_bd_ipv4_mcast_switch_ipv6_mcast_route_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode,
        stp_group, exclusion_id, stats_idx) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(multicast_metadata.outer_ipv4_mcast_key_type, 0);
    modify_field(multicast_metadata.outer_ipv4_mcast_key, bd);
    modify_field(multicast_metadata.outer_ipv6_mcast_key_type, 1);
    modify_field(multicast_metadata.outer_ipv6_mcast_key, vrf);

    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ipv6_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(multicast_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(multicast_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(multicast_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(multicast_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ipv4_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ipv6_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(multicast_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(l2_metadata.stp_group, stp_group);
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, exclusion_id);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);
}

action set_bd_ipv4_mcast_route_ipv6_mcast_switch_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode,
        stp_group, exclusion_id, stats_idx) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(multicast_metadata.outer_ipv4_mcast_key_type, 1);
    modify_field(multicast_metadata.outer_ipv4_mcast_key, vrf);
    modify_field(multicast_metadata.outer_ipv6_mcast_key_type, 0);
    modify_field(multicast_metadata.outer_ipv6_mcast_key, bd);

    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ipv6_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(multicast_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(multicast_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(multicast_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(multicast_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ipv4_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ipv6_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(multicast_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(l2_metadata.stp_group, stp_group);
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, exclusion_id);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);
}

action set_bd_ipv4_mcast_route_ipv6_mcast_route_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode,
        stp_group, exclusion_id, stats_idx) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(multicast_metadata.outer_ipv4_mcast_key_type, 1);
    modify_field(multicast_metadata.outer_ipv4_mcast_key, vrf);
    modify_field(multicast_metadata.outer_ipv6_mcast_key_type, 1);
    modify_field(multicast_metadata.outer_ipv6_mcast_key, vrf);

    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ipv6_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(multicast_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(multicast_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(multicast_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(multicast_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ipv4_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ipv6_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(multicast_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(l2_metadata.stp_group, stp_group);
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, exclusion_id);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);
}

action set_bd(bd, vrf, rmac_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled,
        igmp_snooping_enabled,
        stp_group, exclusion_id, stats_idx) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(multicast_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(l2_metadata.stp_group, stp_group);
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, exclusion_id);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);
}

action_profile bd_action_profile {
    actions {
        set_bd;
        set_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags;
        set_bd_ipv4_mcast_switch_ipv6_mcast_route_flags;
        set_bd_ipv4_mcast_route_ipv6_mcast_switch_flags;
        set_bd_ipv4_mcast_route_ipv6_mcast_route_flags;
    }
    size : 16384;
}

table port_vlan_mapping {
    reads {
        ingress_metadata.ifindex : exact;
        vlan_tag_[0] : valid;
        vlan_tag_[0].vid : exact;
        vlan_tag_[1] : valid;
        vlan_tag_[1].vid : exact;
    }

    action_profile: bd_action_profile;
    size : 32768;
}

control process_port_vlan_mapping {
    apply(port_vlan_mapping);
}


counter ingress_bd_stats {
    type : packets_and_bytes;
    instance_count : 16384;
}

action update_ingress_bd_stats() {
    count(ingress_bd_stats, l2_metadata.bd_stats_idx);
}

table ingress_bd_stats {
    actions {
        update_ingress_bd_stats;
    }
    size : 64;
}


control process_ingress_bd_stats {
    apply(ingress_bd_stats);
}

field_list lag_hash_fields {
    l2_metadata.lkp_mac_sa;
    l2_metadata.lkp_mac_da;
    i_fabric_header.lkp_mac_type;
    ipv4_metadata.lkp_ipv4_sa;
    ipv4_metadata.lkp_ipv4_da;
    l3_metadata.lkp_ip_proto;
    ingress_metadata.lkp_l4_sport;
    ingress_metadata.lkp_l4_dport;
}

field_list_calculation lag_hash {
    input {
        lag_hash_fields;
    }
    algorithm : crc16;
    output_width : 8;
}

action_selector lag_selector {
    selection_key : lag_hash;
}







action set_lag_port(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}


action_profile lag_action_profile {
    actions {
        nop;
        set_lag_port;
    }
    size : 1024;
    dynamic_action_selection : lag_selector;
}

table lag_group {
    reads {
        ingress_metadata.egress_ifindex : exact;
    }
    action_profile: lag_action_profile;
    size : 1024;
}

control process_lag {
    apply(lag_group);
}

action set_egress_packet_vlan_tagged(vlan_id) {
    add_header(vlan_tag_[0]);
    modify_field(vlan_tag_[0].etherType, ethernet.etherType);
    modify_field(vlan_tag_[0].vid, vlan_id);
    modify_field(ethernet.etherType, 0x8100);
}

action set_egress_packet_vlan_untagged() {
}

table egress_vlan_xlate {
    reads {
        eg_intr_md.egress_port : exact;
        egress_metadata.bd : exact;
    }
    actions {
        set_egress_packet_vlan_tagged;
        set_egress_packet_vlan_untagged;
    }
    size : 32768;
}

control process_vlan_xlate {
    apply(egress_vlan_xlate);
}
# 83 "p4src/switch.p4" 2
# 1 "p4src/l2.p4" 1
header_type l2_metadata_t {
    fields {
        lkp_pkt_type : 3;
        lkp_mac_sa : 48;
        lkp_mac_da : 48;

        l2_nexthop : 16;
        l2_nexthop_type : 1;
        l2_redirect : 1;
        l2_src_miss : 1;
        l2_src_move : 16;
        stp_group: 10;
        stp_state : 3;
        bd_stats_idx : 16;
    }
}

metadata l2_metadata_t l2_metadata;
# 37 "p4src/l2.p4"
control process_spanning_tree {





}
# 117 "p4src/l2.p4"
control process_mac {




}
# 149 "p4src/l2.p4"
control process_mac_learning {



}

action set_unicast() {
}

action set_unicast_and_ipv6_src_is_link_local() {
    modify_field(ingress_metadata.src_is_link_local, 1);
}

action set_multicast() {
    add_to_field(l2_metadata.bd_stats_idx, 1);
}

action set_ip_multicast() {
    modify_field(multicast_metadata.ip_multicast, 1);
    add_to_field(l2_metadata.bd_stats_idx, 1);
}

action set_ip_multicast_and_ipv6_src_is_link_local() {
    modify_field(multicast_metadata.ip_multicast, 1);
    modify_field(ingress_metadata.src_is_link_local, 1);
    add_to_field(l2_metadata.bd_stats_idx, 1);
}

action set_broadcast() {
    add_to_field(l2_metadata.bd_stats_idx, 2);
}

table validate_packet {
    reads {
        l2_metadata.lkp_mac_da : ternary;



    }
    actions {
        nop;
        set_unicast;
        set_unicast_and_ipv6_src_is_link_local;
        set_multicast;
        set_ip_multicast;
        set_ip_multicast_and_ipv6_src_is_link_local;
        set_broadcast;
    }
    size : 64;
}

control process_validate_packet {
    apply(validate_packet);
}


action rewrite_unicast_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, egress_metadata.mac_da);
}

action rewrite_multicast_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
    modify_field(ethernet.dstAddr, 0x01005E000000);



    add_to_field(ipv4.ttl, -1);
}

table mac_rewrite {
    reads {
        egress_metadata.smac_idx : exact;
        ipv4.dstAddr : ternary;
    }
    actions {
        nop;
        rewrite_unicast_mac;
        rewrite_multicast_mac;
    }
    size : 512;
}

control process_mac_rewrite {
    if (i_fabric_header.routed == 1) {
        apply(mac_rewrite);
    }
}
# 286 "p4src/l2.p4"
control process_replication {
# 296 "p4src/l2.p4"
}

action set_egress_bd_properties(nat_mode) {
    modify_field(nat_metadata.egress_nat_mode, nat_mode);
}

table egress_bd_map {
    reads {
        i_fabric_header.egress_bd : exact;
    }
    actions {
        nop;
        set_egress_bd_properties;
    }
    size : 16384;
}

control process_egress_bd {
    apply(egress_bd_map);
}

action vlan_decap_nop() {
    modify_field(ethernet.etherType, i_fabric_header.lkp_mac_type);
}

action remove_vlan_single_tagged() {
    remove_header(vlan_tag_[0]);
    modify_field(ethernet.etherType, i_fabric_header.lkp_mac_type);
}

action remove_vlan_double_tagged() {
    remove_header(vlan_tag_[0]);
    remove_header(vlan_tag_[1]);
    modify_field(ethernet.etherType, i_fabric_header.lkp_mac_type);
}

action remove_vlan_qinq_tagged() {
    remove_header(vlan_tag_[0]);
    remove_header(vlan_tag_[1]);
    modify_field(ethernet.etherType, i_fabric_header.lkp_mac_type);
}

table vlan_decap {
    reads {
        egress_metadata.drop_exception : exact;
        vlan_tag_[0] : valid;
        vlan_tag_[1] : valid;
    }
    actions {
        vlan_decap_nop;
        remove_vlan_single_tagged;
        remove_vlan_double_tagged;
        remove_vlan_qinq_tagged;
    }
    size: 256;
}

control process_vlan_decap {
    apply(vlan_decap);
}
# 84 "p4src/switch.p4" 2
# 1 "p4src/l3.p4" 1




 header_type l3_metadata_t {
     fields {
        lkp_ip_type : 2;
        lkp_ip_proto : 8;
        lkp_ip_tc : 8;
        lkp_ip_ttl : 8;
        rmac_group : 10;
        rmac_hit : 1;
        urpf_mode : 2;
        urpf_hit : 1;
        urpf_check_fail :1;
        urpf_bd_group : 16;
        fib_hit : 1;
        fib_nexthop : 16;
        fib_nexthop_type : 1;
     }
 }

 metadata l3_metadata_t l3_metadata;

action fib_hit_nexthop(nexthop_index) {
    modify_field(l3_metadata.fib_hit, 1);
    modify_field(l3_metadata.fib_nexthop, nexthop_index);
    modify_field(l3_metadata.fib_nexthop_type, 0);
}

action fib_hit_ecmp(ecmp_index) {
    modify_field(l3_metadata.fib_hit, 1);
    modify_field(l3_metadata.fib_nexthop, ecmp_index);
    modify_field(l3_metadata.fib_nexthop_type, 1);
}

action rmac_hit() {
    modify_field(l3_metadata.rmac_hit, 1);
    modify_field(ingress_metadata.egress_ifindex, 64);
    modify_field(ig_intr_md_for_tm.mcast_grp_a, 0);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action rmac_miss() {
    modify_field(l3_metadata.rmac_hit, 0);
}

table rmac {
    reads {
        l3_metadata.rmac_group : exact;
        l2_metadata.lkp_mac_da : exact;
    }
    actions {
        rmac_hit;
        rmac_miss;
    }
    size : 512;
}
# 85 "p4src/l3.p4"
control process_urpf_bd {






}
# 117 "p4src/l3.p4"
control process_mtu {



}
# 85 "p4src/switch.p4" 2
# 1 "p4src/ipv4.p4" 1
# 18 "p4src/ipv4.p4"
 header_type ipv4_metadata_t {
     fields {
        lkp_ipv4_sa : 32;
        lkp_ipv4_da : 32;
        ipv4_unicast_enabled : 1;
        ipv4_urpf_mode : 2;

        fib_hit_exm_prefix_length_32 : 1;
        fib_nexthop_exm_prefix_length_32 :10;
        fib_nexthop_type_exm_prefix_length_32 : 1;

        fib_hit_exm_prefix_length_31 : 1;
        fib_nexthop_exm_prefix_length_31 :10;
        fib_nexthop_type_exm_prefix_length_31 : 1;

        fib_hit_exm_prefix_length_30 : 1;
        fib_nexthop_exm_prefix_length_30 :10;
        fib_nexthop_type_exm_prefix_length_30 : 1;

        fib_hit_exm_prefix_length_29 : 1;
        fib_nexthop_exm_prefix_length_29 :10;
        fib_nexthop_type_exm_prefix_length_29 : 1;

        fib_hit_exm_prefix_length_28 : 1;
        fib_nexthop_exm_prefix_length_28 :10;
        fib_nexthop_type_exm_prefix_length_28 : 1;

        fib_hit_exm_prefix_length_27 : 1;
        fib_nexthop_exm_prefix_length_27 :10;
        fib_nexthop_type_exm_prefix_length_27 : 1;

        fib_hit_exm_prefix_length_26 : 1;
        fib_nexthop_exm_prefix_length_26 :10;
        fib_nexthop_type_exm_prefix_length_26 : 1;

        fib_hit_exm_prefix_length_25 : 1;
        fib_nexthop_exm_prefix_length_25 :10;
        fib_nexthop_type_exm_prefix_length_25 : 1;

        fib_hit_exm_prefix_length_24 : 1;
        fib_nexthop_exm_prefix_length_24 :10;
        fib_nexthop_type_exm_prefix_length_24 : 1;

        fib_hit_exm_prefix_length_23 : 1;
        fib_nexthop_exm_prefix_length_23 :10;
        fib_nexthop_type_exm_prefix_length_23 : 1;

        fib_hit_lpm_prefix_range_22_to_0 : 1;
        fib_nexthop_lpm_prefix_range_22_to_0 :10;
        fib_nexthop_type_lpm_prefix_range_22_to_0 : 1;

     }
 }

metadata ipv4_metadata_t ipv4_metadata;



action set_valid_outer_ipv4_packet() {
    modify_field(l3_metadata.lkp_ip_type, 1);
    modify_field(ipv4_metadata.lkp_ipv4_sa, ipv4.srcAddr);
    modify_field(ipv4_metadata.lkp_ipv4_da, ipv4.dstAddr);
    modify_field(l3_metadata.lkp_ip_proto, ipv4.protocol);
    modify_field(l3_metadata.lkp_ip_tc, ipv4.diffserv);
    modify_field(l3_metadata.lkp_ip_ttl, ipv4.ttl);
}

action set_malformed_outer_ipv4_packet() {
}

table validate_outer_ipv4_packet {
    reads {
        ipv4.version : exact;
        ipv4.ihl : exact;
        ipv4.ttl : exact;
        ipv4.srcAddr : ternary;
        ipv4.dstAddr : ternary;
    }
    actions {
        set_valid_outer_ipv4_packet;
        set_malformed_outer_ipv4_packet;
    }
    size : 64;
}


control validate_outer_ipv4_header {

    apply(validate_outer_ipv4_packet);

}



action fib_hit_exm_prefix_length_32_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_32, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_32,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_32,
                 0);
}

action fib_hit_exm_prefix_length_31_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_31, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_31,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_31,
                 0);
}

action fib_hit_exm_prefix_length_30_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_30, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_30,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_30,
                 0);
}

action fib_hit_exm_prefix_length_29_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_29, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_29,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_29,
                 0);
}

action fib_hit_exm_prefix_length_28_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_28, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_28,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_28,
                 0);
}

action fib_hit_exm_prefix_length_27_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_27, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_27,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_27,
                 0);
}

action fib_hit_exm_prefix_length_26_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_26, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_26,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_26,
                 0);
}

action fib_hit_exm_prefix_length_25_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_25, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_25,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_25,
                 0);
}

action fib_hit_exm_prefix_length_24_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_24, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_24,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_24,
                 0);
}

action fib_hit_exm_prefix_length_23_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_23, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_23,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_23,
                 0);
}

action fib_hit_lpm_prefix_range_22_to_0_nexthop(nexthop_index) {
    modify_field(ipv4_metadata.fib_hit_lpm_prefix_range_22_to_0, 1);
    modify_field(ipv4_metadata.fib_nexthop_lpm_prefix_range_22_to_0,
                 nexthop_index);
    modify_field(ipv4_metadata.fib_nexthop_type_lpm_prefix_range_22_to_0,
                 0);
}

action fib_hit_exm_prefix_length_32_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_32, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_32, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_32,
                 1);
}

action fib_hit_exm_prefix_length_31_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_31, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_31, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_31,
                 1);
}

action fib_hit_exm_prefix_length_30_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_30, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_30, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_30,
                 1);
}

action fib_hit_exm_prefix_length_29_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_29, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_29, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_29,
                 1);
}

action fib_hit_exm_prefix_length_28_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_28, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_28, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_28,
                 1);
}

action fib_hit_exm_prefix_length_27_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_27, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_27, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_27,
                 1);
}

action fib_hit_exm_prefix_length_26_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_26, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_26, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_26,
                 1);
}

action fib_hit_exm_prefix_length_25_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_25, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_25, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_25,
                 1);
}

action fib_hit_exm_prefix_length_24_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_24, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_24, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_24,
                 1);
}

action fib_hit_exm_prefix_length_23_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_exm_prefix_length_23, 1);
    modify_field(ipv4_metadata.fib_nexthop_exm_prefix_length_23, ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_exm_prefix_length_23,
                 1);
}

action fib_hit_lpm_prefix_range_22_to_0_ecmp(ecmp_index) {
    modify_field(ipv4_metadata.fib_hit_lpm_prefix_range_22_to_0, 1);
    modify_field(ipv4_metadata.fib_nexthop_lpm_prefix_range_22_to_0,
                 ecmp_index);
    modify_field(ipv4_metadata.fib_nexthop_type_lpm_prefix_range_22_to_0,
                 1);
}

table ipv4_fib_exm_prefix_length_32 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_32_nexthop;
        fib_hit_exm_prefix_length_32_ecmp;
    }
    size : 19200;
}

table ipv4_fib_exm_prefix_length_31 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFFFE: exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_31_nexthop;
        fib_hit_exm_prefix_length_31_ecmp;
    }
    size : 1024;
}

table ipv4_fib_exm_prefix_length_30 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFFFC: exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_30_nexthop;
        fib_hit_exm_prefix_length_30_ecmp;
    }
    size : 23040;
}

table ipv4_fib_exm_prefix_length_29 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFFF8 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_29_nexthop;
        fib_hit_exm_prefix_length_29_ecmp;
    }
    size : 15360;
}

table ipv4_fib_exm_prefix_length_28 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFFF0 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_28_nexthop;
        fib_hit_exm_prefix_length_28_ecmp;
    }
    size : 30720;
}

table ipv4_fib_exm_prefix_length_27 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFFE0 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_27_nexthop;
        fib_hit_exm_prefix_length_27_ecmp;
    }
    size : 7680;
}

table ipv4_fib_exm_prefix_length_26 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFFC0 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_26_nexthop;
        fib_hit_exm_prefix_length_26_ecmp;
    }
    size : 7680;
}

table ipv4_fib_exm_prefix_length_25 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFF80 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_25_nexthop;
        fib_hit_exm_prefix_length_25_ecmp;
    }
    size : 3840;
}

table ipv4_fib_exm_prefix_length_24 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFF00 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_24_nexthop;
        fib_hit_exm_prefix_length_24_ecmp;
    }
    size : 38400;
}

table ipv4_fib_exm_prefix_length_23 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da mask 0xFFFFFE00 : exact;
    }
    actions {
        on_miss;
        fib_hit_exm_prefix_length_23_nexthop;
        fib_hit_exm_prefix_length_23_ecmp;
    }
    size : 30720;
}

table ipv4_fib_lpm_prefix_range_22_to_0 {
    reads {
        ingress_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da : lpm;
    }
    actions {
        on_miss;
        fib_hit_lpm_prefix_range_22_to_0_nexthop;
        fib_hit_lpm_prefix_range_22_to_0_ecmp;
    }
    size : 512;
}
# 449 "p4src/ipv4.p4"
control process_ipv4_fib {



    apply(ipv4_fib_exm_prefix_length_32);
    apply(ipv4_fib_exm_prefix_length_31);
    apply(ipv4_fib_exm_prefix_length_30);
    apply(ipv4_fib_exm_prefix_length_29);
    apply(ipv4_fib_exm_prefix_length_28);
    apply(ipv4_fib_exm_prefix_length_27);
    apply(ipv4_fib_exm_prefix_length_26);
    apply(ipv4_fib_exm_prefix_length_25);
    apply(ipv4_fib_exm_prefix_length_24);
    apply(ipv4_fib_exm_prefix_length_23);
    apply(ipv4_fib_lpm_prefix_range_22_to_0);
# 472 "p4src/ipv4.p4"
}
# 506 "p4src/ipv4.p4"
control process_ipv4_urpf {
# 517 "p4src/ipv4.p4"
}
# 86 "p4src/switch.p4" 2
# 1 "p4src/ipv6.p4" 1
# 11 "p4src/ipv6.p4"
header_type ipv6_metadata_t {
    fields {
        lkp_ipv6_sa : 128;
        lkp_ipv6_da : 128;
        ipv6_unicast_enabled : 1;
        ipv6_urpf_mode : 2;

        fib_hit_exm_prefix_length_128 : 1;
        fib_nexthop_exm_prefix_length_128 : 16;
        fib_nexthop_type_exm_prefix_length_128 : 1;

        fib_hit_lpm_prefix_range_127_to_65 : 1;
        fib_nexthop_lpm_prefix_range_127_to_65 : 16;
        fib_nexthop_type_lpm_prefix_range_127_to_65 : 1;

        fib_hit_exm_prefix_length_64 : 1;
        fib_nexthop_exm_prefix_length_64 : 16;
        fib_nexthop_type_exm_prefix_length_64 : 1;

        fib_hit_lpm_prefix_range_63_to_0 : 1;
        fib_nexthop_lpm_prefix_range_63_to_0 : 16;
        fib_nexthop_type_lpm_prefix_range_63_to_0 : 1;

    }
}
metadata ipv6_metadata_t ipv6_metadata;
# 67 "p4src/ipv6.p4"
control validate_outer_ipv6_header {



}
# 233 "p4src/ipv6.p4"
control process_ipv6_fib {
# 249 "p4src/ipv6.p4"
}
# 284 "p4src/ipv6.p4"
control process_ipv6_urpf {
# 295 "p4src/ipv6.p4"
}
# 87 "p4src/switch.p4" 2
# 1 "p4src/tunnel.p4" 1



header_type tunnel_metadata_t {
    fields {
        ingress_tunnel_type : 8;
        tunnel_vni : 24;
        mpls_enabled : 1;
        mpls_label: 20;
        mpls_exp: 3;
        mpls_ttl: 8;
        egress_tunnel_type : 8;
        tunnel_index: 14;
        tunnel_src_index : 9;
        tunnel_smac_index : 9;
        tunnel_dst_index : 14;
        tunnel_dmac_index : 14;
        vnid : 24;
    }
}
metadata tunnel_metadata_t tunnel_metadata;
# 107 "p4src/tunnel.p4"
control process_ipv4_vtep {







}

control process_ipv6_vtep {







}
# 302 "p4src/tunnel.p4"
control process_tunnel {



}
# 346 "p4src/tunnel.p4"
control validate_mpls_header {



}
# 440 "p4src/tunnel.p4"
control process_mpls {



}
# 747 "p4src/tunnel.p4"
control process_tunnel_decap {
# 757 "p4src/tunnel.p4"
}
# 1316 "p4src/tunnel.p4"
control process_tunnel_encap {
# 1333 "p4src/tunnel.p4"
}
# 88 "p4src/switch.p4" 2
# 1 "p4src/acl.p4" 1



header_type acl_metadata_t {
    fields {
        acl_deny : 1;
        racl_deny : 1;
        acl_nexthop : 16;
        racl_nexthop : 16;
        acl_nexthop_type : 1;
        racl_nexthop_type : 1;
        acl_redirect : 1;
        racl_redirect : 1;
    }
}

metadata acl_metadata_t acl_metadata;
# 78 "p4src/acl.p4"
control process_mac_acl {





}
# 170 "p4src/acl.p4"
control process_ip_acl {
# 184 "p4src/acl.p4"
}
# 227 "p4src/acl.p4"
control process_qos {





}


action racl_log() {
}

action racl_deny() {
    modify_field(acl_metadata.racl_deny, 1);
}

action racl_permit() {
}

action racl_set_nat_mode(nat_mode) {
    modify_field(nat_metadata.ingress_nat_mode, nat_mode);
}

action racl_redirect_nexthop(nexthop_index) {
    modify_field(acl_metadata.racl_redirect, 1);
    modify_field(acl_metadata.racl_nexthop, nexthop_index);
    modify_field(acl_metadata.racl_nexthop_type, 0);
}

action racl_redirect_ecmp(ecmp_index) {
    modify_field(acl_metadata.racl_redirect, 1);
    modify_field(acl_metadata.racl_nexthop, ecmp_index);
    modify_field(acl_metadata.racl_nexthop_type, 1);
}
# 296 "p4src/acl.p4"
control process_ipv4_racl {





}
# 330 "p4src/acl.p4"
control process_ipv6_racl {





}

field_list mirror_info {
    ingress_metadata.ifindex;
    ingress_metadata.drop_reason;
    l3_metadata.lkp_ip_ttl;
}

action negative_mirror(clone_spec, drop_reason) {
    modify_field(ingress_metadata.drop_reason, drop_reason);
    clone_ingress_pkt_to_egress(clone_spec, mirror_info);
    drop();
}

action redirect_to_cpu() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 64);
    modify_field(ig_intr_md_for_tm.mcast_grp_a, 0);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action copy_to_cpu() {
    clone_ingress_pkt_to_egress(250);
}

action drop_packet() {
    drop();
}

table system_acl {
    reads {
        ingress_metadata.if_label : ternary;
        ingress_metadata.bd_label : ternary;


        ipv4_metadata.lkp_ipv4_sa : ternary;
        ipv4_metadata.lkp_ipv4_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;


        l2_metadata.lkp_mac_sa : ternary;
        l2_metadata.lkp_mac_da : ternary;
        i_fabric_header.lkp_mac_type : ternary;


        ingress_metadata.ipsg_check_fail : ternary;
        acl_metadata.acl_deny : ternary;
        acl_metadata.racl_deny: ternary;
        l3_metadata.urpf_check_fail : ternary;





        i_fabric_header.routed : ternary;
        ingress_metadata.src_is_link_local : ternary;
        ingress_metadata.same_bd_check : ternary;
        l3_metadata.lkp_ip_ttl : ternary;
        l2_metadata.stp_state : ternary;
        ingress_metadata.control_frame: ternary;
        ipv4_metadata.ipv4_unicast_enabled : ternary;


        ig_intr_md_for_tm.ucast_egress_port : ternary;
    }
    actions {
        nop;
        redirect_to_cpu;
        copy_to_cpu;
        drop_packet;
        negative_mirror;
    }
    size : 512;
}

control process_system_acl {
    apply(system_acl);
}
# 452 "p4src/acl.p4"
control process_egress_acl {



}
# 89 "p4src/switch.p4" 2
# 1 "p4src/nat.p4" 1
header_type nat_metadata_t {
    fields {
        ingress_nat_mode : 2;
        egress_nat_mode : 2;
        nat_nexthop : 16;
        nat_hit : 1;
        nat_rewrite_index : 16;
    }
}

metadata nat_metadata_t nat_metadata;
# 79 "p4src/nat.p4"
control process_nat {
# 94 "p4src/nat.p4"
}
# 109 "p4src/nat.p4"
control process_egress_nat {
# 118 "p4src/nat.p4"
}
# 90 "p4src/switch.p4" 2
# 1 "p4src/multicast.p4" 1
header_type multicast_metadata_t {
    fields {
        outer_ipv4_mcast_key_type : 1;
        outer_ipv4_mcast_key : 8;
        outer_ipv6_mcast_key_type : 1;
        outer_ipv6_mcast_key : 8;
        outer_mcast_route_hit : 1;
        outer_mcast_mode : 2;
        ip_multicast : 1;
        mcast_route_hit : 1;
        mcast_bridge_hit : 1;
        ipv4_multicast_mode : 2;
        ipv6_multicast_mode : 2;
        igmp_snooping_enabled : 1;
        mld_snooping_enabled : 1;
        bd_mrpf_group : 16;
        mcast_rpf_group : 16;
        mcast_mode : 2;
        multicast_route_mc_index : 16;
        multicast_bridge_mc_index : 16;
    }
}

metadata multicast_metadata_t multicast_metadata;
# 45 "p4src/multicast.p4"
control process_outer_multicast_rpf {
# 54 "p4src/multicast.p4"
}
# 145 "p4src/multicast.p4"
control process_outer_ipv4_multicast {
# 156 "p4src/multicast.p4"
}
# 192 "p4src/multicast.p4"
control process_outer_ipv6_multicast {
# 203 "p4src/multicast.p4"
}

control process_outer_multicast {
# 216 "p4src/multicast.p4"
}
# 244 "p4src/multicast.p4"
control process_multicast_rpf {







}
# 350 "p4src/multicast.p4"
control process_ipv4_multicast {
# 368 "p4src/multicast.p4"
}
# 427 "p4src/multicast.p4"
control process_ipv6_multicast {
# 444 "p4src/multicast.p4"
}

control process_multicast {
# 459 "p4src/multicast.p4"
}
# 91 "p4src/switch.p4" 2
# 1 "p4src/nexthop.p4" 1



header_type nexthop_metadata_t {
    fields {
        nexthop_type : 1;
    }
}

metadata nexthop_metadata_t nexthop_metadata;

action set_l2_redirect_action() {
    modify_field(i_fabric_header.nexthop_index, l2_metadata.l2_nexthop);
    modify_field(nexthop_metadata.nexthop_type,
                 l2_metadata.l2_nexthop_type);
}

action set_acl_redirect_action() {
    modify_field(i_fabric_header.nexthop_index, acl_metadata.acl_nexthop);
    modify_field(nexthop_metadata.nexthop_type,
                 acl_metadata.acl_nexthop_type);
}

action set_racl_redirect_action() {
    modify_field(i_fabric_header.nexthop_index, acl_metadata.racl_nexthop);
    modify_field(nexthop_metadata.nexthop_type,
                 acl_metadata.racl_nexthop_type);
    modify_field(i_fabric_header.routed, 1);
}

action set_fib_redirect_action() {
    modify_field(i_fabric_header.nexthop_index, l3_metadata.fib_nexthop);
    modify_field(nexthop_metadata.nexthop_type,
                 l3_metadata.fib_nexthop_type);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_nat_redirect_action() {
    modify_field(i_fabric_header.nexthop_index, nat_metadata.nat_nexthop);
    modify_field(nexthop_metadata.nexthop_type, 0);
    modify_field(i_fabric_header.routed, 1);
}

action set_multicast_route_action() {
    modify_field(ig_intr_md_for_tm.mcast_grp_b,
                 multicast_metadata.multicast_route_mc_index);



}

action set_multicast_bridge_action() {
    modify_field(ig_intr_md_for_tm.mcast_grp_b,
                 multicast_metadata.multicast_bridge_mc_index);



}



action set_fib_exm_prefix_length_32_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_32);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_31_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_31);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_30_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_30);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_29_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_29);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_28_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_28);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_27_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_27);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_26_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_26);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_25_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_25);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_24_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_24);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_exm_prefix_length_23_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_exm_prefix_length_23);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}

action set_fib_lpm_prefix_range_22_to_0_redirect_action() {

    modify_field(nexthop_metadata.nexthop_type,
                 ipv4_metadata.fib_nexthop_type_lpm_prefix_range_22_to_0);
    modify_field(i_fabric_header.routed, 1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, 0);



}
# 239 "p4src/nexthop.p4"
table fwd_result {
    reads {
# 263 "p4src/nexthop.p4"
        ipv4_metadata.fib_hit_exm_prefix_length_32 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_31 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_30 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_29 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_28 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_27 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_26 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_25 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_24 : ternary;
        ipv4_metadata.fib_hit_exm_prefix_length_23 : ternary;
        ipv4_metadata.fib_hit_lpm_prefix_range_22_to_0 : ternary;
# 282 "p4src/nexthop.p4"
    }
    actions {
        nop;
        set_l2_redirect_action;
        set_acl_redirect_action;
        set_racl_redirect_action;
        set_fib_redirect_action;
        set_nat_redirect_action;






        set_fib_exm_prefix_length_32_redirect_action;
        set_fib_exm_prefix_length_31_redirect_action;
        set_fib_exm_prefix_length_30_redirect_action;
        set_fib_exm_prefix_length_29_redirect_action;
        set_fib_exm_prefix_length_28_redirect_action;
        set_fib_exm_prefix_length_27_redirect_action;
        set_fib_exm_prefix_length_26_redirect_action;
        set_fib_exm_prefix_length_25_redirect_action;
        set_fib_exm_prefix_length_24_redirect_action;
        set_fib_exm_prefix_length_23_redirect_action;
        set_fib_lpm_prefix_range_22_to_0_redirect_action;
# 315 "p4src/nexthop.p4"
    }
    size : 512;
}

control process_merge_results {
    apply(fwd_result);
}

field_list l3_hash_fields {
    ipv4_metadata.lkp_ipv4_sa;
    ipv4_metadata.lkp_ipv4_da;
    l3_metadata.lkp_ip_proto;
    ingress_metadata.lkp_l4_sport;
    ingress_metadata.lkp_l4_dport;
}

field_list_calculation ecmp_hash {
    input {
        l3_hash_fields;
    }
    algorithm : crc16;
    output_width : 10;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash;
}

action_profile ecmp_action_profile {
    actions {
        nop;
        set_ecmp_nexthop_details;
        set_ecmp_nexthop_details_for_post_routed_flood;
    }
    size : 16384;
    dynamic_action_selection : ecmp_selector;
}

table ecmp_group {
    reads {
        i_fabric_header.nexthop_index : exact;
    }
    action_profile: ecmp_action_profile;
    size : 1024;
}

action set_nexthop_details(ifindex, bd) {
    modify_field(ingress_metadata.egress_ifindex, ifindex);
    modify_field(i_fabric_header.egress_bd, bd);
    bit_xor(ingress_metadata.same_bd_check, ingress_metadata.bd, bd);
}

action set_ecmp_nexthop_details(ifindex, bd, nhop_index) {
    modify_field(ingress_metadata.egress_ifindex, ifindex);
    modify_field(i_fabric_header.egress_bd, bd);
    modify_field(i_fabric_header.nexthop_index, nhop_index);
    bit_xor(ingress_metadata.same_bd_check, ingress_metadata.bd, bd);
}





action set_nexthop_details_for_post_routed_flood(bd, uuc_mc_index) {
    modify_field(ig_intr_md_for_tm.mcast_grp_b, uuc_mc_index);
    modify_field(i_fabric_header.egress_bd, bd);
    bit_xor(ingress_metadata.same_bd_check, ingress_metadata.bd, bd);



}

action set_ecmp_nexthop_details_for_post_routed_flood(bd, uuc_mc_index,
                                                      nhop_index) {
    modify_field(ig_intr_md_for_tm.mcast_grp_b, uuc_mc_index);
    modify_field(i_fabric_header.egress_bd, bd);
    modify_field(i_fabric_header.nexthop_index, nhop_index);
    bit_xor(ingress_metadata.same_bd_check, ingress_metadata.bd, bd);



}

table nexthop {
    reads {
        i_fabric_header.nexthop_index : exact;
    }
    actions {
        nop;
        set_nexthop_details;
        set_nexthop_details_for_post_routed_flood;
    }
    size : 1024;
}

control process_nexthop {
    if (nexthop_metadata.nexthop_type == 1) {

        apply(ecmp_group);
    } else {

        apply(nexthop);
    }
}
# 92 "p4src/switch.p4" 2
# 1 "p4src/rewrite.p4" 1
action set_l2_rewrite() {
    modify_field(egress_metadata.routed, 0);
    modify_field(egress_metadata.bd, i_fabric_header.egress_bd);
}

action set_ipv4_unicast_rewrite(smac_idx, dmac) {
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(egress_metadata.routed, 1);
    add_to_field(ipv4.ttl, -1);
    modify_field(egress_metadata.bd, i_fabric_header.egress_bd);
}

action set_ipv6_unicast_rewrite(smac_idx, dmac) {
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(egress_metadata.routed, 1);
    add_to_field(ipv6.hopLimit, -1);
    modify_field(egress_metadata.bd, i_fabric_header.egress_bd);
}

action set_ipv4_vxlan_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 1);
}

action set_ipv6_vxlan_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 2);
}

action set_ipv4_geneve_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 3);
}

action set_ipv6_geneve_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 4);
}

action set_ipv4_nvgre_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 5);
}

action set_ipv6_nvgre_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 6);
}

action set_ipv4_erspan_v2_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 7);
}

action set_ipv6_erspan_v2_rewrite(outer_bd, tunnel_index, smac_idx, dmac) {
    modify_field(egress_metadata.bd, outer_bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, 8);
}
# 135 "p4src/rewrite.p4"
table rewrite {
    reads {
        i_fabric_header.nexthop_index : exact;
    }
    actions {
        nop;
        set_l2_rewrite;
        set_ipv4_unicast_rewrite;
# 175 "p4src/rewrite.p4"
    }
    size : 1024;
}

control process_rewrite {
    apply(rewrite);
}
# 93 "p4src/switch.p4" 2
# 1 "p4src/security.p4" 1



header_type security_metadata_t {
    fields {
        storm_control_color : 1;
        ipsg_enabled : 1;
    }
}

metadata security_metadata_t security_metadata;
# 38 "p4src/security.p4"
control process_storm_control {



}
# 75 "p4src/security.p4"
control process_ip_sourceguard {
# 86 "p4src/security.p4"
}
# 94 "p4src/switch.p4" 2
# 1 "p4src/fabric.p4" 1



metadata fabric_header_internal_t i_fabric_header;


action add_i_fabric_header(etherType) {






    modify_field(i_fabric_header.ingress_tunnel_type,
                 tunnel_metadata.ingress_tunnel_type);
    modify_field(i_fabric_header.lkp_mac_type, etherType);



}
# 135 "p4src/fabric.p4"
control process_fabric_ingress {



}
# 267 "p4src/fabric.p4"
control process_fabric_lag {



}

action cpu_tx_rewrite() {
    modify_field(ethernet.etherType, fabric_payload_header.etherType);
    remove_header(fabric_header);
    remove_header(fabric_header_cpu);
    remove_header(fabric_payload_header);
    modify_field(egress_metadata.fabric_bypass, 1);
}

action cpu_rx_rewrite() {
    add_header(fabric_header);
    add_header(fabric_header_cpu);
    modify_field(fabric_header.headerVersion, 0);
    modify_field(fabric_header.packetVersion, 0);
    modify_field(fabric_header.pad1, 0);
    modify_field(fabric_header.packetType, 6);
    modify_field(fabric_header_cpu.port, ig_intr_md.ingress_port);
    modify_field(egress_metadata.fabric_bypass, 1);



    add_header(fabric_payload_header);
    modify_field(fabric_payload_header.etherType, ethernet.etherType);
    modify_field(ethernet.etherType, 0x9000);

}







table fabric_rewrite {
    reads {
        eg_intr_md.egress_port : ternary;
        ig_intr_md.ingress_port : ternary;
    }
    actions {
        nop;
        cpu_tx_rewrite;
        cpu_rx_rewrite;



    }
    size : 512;
}

control process_fabric_egress {
    apply(fabric_rewrite);
}

action strip_i_fabric_header() {
    remove_header(i_fabric_header);
    remove_header(fabric_payload_header);
}

table egress_i_fabric_rewrite {
    reads {
        i_fabric_header : valid;
    }
    actions {
        strip_i_fabric_header;
    }
}

control process_i_fabric_egress {



}
# 95 "p4src/switch.p4" 2
# 1 "p4src/egress_filter.p4" 1
# 40 "p4src/egress_filter.p4"
control process_egress_filter {
# 51 "p4src/egress_filter.p4"
}
# 96 "p4src/switch.p4" 2

action nop() {
}

action on_miss() {
}

control ingress {

    if (valid(fabric_header)) {


        process_fabric_ingress();
    } else {


        validate_outer_ethernet_header();


        if (valid(ipv4)) {
            validate_outer_ipv4_header();
        } else {
            if (valid(ipv6)) {
                validate_outer_ipv6_header();
            }
        }

        if (valid(mpls[0])) {
            validate_mpls_header();
        }






         process_port_mapping();

         process_storm_control();


        process_port_vlan_mapping();
        process_spanning_tree();
        process_ip_sourceguard();
# 182 "p4src/switch.p4"
            process_validate_packet();


            process_mac();


            if (l3_metadata.lkp_ip_type == 0) {
                process_mac_acl();
            } else {
                process_ip_acl();
            }

            process_qos();

            apply(rmac) {
                rmac_miss {
                    process_multicast();
                }
                default {
                    if ((l3_metadata.lkp_ip_type == 1) and
                        (ipv4_metadata.ipv4_unicast_enabled == 1)) {

                        process_ipv4_racl();
                        process_nat();

                        process_ipv4_urpf();
                        process_ipv4_fib();

                    } else {
                        if ((l3_metadata.lkp_ip_type == 2) and
                            (ipv6_metadata.ipv6_unicast_enabled == 1)) {


                            process_ipv6_racl();
                            process_ipv6_urpf();
                            process_ipv6_fib();
                        }
                    }
                    process_urpf_bd();
                }
            }






        process_merge_results();


        process_nexthop();


        process_ingress_bd_stats();


        process_lag();


        process_mac_learning();
    }


    process_fabric_lag();


    process_system_acl();
}

control egress {

    if (egress_metadata.egress_bypass == 0) {


        process_fabric_egress();

        if (egress_metadata.fabric_bypass == 0) {

            process_replication();

            process_vlan_decap();


            process_tunnel_decap();


            process_egress_bd();

            process_egress_nat();


            process_rewrite();


            process_mac_rewrite();


            process_tunnel_encap();

            process_mtu();


            process_vlan_xlate();


            process_egress_filter();


            process_egress_acl();


            process_i_fabric_egress();
        }
    }
}
