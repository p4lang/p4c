# Steps to run tests

Tests use two VMs:
- `switch` - on this VM we run PTF tests 
- `generator` - expose two interfaces to P4rt-OVS switch installed on switch VM

**Note.** As P4rt-OVS (the test uBPF target) is built on top of DPDK the tests require to be run in virtual 
environment with two VMs (`generator` + `switch`). 
 
0. Install `Virtualbox` and `Vagrant` on you machine:

    `sudo apt install -y virtualbox vagrant`

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
    
    After any system's reboot the below scripts have to be run again:
    
    ```bash
    $ cd /vagrant
    $ ./configure_dpdk.sh #This script configures dpdk interfaces that p4rt-ovs switch use
    $ ./run_switch.sh # This script runs p4rt-switch
    ```
    
    In case of tests errors please check if the `p4rt-ovs` switch is up and running (ie. `ps aux`). 
    