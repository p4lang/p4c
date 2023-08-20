header yy {
    bit<1> tt;
}

struct y {
    yy[2] tt;
}

header t<tt> {
    tt y;
}

typedef t<y> tty;
