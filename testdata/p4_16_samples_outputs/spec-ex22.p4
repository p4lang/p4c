control c() {
    action Forward_a(out bit<9> outputPort, bit<9> port) {
        outputPort = port;
    }
    apply {
    }
}

