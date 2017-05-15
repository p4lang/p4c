header h_t {
    bit<8> f;
}

struct my_packet {
    h_t h;
}

control c() {
    apply {
        h_t h = { 0 };
        h_t h1 = h;
        h_t h3 = { h.f };
    }
}

