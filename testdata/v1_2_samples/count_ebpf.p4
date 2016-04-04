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
    CounterArray(32w10, true) counters;

    apply {
        if (headers.ipv4.valid)
        {
            counters.increment((bit<32>)headers.ipv4.dstAddr);
            pass = true;
        }
        else
            pass = false;
    }
}

ebpfFilter(prs(), pipe()) main;
