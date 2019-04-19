/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


extern BFD_Offload {
    BFD_Offload(bit<16> size);

    // State manipulation
    void setTx(in bit<16> index, in bit<8> data);
    bit<8> getTx(in bit<16> index);

    abstract void on_rx( in bit<16> index );
    abstract bool on_tx( in bit<16> index );
}

BFD_Offload(32768) bfd_session_liveness_tracker = {
    void on_rx( in bit<16> index ) {
        this.setTx(index, 0);
    }
    bool on_tx( in bit<16> index ) {
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
        bfd_session_liveness_tracker.on_rx( index );
    }
}
control for_tx_bfd_packets() {
    apply {
        bit<16> index;
        bfd_session_liveness_tracker.on_tx( index );
    }
}
