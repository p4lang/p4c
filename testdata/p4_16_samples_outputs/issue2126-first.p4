parser p() {
    state start {
        {
            bit<16> retval = 16w0;
        }
        {
            bit<16> retval = 16w1;
        }
        transition accept;
    }
}

