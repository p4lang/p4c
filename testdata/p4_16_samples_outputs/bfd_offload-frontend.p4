extern BFD_Offload {
    BFD_Offload(bit<16> size);
    void setTx(in bit<16> index, in bit<8> data);
    bit<8> getTx(in bit<16> index);
    abstract void on_rx(in bit<16> index);
    abstract bool on_tx(in bit<16> index);
}

BFD_Offload(16w32768) bfd_session_liveness_tracker = {
    void on_rx(in bit<16> index) {
        this.setTx(index, 8w0);
    }
    bool on_tx(in bit<16> index) {
        bit<8> tmp_1;
        bit<8> tmp_2;
        bit<8> c;
        tmp_1 = this.getTx(index);
        tmp_2 = tmp_1 + 8w1;
        c = tmp_2;
        if (c >= 8w4) 
            return true;
        else {
            this.setTx(index, c);
            return false;
        }
    }
};

