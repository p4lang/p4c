@pragma deprecated "Please use verify_checksum/update_checksum instead."
extern Checksum16 {
    Checksum16();
}

@pragma deprecated "Please don't use this function."
extern bit<6> wrong();

Checksum16() instance;

control c() {
    apply {
        bit<6> x = wrong();
    }
}
