struct s {
}

header h {
    s field;
}

parser p();
struct s1 {
    p field;
    _ field1;
}

header_union u {
    s      field;
    bit<1> field1;
}

