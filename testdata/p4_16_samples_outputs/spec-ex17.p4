#include "/home/mbudiu/git/p4c/build/../p4include/core.p4"

struct Counters {
}

parser P<IH>(packet_in b, out IH packetHeaders, out Counters counters);
