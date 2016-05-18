#include "/home/mbudiu/git/p4c/build/../p4include/core.p4"

struct Ports {
}

control Dep<OH>(packet_out b, in OH packetHeaders, in Ports ports);
