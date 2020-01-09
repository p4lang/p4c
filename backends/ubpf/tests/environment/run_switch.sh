#! /bin/sh
# This script runs p4rt-ovs switch.
# It has to been done after every machine reboot.

sudo ovs-appctl -t ovsdb-server exit
sudo ovs-appctl -t ovs-vswitchd exit
sudo ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock     --remote=db:Open_vSwitch,Open_vSwitch,manager_options     --pidfile --detach
sudo ovs-vsctl --no-wait init
export DB_SOCK=/usr/local/var/run/openvswitch/db.sock
sudo ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
sudo ovs-vswitchd unix: --pidfile --detach --log=oko.log
