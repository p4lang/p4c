struct Ethernet_h {
}

struct Ip_h {
}

header Tcp_h {
}

header Udp_h {
}

struct Parsed_headers {
    Ethernet_h ethernet;
    Ip_h       ip;
    Tcp_h      tcpudp_tcp;
    Udp_h      tcpudp_udp;
}

