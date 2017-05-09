header h_t {
    bit<8> f;
}

struct my_packet {
    h_t h;
}

control c() {
    apply {
        h_t h = { 0 };
        h_t h2 = (h_t)h;
        h_t h4 = (h_t){ h.f };
    }
}

