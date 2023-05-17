struct Version {
    bit<8> major;
    bit<8> minor;
}

const .Version version = (Version){major = 8w0,minor = 8w1};
