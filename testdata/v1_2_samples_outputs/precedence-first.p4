struct s {
    bit<1> z;
    bit<1> w;
}

struct t {
    s s1;
    s s2;
}

typedef bit<1> Bit;
extern bit<1> fct(in bit<1> a, in bit<1> b);
action ac() {
    bit<1> a;
    bit<1> b;
    bit<1> c;
    bit<1> d;
    bool e;
    s f;
    t g;
    a = b + c + d;
    a = b + c + d;
    a = b + (c + d);
    a = b * c + d;
    a = b * (c + d);
    a = b + c * d;
    a = (b + c) * d;
    a = b * c * d;
    a = b * (c * d);
    a = b + c / d;
    a = (b + c) / d;
    a = b / c + d;
    a = b / (c + d);
    a = b / c / d;
    a = b / (c / d);
    a = b | c | d;
    a = b | c | d;
    a = b | (c | d);
    a = (e ? c : d);
    a = (e ? c + c : d + d);
    a = (e ? c + c : d + d);
    a = d + (e ? c + c : d);
    a = (e ? c + c : d) + d;
    a = (e ? (e ? b : c) : (e ? b : c));
    a = ((e ? (e ? b : c) : b) == b ? b : c);
    a = b & c | d;
    a = b | c & d;
    a = b & c | d;
    a = b & (c | d);
    a = (b | c) & d;
    a = b | c & d;
    e = e && e || e;
    e = e || e && e;
    e = e && e || e;
    e = e && (e || e);
    e = (e || e) && e;
    e = e == e == e;
    e = e == e == e;
    e = e == (e == e);
    e = e != e != e;
    e = e != e != e;
    e = e != (e != e);
    e = e == e != e;
    e = e == e != e;
    e = e == (e != e);
    e = e != e == e;
    e = e != e == e;
    e = e != (e == e);
    e = a < b == e;
    e = e == a < b;
    e = a < b == e;
    a = a << b << c;
    a = a << (b << c);
    a = a << b >> c;
    a = a << (b >> c);
    a = f.z + a;
    a = fct(a, b);
    a = fct(a + b, a + c);
    a = fct(a, fct(b, a) + c);
    a = fct(a + b * c, a * (b + c));
    a = (Bit)b;
    a = (Bit)b + c;
    a = (Bit)(b + c);
    a = (Bit)f.z + b;
    a = (Bit)fct((Bit)f.z + b, b + c);
    f = { a + b, c };
    g = { { a + b, c }, { a + b, c } };
    g = { { (Bit)(a + b) + b, c }, { a + (b + c), c } };
}
