# Steps to run tests

Tests use two VMs:
- switch - on this VM we run PTF tests 
- generator - expose two interfaces to P4rt-OVS switch installed on switch VM

1. Install and configure environment for tests
    ```bash
    $ cd environment
    $ vagrant up
    ```
2. Run PTF agent on generator machine
    ```bash
    $ vagrant ssh generator
    $ cd ptf/ptf_nn/
    $ sudo python ptf_nn_agent.py --device-socket 0@tcp://192.168.100.20:10001 -i 0-1@enp0s8 -i 0-2@enp0s9 -v
    ```
3. Compile P4 programs on switch machine
    ```bash
    $ vagrant ssh switch
    $ cd p4c/backends/ubpf/tests
    $ sudo python compile_testdata.py
    ```
4. Run tests on switch machine
    ```bash
    $ vagrant ssh switch
    $ cd p4c/backends/ubpf/tests
    $ sudo ptf --failfast --test-dir ptf/ --device-socket 0-{1-2}@tcp://192.168.100.20:10001 --platform nn
    ```
    **Important**  
    In case of tests errors please check if p4rt-switch on switch machine works properly (ie. `ps aux`).
    ```bash
    $ cd /vagrant
    $ ./configure_dpdk.sh #This script configures dpdk interfaces that p4rt-ovs switch use
    $ ./run_switch.sh # This script runs p4rt-switch
    ```
    