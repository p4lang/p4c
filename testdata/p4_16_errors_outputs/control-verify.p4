error {
    Oops
}

control C() {
    apply {
        verify(8w0 == 8w1, error.Oops);
    }
}

