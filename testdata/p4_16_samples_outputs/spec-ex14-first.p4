const bit<32> COUNTER = 32w0x0;
struct Version {
    bit<32> major;
    bit<32> minor;
}

const Version version = (Version){major = 32w0,minor = 32w0};
