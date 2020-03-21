header h_t {
    bit<8> f;
}

struct my_packet {
    h_t h;
}

control c() {
    apply {
<<<<<<< HEAD
        h_t h = h_t {f = 8w0};
        h_t h1 = h;
        h_t h3 = h_t {f = h.f};
=======
        h_t h = (h_t){f = 8w0};
        h_t h1 = h;
        h_t h3 = (h_t){f = h.f};
>>>>>>> Change struct-valued expression to use type-cast syntax
    }
}

