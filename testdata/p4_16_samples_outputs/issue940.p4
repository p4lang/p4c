extern Checksum16 {
    Checksum16();
}

extern bit<6> wrong();
Checksum16() instance;
control c() {
    apply {
        bit<6> x = wrong();
    }
}

