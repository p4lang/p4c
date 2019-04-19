header_type stack_t {
    fields {
        f : 16;
    }
}

header stack_t stack[4];

parser start {
    extract(stack[0]);
    return select(latest.f) {
        0xffff : p1;
        default: p2;
    }
}

parser p1 {
    extract(stack[1]);
    extract(stack[2]);
    return ingress;
}

parser p2 {
    extract(stack[2]);
    extract(stack[1]);
    return ingress;
}

control ingress { }
