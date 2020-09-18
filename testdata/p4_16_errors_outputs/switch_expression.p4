const bit<8> NumConstant = 0x55;
const bit<8> Another = 0xee;
header headers {
    bit<8> field;
}

control ctrl(in headers hdr) {
    apply {
        switch (hdr.field) {
            NumConstant: {
            }
        }

    }
}

