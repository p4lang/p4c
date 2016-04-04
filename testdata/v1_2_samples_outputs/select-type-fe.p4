parser p() {
    state start {
        transition select(32w0) {
            32w5: reject;
            default: reject;
        }
    }
}

