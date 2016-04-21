control p() {
    apply {
        bit<32> b;
        b = 32w0 & _;
        b = 32w0 + _;
        b = (true ? 32w0 : _);
    }
}

