header_union HU {
}

parser p(out bool hu) {
    state start {
        hu = (HU){#};
    }
}

