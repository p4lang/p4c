control c() {
    action Forward_a(bit<9> port, out bit<9> outputPort) {
        outputPort = port;
    }
    apply {
    }
}

