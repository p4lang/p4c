extern BFD_Offload {
    BFD_Offload(bit<16> size);
    void setTx(in bit<16> index, in bit<8> data);
    bit<8> getTx(in bit<16> index);
    abstract void on_rx(in bit<16> index);
    abstract bool on_tx(in bit<16> index);
}

BFD_Offload(32768) bfd_session_liveness_tracker = {
    void on_rx(in bit<16> index) {
        this.setTx(index, 0);
    }
    bool on_tx(in bit<16> index) {
        bit<8> c = this.getTx(index) + 1;
        if (c >= 4) {
            return true;
        } else {
            this.setTx(index, c);
            return false;
        }
    }
};

control for_rx_bfd_packets() {
    apply {
        bit<16> index;
        bfd_session_liveness_tracker.on_rx(index);
    }
}

control for_tx_bfd_packets() {
    apply {
        bit<16> index;
        bfd_session_liveness_tracker.on_tx(index);
    }
}

