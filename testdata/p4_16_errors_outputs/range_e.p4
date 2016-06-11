parser p() {
    state start {
        transition select(2w0) {
            2w2 .. 3w3: reject;
        }
    }
}

