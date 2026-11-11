// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

header H {
    bit<8> value;
}

header_union U {
    H first;
    H second;
}

struct Headers {
    U src;
    U dst;
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

        // A copy must preserve either valid member and invalidate an old destination.
        hdr.dst.second.setValid();
        hdr.dst = hdr.src;
        hdr.dst = hdr.dst;
        ok = hdr.dst.first.isValid() == (source == 0) &&
             hdr.dst.second.isValid() == (source == 1);
        if (hdr.dst.first.isValid()) {
            ok = ok && hdr.dst.first.value == 0x11;
        }
        if (hdr.dst.second.isValid()) {
            ok = ok && hdr.dst.second.value == 0x22;
        }

        // Invalidating an inactive member still invalidates the whole union.
        hdr.dst.first.setValid();
        hdr.dst.second.setInvalid();
        ok = ok && !hdr.dst.first.isValid() && !hdr.dst.second.isValid();

        // Assigning an invalid header to a member has the same effect.
        hdr.dst.first.setValid();
        hdr.dst.second = invalid;
        ok = ok && !hdr.dst.first.isValid() && !hdr.dst.second.isValid();

        // Field writes do not change the validity of either member.
        hdr.dst.first.setValid();
        hdr.dst.second.value = 0x33;
        ok = ok && hdr.dst.first.isValid() && !hdr.dst.second.isValid();
        hdr.dst.first.setInvalid();
        hdr.dst.second.value = 0x44;
        ok = ok && !hdr.dst.first.isValid() && !hdr.dst.second.isValid();

        // Copying between siblings must save the source before changing validity.
        hdr.dst.first.setValid();
        hdr.dst.first.value = 0x55;
        hdr.dst.second = hdr.dst.first;
        ok = ok && !hdr.dst.first.isValid() && hdr.dst.second.isValid();
        if (hdr.dst.second.isValid()) {
            ok = ok && hdr.dst.second.value == 0x55;
        }

        // Stack shifts must use whole-union copy semantics, including invalid sources.
        hdr.stack[0] = hdr.src;
        hdr.stack[1].first.setValid();
        hdr.stack.push_front(1);
        ok = ok && !hdr.stack[0].first.isValid() && !hdr.stack[0].second.isValid() &&
             hdr.stack[1].first.isValid() == (source == 0) &&
             hdr.stack[1].second.isValid() == (source == 1);
        hdr.stack.pop_front(1);
        ok = ok && !hdr.stack[1].first.isValid() && !hdr.stack[1].second.isValid() &&
             hdr.stack[0].first.isValid() == (source == 0) &&
             hdr.stack[0].second.isValid() == (source == 1);
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
