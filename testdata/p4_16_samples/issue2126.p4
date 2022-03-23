parser p() {
    state start {
        {
            bit<16> retval = 0;
        }
        {
            bit<16> retval = 1;
        }
        transition accept;
    }
}
