#include "test.h"

/*
 * This file implements sample C extern function. It contains definition of the following C extern function:
 *
 * bool tcp_conntrack(in Headers_t hdrs)
 *
 * The implementation shows how to use BPF maps in the C extern function to perform stateful packet processing.
 */

#define MAX_ENTRIES 10000

struct connection_id_struct {
    u32 saddr;
    u32 daddr;
    u16 source;
    u16 dest;
};

enum state {
    SYNSENT = 1,
    SYNACKED,
    ESTABLISHED
};

struct connInfo {
    enum state state;
    u32 server;
};

REGISTER_START()
REGISTER_TABLE(tcp_reg, BPF_MAP_TYPE_HASH, sizeof(u32), sizeof(struct connInfo), MAX_ENTRIES)
REGISTER_END()

static inline u8 tcp_conntrack(struct Headers_t hdrs)
{
    u32 saddr = hdrs.ipv4.srcAddr;
    u32 daddr = hdrs.ipv4.dstAddr;
    u16 sport = hdrs.tcp.srcPort;
    u16 dport = hdrs.tcp.dstPort;

    struct connection_id_struct conn;

    if (saddr < daddr) {
        conn.saddr = saddr;
        conn.daddr = daddr;
    } else {
        conn.saddr = daddr;
        conn.daddr = saddr;
    }
    if (sport < dport) {
        conn.source = hdrs.tcp.srcPort;
        conn.dest = hdrs.tcp.dstPort;
    } else {
        conn.source = hdrs.tcp.dstPort;
        conn.dest = hdrs.tcp.srcPort;
    }

    struct connInfo *info = BPF_MAP_LOOKUP_ELEM(tcp_reg, &conn);
    if(!info) {
        if (hdrs.tcp.syn == 1 && hdrs.tcp.ack == 0) {
            // It's a SYN
            struct connInfo new_info = {0};
            new_info.state = SYNSENT;
            new_info.server = hdrs.ipv4.dstAddr;
            BPF_MAP_UPDATE_ELEM(tcp_reg, &conn, &new_info, 0);
            return 1;
        }
        return 0;
    } else if (hdrs.ipv4.srcAddr == info->server) {
        switch (info->state) {
            case SYNSENT: // SYN Sent Awaiting SYN+ACK
                if (hdrs.tcp.syn == 1 && hdrs.tcp.ack == 1) {
                    // It's a SYN+ACK
                    info->state = SYNACKED;
                    return 1;
                }
                return 0;
            case SYNACKED: // SYN+ACK Sent Awaiting ACK
                return 0;
            case ESTABLISHED: // Connection established, awaiting FIN
                return 1;
        }
    } else {
        switch (info->state) {
            case SYNSENT: // SYN Sent Awaiting SYN+ACK
                return 0;
            case SYNACKED: // SYN+ACK Sent Awaiting ACK
                if (hdrs.tcp.syn == 0 && hdrs.tcp.ack == 1) {
                    // It's a ACK
                    info->state = ESTABLISHED;
                    return 1;
                }
                return 0;
            case ESTABLISHED: // Connection established, awaiting FIN
                if (hdrs.tcp.syn == 1 && hdrs.tcp.ack == 0) {
                    // Connection already established, rejecting
                    return 0;
                }
                if (hdrs.tcp.fin == 1 && hdrs.tcp.ack == 1) {
                    // It's a FIN+ACK
                    BPF_MAP_DELETE_ELEM(tcp_reg, &conn);
                }
                return 1;
        }
    }
    return 0;
}
