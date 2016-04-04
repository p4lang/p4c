struct bs {
}

parser p(in bs b, out bool matches) {
    state start {
        transition next;
    }
    state next {
        transition accept;
    }
}

