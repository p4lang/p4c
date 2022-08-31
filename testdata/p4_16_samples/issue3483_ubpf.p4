#include <ubpf_model.p4>

#include "ebpf_headers.p4"

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct Meta {
    bit b;
}

parser prs(packet_in p, out Headers_t headers, inout Meta meta, inout standard_metadata std_meta)
{
    state start
    {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType)
        {
            16w0x800 : ip;
            default : reject;
        }
    }

    state ip
    {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout Meta meta, inout standard_metadata unused)
{
    action Reject(IPv4Address add)
    {
        headers.ipv4.srcAddr = add;
    }

    table Check_src_ip {
        key = { headers.ipv4.srcAddr : exact; }
        actions =
        {
            Reject;
            NoAction;
        }

        default_action = NoAction;
    }

    apply {
        if (!Check_src_ip.apply().hit) {
            meta.b = 1;
        }

        switch (Check_src_ip.apply().action_run) {
        Reject: {
            meta.b = 0;
        }
        NoAction: {}
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}


ubpf(prs(), pipe(), dprs()) main;
