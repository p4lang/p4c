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
    Headers hdr;
    H invalid;
    apply {
        invalid.setInvalid();
        hdr.src.first.setInvalid();
        if (source == 0) {
            hdr.src.first.setValid();
            hdr.src.first.value = 0x11;
        } else if (source == 1) {
            hdr.src.second.setValid();
            hdr.src.second.value = 0x22;
        }
        hdr.dst.second.setValid();
        hdr.dst = hdr.src;
        hdr.dst = hdr.dst;
        ok = hdr.dst.first.isValid() == (source == 0) && hdr.dst.second.isValid() == (source == 1);
        if (hdr.dst.first.isValid()) {
            ok = ok && hdr.dst.first.value == 0x11;
        }
        if (hdr.dst.second.isValid()) {
            ok = ok && hdr.dst.second.value == 0x22;
        }
        hdr.dst.first.setValid();
        hdr.dst.second.setInvalid();
        ok = ok && !hdr.dst.first.isValid() && !hdr.dst.second.isValid();
        hdr.dst.first.setValid();
        hdr.dst.second = invalid;
        ok = ok && !hdr.dst.first.isValid() && !hdr.dst.second.isValid();
        hdr.dst.first.setValid();
        hdr.dst.second.value = 0x33;
        ok = ok && hdr.dst.first.isValid() && !hdr.dst.second.isValid();
        hdr.dst.first.setInvalid();
        hdr.dst.second.value = 0x44;
        ok = ok && !hdr.dst.first.isValid() && !hdr.dst.second.isValid();
        hdr.dst.first.setValid();
        hdr.dst.first.value = 0x55;
        hdr.dst.second = hdr.dst.first;
        ok = ok && !hdr.dst.first.isValid() && hdr.dst.second.isValid();
        if (hdr.dst.second.isValid()) {
            ok = ok && hdr.dst.second.value == 0x55;
        }
        hdr.stack[0] = hdr.src;
        hdr.stack[1].first.setValid();
        hdr.stack.push_front(1);
        ok = ok && !hdr.stack[0].first.isValid() && !hdr.stack[0].second.isValid() && hdr.stack[1].first.isValid() == (source == 0) && hdr.stack[1].second.isValid() == (source == 1);
        hdr.stack.pop_front(1);
        ok = ok && !hdr.stack[1].first.isValid() && !hdr.stack[1].second.isValid() && hdr.stack[0].first.isValid() == (source == 0) && hdr.stack[0].second.isValid() == (source == 1);
        if (hdr.stack[0].first.isValid()) {
            ok = ok && hdr.stack[0].first.value == 0x11;
        }
        if (hdr.stack[0].second.isValid()) {
            ok = ok && hdr.stack[0].second.value == 0x22;
        }
    }
}

control Test(in bit<2> source, out bool ok);
package Top(Test test);
Top(Exercise()) main;
