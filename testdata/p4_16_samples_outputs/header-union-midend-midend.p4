header H {
    bit<8> value;
}

header_union U {
    H first;
    H second;
}

struct Headers {
    H    src_first;
    H    src_second;
    H    dst_first;
    H    dst_second;
    U[2] stack;
}

control Exercise(in bit<2> source, out bool ok) {
    H tmp_0;
    H tmp_first;
    H tmp_second;
    H hdr_0_src_first;
    H hdr_0_src_second;
    H hdr_0_dst_first;
    H hdr_0_dst_second;
    U[2] hdr_0_stack;
    @name("Exercise.invalid") H invalid_0;
    @hidden action headerunionmidend27() {
        hdr_0_src_first.setValid();
        hdr_0_src_second.setInvalid();
        hdr_0_src_first.value = 8w0x11;
    }
    @hidden action headerunionmidend30() {
        hdr_0_src_second.setValid();
        hdr_0_src_first.setInvalid();
        hdr_0_src_second.value = 8w0x22;
    }
    @hidden action headerunionmidend21() {
        hdr_0_src_first.setInvalid();
        hdr_0_src_second.setInvalid();
        hdr_0_src_second.setInvalid();
        hdr_0_src_first.setInvalid();
        hdr_0_dst_first.setInvalid();
        hdr_0_dst_second.setInvalid();
        hdr_0_dst_second.setInvalid();
        hdr_0_dst_first.setInvalid();
        hdr_0_stack[0].first.setInvalid();
        hdr_0_stack[0].second.setInvalid();
        hdr_0_stack[1].first.setInvalid();
        hdr_0_stack[1].second.setInvalid();
        invalid_0.setInvalid();
        invalid_0.setInvalid();
        hdr_0_src_first.setInvalid();
        hdr_0_src_second.setInvalid();
    }
    @hidden action headerunionmidend36() {
        if (hdr_0_src_first.isValid()) {
            hdr_0_dst_first.setValid();
            hdr_0_dst_first = hdr_0_src_first;
            hdr_0_dst_second.setInvalid();
        } else {
            hdr_0_dst_first.setInvalid();
            hdr_0_dst_second.setInvalid();
        }
    }
    @hidden action headerunionmidend36_0() {
        if (hdr_0_src_second.isValid()) {
            hdr_0_dst_second.setValid();
            hdr_0_dst_second = hdr_0_src_second;
            hdr_0_dst_first.setInvalid();
        } else {
            hdr_0_dst_second.setInvalid();
            hdr_0_dst_first.setInvalid();
        }
    }
    @hidden action headerunionmidend36_1() {
        hdr_0_dst_first.setInvalid();
        hdr_0_dst_second.setInvalid();
    }
    @hidden action headerunionmidend35() {
        hdr_0_dst_second.setValid();
        hdr_0_dst_first.setInvalid();
    }
    @hidden action headerunionmidend37() {
        if (hdr_0_dst_first.isValid()) {
            tmp_first.setValid();
            tmp_first = hdr_0_dst_first;
            tmp_second.setInvalid();
        } else {
            tmp_first.setInvalid();
            tmp_second.setInvalid();
        }
    }
    @hidden action headerunionmidend37_0() {
        if (hdr_0_dst_second.isValid()) {
            tmp_second.setValid();
            tmp_second = hdr_0_dst_second;
            tmp_first.setInvalid();
        } else {
            tmp_second.setInvalid();
            tmp_first.setInvalid();
        }
    }
    @hidden action headerunionmidend37_1() {
        tmp_first.setInvalid();
        tmp_second.setInvalid();
    }
    @hidden action headerunionmidend37_2() {
        if (tmp_first.isValid()) {
            hdr_0_dst_first.setValid();
            hdr_0_dst_first = tmp_first;
            hdr_0_dst_second.setInvalid();
        } else {
            hdr_0_dst_first.setInvalid();
            hdr_0_dst_second.setInvalid();
        }
    }
    @hidden action headerunionmidend37_3() {
        if (tmp_second.isValid()) {
            hdr_0_dst_second.setValid();
            hdr_0_dst_second = tmp_second;
            hdr_0_dst_first.setInvalid();
        } else {
            hdr_0_dst_second.setInvalid();
            hdr_0_dst_first.setInvalid();
        }
    }
    @hidden action headerunionmidend37_4() {
        hdr_0_dst_first.setInvalid();
        hdr_0_dst_second.setInvalid();
    }
    @hidden action headerunionmidend41() {
        ok = ok && hdr_0_dst_first.value == 8w0x11;
    }
    @hidden action headerunionmidend38() {
        ok = hdr_0_dst_first.isValid() == (source == 2w0) && hdr_0_dst_second.isValid() == (source == 2w1);
    }
    @hidden action headerunionmidend44() {
        ok = ok && hdr_0_dst_second.value == 8w0x22;
    }
    @hidden action headerunionmidend71() {
        ok = ok && hdr_0_dst_second.value == 8w0x55;
    }
    @hidden action headerunionmidend48() {
        hdr_0_dst_first.setValid();
        hdr_0_dst_second.setInvalid();
        hdr_0_dst_second.setInvalid();
        hdr_0_dst_first.setInvalid();
        ok = ok && !hdr_0_dst_first.isValid() && !hdr_0_dst_second.isValid();
        hdr_0_dst_first.setValid();
        hdr_0_dst_second.setInvalid();
        if (invalid_0.isValid()) {
            hdr_0_dst_second.setValid();
            hdr_0_dst_second = invalid_0;
            hdr_0_dst_first.setInvalid();
        } else {
            hdr_0_dst_second.setInvalid();
            hdr_0_dst_first.setInvalid();
        }
        ok = ok && !hdr_0_dst_first.isValid() && !hdr_0_dst_second.isValid();
        hdr_0_dst_first.setValid();
        hdr_0_dst_second.setInvalid();
        ok = ok && hdr_0_dst_first.isValid() && !hdr_0_dst_second.isValid();
        hdr_0_dst_first.setInvalid();
        hdr_0_dst_second.setInvalid();
        ok = ok && !hdr_0_dst_first.isValid() && !hdr_0_dst_second.isValid();
        hdr_0_dst_first.setValid();
        hdr_0_dst_second.setInvalid();
        hdr_0_dst_first.value = 8w0x55;
        tmp_0 = hdr_0_dst_first;
        if (tmp_0.isValid()) {
            hdr_0_dst_second.setValid();
            hdr_0_dst_second = tmp_0;
            hdr_0_dst_first.setInvalid();
        } else {
            hdr_0_dst_second.setInvalid();
            hdr_0_dst_first.setInvalid();
        }
        ok = ok && !hdr_0_dst_first.isValid() && hdr_0_dst_second.isValid();
    }
    @hidden action headerunionmidend75() {
        hdr_0_stack[0].first = hdr_0_src_first;
    }
    @hidden action headerunionmidend75_0() {
        hdr_0_stack[0].second = hdr_0_src_second;
    }
    @hidden action headerunionmidend75_1() {
        hdr_0_stack[0].first.setInvalid();
    }
    @hidden action headerunionmidend86() {
        ok = ok && hdr_0_stack[0].first.value == 8w0x11;
    }
    @hidden action headerunionmidend76() {
        hdr_0_stack[1].first.setValid();
        hdr_0_stack.push_front(1);
        ok = ok && !hdr_0_stack[0].first.isValid() && !hdr_0_stack[0].second.isValid() && hdr_0_stack[1].first.isValid() == (source == 2w0) && hdr_0_stack[1].second.isValid() == (source == 2w1);
        hdr_0_stack.pop_front(1);
        ok = ok && !hdr_0_stack[1].first.isValid() && !hdr_0_stack[1].second.isValid() && hdr_0_stack[0].first.isValid() == (source == 2w0) && hdr_0_stack[0].second.isValid() == (source == 2w1);
    }
    @hidden action headerunionmidend89() {
        ok = ok && hdr_0_stack[0].second.value == 8w0x22;
    }
    @hidden table tbl_headerunionmidend21 {
        actions = {
            headerunionmidend21();
        }
        const default_action = headerunionmidend21();
    }
    @hidden table tbl_headerunionmidend27 {
        actions = {
            headerunionmidend27();
        }
        const default_action = headerunionmidend27();
    }
    @hidden table tbl_headerunionmidend30 {
        actions = {
            headerunionmidend30();
        }
        const default_action = headerunionmidend30();
    }
    @hidden table tbl_headerunionmidend35 {
        actions = {
            headerunionmidend35();
        }
        const default_action = headerunionmidend35();
    }
    @hidden table tbl_headerunionmidend36 {
        actions = {
            headerunionmidend36();
        }
        const default_action = headerunionmidend36();
    }
    @hidden table tbl_headerunionmidend36_0 {
        actions = {
            headerunionmidend36_0();
        }
        const default_action = headerunionmidend36_0();
    }
    @hidden table tbl_headerunionmidend36_1 {
        actions = {
            headerunionmidend36_1();
        }
        const default_action = headerunionmidend36_1();
    }
    @hidden table tbl_headerunionmidend37 {
        actions = {
            headerunionmidend37();
        }
        const default_action = headerunionmidend37();
    }
    @hidden table tbl_headerunionmidend37_0 {
        actions = {
            headerunionmidend37_0();
        }
        const default_action = headerunionmidend37_0();
    }
    @hidden table tbl_headerunionmidend37_1 {
        actions = {
            headerunionmidend37_1();
        }
        const default_action = headerunionmidend37_1();
    }
    @hidden table tbl_headerunionmidend37_2 {
        actions = {
            headerunionmidend37_2();
        }
        const default_action = headerunionmidend37_2();
    }
    @hidden table tbl_headerunionmidend37_3 {
        actions = {
            headerunionmidend37_3();
        }
        const default_action = headerunionmidend37_3();
    }
    @hidden table tbl_headerunionmidend37_4 {
        actions = {
            headerunionmidend37_4();
        }
        const default_action = headerunionmidend37_4();
    }
    @hidden table tbl_headerunionmidend38 {
        actions = {
            headerunionmidend38();
        }
        const default_action = headerunionmidend38();
    }
    @hidden table tbl_headerunionmidend41 {
        actions = {
            headerunionmidend41();
        }
        const default_action = headerunionmidend41();
    }
    @hidden table tbl_headerunionmidend44 {
        actions = {
            headerunionmidend44();
        }
        const default_action = headerunionmidend44();
    }
    @hidden table tbl_headerunionmidend48 {
        actions = {
            headerunionmidend48();
        }
        const default_action = headerunionmidend48();
    }
    @hidden table tbl_headerunionmidend71 {
        actions = {
            headerunionmidend71();
        }
        const default_action = headerunionmidend71();
    }
    @hidden table tbl_headerunionmidend75 {
        actions = {
            headerunionmidend75();
        }
        const default_action = headerunionmidend75();
    }
    @hidden table tbl_headerunionmidend75_0 {
        actions = {
            headerunionmidend75_0();
        }
        const default_action = headerunionmidend75_0();
    }
    @hidden table tbl_headerunionmidend75_1 {
        actions = {
            headerunionmidend75_1();
        }
        const default_action = headerunionmidend75_1();
    }
    @hidden table tbl_headerunionmidend76 {
        actions = {
            headerunionmidend76();
        }
        const default_action = headerunionmidend76();
    }
    @hidden table tbl_headerunionmidend86 {
        actions = {
            headerunionmidend86();
        }
        const default_action = headerunionmidend86();
    }
    @hidden table tbl_headerunionmidend89 {
        actions = {
            headerunionmidend89();
        }
        const default_action = headerunionmidend89();
    }
    apply {
        tbl_headerunionmidend21.apply();
        if (source == 2w0) {
            tbl_headerunionmidend27.apply();
        } else if (source == 2w1) {
            tbl_headerunionmidend30.apply();
        }
        tbl_headerunionmidend35.apply();
        if (hdr_0_src_first.isValid()) {
            tbl_headerunionmidend36.apply();
        } else if (hdr_0_src_second.isValid()) {
            tbl_headerunionmidend36_0.apply();
        } else {
            tbl_headerunionmidend36_1.apply();
        }
        if (hdr_0_dst_first.isValid()) {
            tbl_headerunionmidend37.apply();
        } else if (hdr_0_dst_second.isValid()) {
            tbl_headerunionmidend37_0.apply();
        } else {
            tbl_headerunionmidend37_1.apply();
        }
        if (tmp_first.isValid()) {
            tbl_headerunionmidend37_2.apply();
        } else if (tmp_second.isValid()) {
            tbl_headerunionmidend37_3.apply();
        } else {
            tbl_headerunionmidend37_4.apply();
        }
        tbl_headerunionmidend38.apply();
        if (hdr_0_dst_first.isValid()) {
            tbl_headerunionmidend41.apply();
        }
        if (hdr_0_dst_second.isValid()) {
            tbl_headerunionmidend44.apply();
        }
        tbl_headerunionmidend48.apply();
        if (hdr_0_dst_second.isValid()) {
            tbl_headerunionmidend71.apply();
        }
        if (hdr_0_src_first.isValid()) {
            tbl_headerunionmidend75.apply();
        } else if (hdr_0_src_second.isValid()) {
            tbl_headerunionmidend75_0.apply();
        } else {
            tbl_headerunionmidend75_1.apply();
        }
        tbl_headerunionmidend76.apply();
        if (hdr_0_stack[0].first.isValid()) {
            tbl_headerunionmidend86.apply();
        }
        if (hdr_0_stack[0].second.isValid()) {
            tbl_headerunionmidend89.apply();
        }
    }
}

control Test(in bit<2> source, out bool ok);
package Top(Test test);
Top(Exercise()) main;
