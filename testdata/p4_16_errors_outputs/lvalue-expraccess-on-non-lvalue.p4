header ethernet_t {
}

struct Headers {
    ethernet_t eth_hdr;
}

void f(Headers h) {
    h.eth_hdr = 0;
}
