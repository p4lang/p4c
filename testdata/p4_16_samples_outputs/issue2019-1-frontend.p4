control C<H>(inout H hdr);
control V<H>(inout H hdr);
struct E {
}

control Some(inout E m) {
    apply {
    }
}

control EmptyV<H>(inout H hdr) {
    apply {
    }
}

package S<H1>(C<H1> s, V<H1> vr=EmptyV<H1>());
control EmptyV_0(inout E hdr) {
    apply {
    }
}

S<E>(s = Some(), vr = EmptyV_0()) main;
