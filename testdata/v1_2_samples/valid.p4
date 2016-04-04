header h {
    bit<32> f;
}

control c(inout h hdr)
{
    action a() {
        hdr.setValid(false);
    }

    apply {}
}
