struct header_t {
}

control MyC(inout header_t h) {
    apply {
    }
}

package Pipeline<Ctrl>(Ctrl c);
Pipeline<bit<8>>(MyC()) main;
