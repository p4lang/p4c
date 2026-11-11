header H {
    bit<8> value;
}

header_union U {
    H first;
    H second;
}

struct Headers {
    U    src;
    U    dst;
    U[2] stack;
}

control Exercise(in bit<2> source, out bool ok) {
    @name("Exercise.hdr") Headers hdr_0;
    @name("Exercise.invalid") H invalid_0;
    apply {
        hdr_0.src.first.setInvalid();
        hdr_0.src.second.setInvalid();
        hdr_0.dst.first.setInvalid();
        hdr_0.dst.second.setInvalid();
        hdr_0.stack[0].first.setInvalid();
        hdr_0.stack[0].second.setInvalid();
        hdr_0.stack[1].first.setInvalid();
        hdr_0.stack[1].second.setInvalid();
        invalid_0.setInvalid();
        invalid_0.setInvalid();
        hdr_0.src.first.setInvalid();
        if (source == 2w0) {
            hdr_0.src.first.setValid();
            hdr_0.src.first.value = 8w0x11;
        } else if (source == 2w1) {
            hdr_0.src.second.setValid();
            hdr_0.src.second.value = 8w0x22;
        }
        hdr_0.dst.second.setValid();
        hdr_0.dst = hdr_0.src;
        hdr_0.dst = hdr_0.dst;
        ok = hdr_0.dst.first.isValid() == (source == 2w0) && hdr_0.dst.second.isValid() == (source == 2w1);
        if (hdr_0.dst.first.isValid()) {
            ok = ok && hdr_0.dst.first.value == 8w0x11;
        }
        if (hdr_0.dst.second.isValid()) {
            ok = ok && hdr_0.dst.second.value == 8w0x22;
        }
        hdr_0.dst.first.setValid();
        hdr_0.dst.second.setInvalid();
        ok = ok && !hdr_0.dst.first.isValid() && !hdr_0.dst.second.isValid();
        hdr_0.dst.first.setValid();
        hdr_0.dst.second = invalid_0;
        ok = ok && !hdr_0.dst.first.isValid() && !hdr_0.dst.second.isValid();
        hdr_0.dst.first.setValid();
        ok = ok && hdr_0.dst.first.isValid() && !hdr_0.dst.second.isValid();
        hdr_0.dst.first.setInvalid();
        ok = ok && !hdr_0.dst.first.isValid() && !hdr_0.dst.second.isValid();
        hdr_0.dst.first.setValid();
        hdr_0.dst.first.value = 8w0x55;
        hdr_0.dst.second = hdr_0.dst.first;
        ok = ok && !hdr_0.dst.first.isValid() && hdr_0.dst.second.isValid();
        if (hdr_0.dst.second.isValid()) {
            ok = ok && hdr_0.dst.second.value == 8w0x55;
        }
        hdr_0.stack[0] = hdr_0.src;
        hdr_0.stack[1].first.setValid();
        hdr_0.stack.push_front(1);
        ok = ok && !hdr_0.stack[0].first.isValid() && !hdr_0.stack[0].second.isValid() && hdr_0.stack[1].first.isValid() == (source == 2w0) && hdr_0.stack[1].second.isValid() == (source == 2w1);
        hdr_0.stack.pop_front(1);
        ok = ok && !hdr_0.stack[1].first.isValid() && !hdr_0.stack[1].second.isValid() && hdr_0.stack[0].first.isValid() == (source == 2w0) && hdr_0.stack[0].second.isValid() == (source == 2w1);
        if (hdr_0.stack[0].first.isValid()) {
            ok = ok && hdr_0.stack[0].first.value == 8w0x11;
        }
        if (hdr_0.stack[0].second.isValid()) {
            ok = ok && hdr_0.stack[0].second.value == 8w0x22;
        }
    }
}

control Test(in bit<2> source, out bool ok);
package Top(Test test);
Top(Exercise()) main;
