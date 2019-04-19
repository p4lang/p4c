extern X<T> {
}

parser p() {
    X() x;
    state start {
        transition accept;
    }
}

