#include <core.p4>
#define UBPF_MODEL_VERSION 20200515
#include <ubpf_model.p4>

#include "ebpf_headers.p4"

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
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

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action Reject(IPv4Address add)
    {
        mark_to_drop();
        headers.ipv4.srcAddr = add;
    }

    table Check_src_ip {
        key = { headers.ipv4.srcAddr : lpm;
                headers.ipv4.protocol: exact;}
        actions =
        {
            Reject;
            NoAction;
        }

        default_action = Reject(0);
    }


    apply
    {
        if (!headers.ipv4.isValid())
        {
            headers.ipv4.setInvalid();
            headers.ipv4.setValid();
            mark_to_drop();
            return;
        }

        Check_src_ip.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;