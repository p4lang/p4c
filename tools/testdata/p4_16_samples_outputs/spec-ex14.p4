const bit<32> COUNTER = 32w0x0;
struct Version {
    bit<32> major;
    bit<32> minor;
}

const Version version = { 32w0, 32w0 };
