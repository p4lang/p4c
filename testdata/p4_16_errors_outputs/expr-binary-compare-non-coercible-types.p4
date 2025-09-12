bit<32> min(in bit<32> a, in bit<32> b) {
    return (a > { false, {#} } ? b : a);
}
