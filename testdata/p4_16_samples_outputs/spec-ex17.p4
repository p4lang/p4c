#include "/home/cdodd/p4c/p4include/core.p4"

struct Counters {
}

parser P<IH>(packet_in b, out IH packetHeaders, out Counters counters);
