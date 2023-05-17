struct Header<St> {
    St data;
    bit valid;
}

struct S {
    bit<32> b;
}

struct U {
    Header<S> f;
}

const U u = { f = { data = { b = 10 }, valid = 1 } };

struct H2<G> {
    Header<G> g;
    bit invalid;
}

struct H4<T> {
    T x;
}

const Header<S> h2 = (Header<S>){
    data = { b = 0 },
    valid = 0
};

const Header<bit<16>> bz = {
    data = 16,
    valid = 1
};

const Header<S> h = {
    data = { b = 0 },
    valid = 0
};

const H2<S> h1 = (H2<S>){
    g = (Header<S>){ data = { b = 0 }, valid = 1 },
    invalid = 1
};

const H2<S> h3 = {
    g = { data = { b = 0 }, valid = 1 },
    invalid = 1
};

typedef H2<S> R;

struct H3<T> {
    R r;
    T s;
    H2<T> h2;
    H4<H2<T>> h3;
    tuple<T, T> t;
}

header GH<T> {
    T data;
}

header X {
    bit<32> b;
}

const GH<bit<32>> g = { data = 0 };
const GH<S> g1 = { data = { b = 0 } };

typedef GH<S>[3] Stack;

const H3<S> h4 = {
    r = { g = { data = { b = 10 }, valid = 0 }, invalid = 1 },
    s = { b = 20 },
    h2 = { g = { data = { b = 0 }, valid = 1 }, invalid = 1 },
    h3 = { x = { g = { data = { b = 0 }, valid = 1 }, invalid = 1 } },
    t = { { b = 0 }, { b = 1 } }
};

header_union HU<T> {
    X xu;
    GH<T> h3u;
}

control c(out bit x) {
    apply {
        H3<S> h5;
        H4<H2<S>> n;
        GH<S> gh;
        bool b = gh.isValid();
        Stack s;
        s[0] = { data = { b = 1 }};
        b = b && s[0].isValid();
        X xinst = { b = 2 };
        HU<bit<32>> z;
        z.xu = xinst;
        z.h3u = { data = 1 };
        x = u.f.valid & h1.g.valid & h.valid & h2.valid & h1.g.valid & h3.g.valid & n.x.g.valid & (bit)b;
    }
}

control ctrl(out bit x);
package top(ctrl _c);
top(c()) main;
