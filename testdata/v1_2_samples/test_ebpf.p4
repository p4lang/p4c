#include "ebpf_model.p4"
#include "core.p4"

#include "ebpf_headers.p4"

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers)
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

control pipe(inout Headers_t headers, out bool pass)
{
    action Reject(IPv4Address add)
    {
        pass = false;
        headers.ipv4.srcAddr = add;
    }

    table Check_src_ip()
    {
        key = { headers.ipv4.srcAddr : exact; }
        actions =
        {
            Reject;
            NoAction;
        }

        implementation = hash_table(1024);
        const default_action = NoAction;
    }

    apply {
        pass = true;
        
        if (!headers.ipv4.isValid())
        {
            pass = false;
            return;
        }
        
        Check_src_ip.apply();
    }
}

ebpfFilter(prs(), pipe()) main;
