parser p() {
    state start {
        transition select(32w0) {
            5: reject;
            default: reject;
        }
    }
}

