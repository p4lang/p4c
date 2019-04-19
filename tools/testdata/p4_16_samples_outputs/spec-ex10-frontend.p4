struct Ethernet_h {
}

struct Ip_h {
}

header Tcp_h {
}

header Udp_h {
}

header_union L4_h {
    Tcp_h tcp;
    Udp_h udp;
}

struct Parsed_headers {
    Ethernet_h ethernet;
    Ip_h       ip;
    L4_h       tcpudp;
}

