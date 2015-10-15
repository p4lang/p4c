# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from mininet.net import Mininet
from mininet.node import Switch, Host
from mininet.log import setLogLevel, info

class P4Host(Host):
    def config(self, **params):
        r = super(Host, self).config(**params)

        self.defaultIntf().rename("eth0")

        for off in ["rx", "tx", "sg"]:
            cmd = "/sbin/ethtool --offload eth0 %s off" % off
            self.cmd(cmd)

        # disable IPv6
        self.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")

        return r

    def describe(self):
        print "**********"
        print self.name
        print "default interface: %s\t%s\t%s" %(
            self.defaultIntf().name,
            self.defaultIntf().IP(),
            self.defaultIntf().MAC()
        )
        print "**********"
        
class P4Switch(Switch):
    """P4 virtual switch"""
    device_id = 0

    def __init__( self, name, sw_path = None, json_path = None,
                  thrift_port = None,
                  pcap_dump = False,
                  verbose = False,
                  device_id = None,
                  **kwargs ):
        Switch.__init__( self, name, **kwargs )
        assert(sw_path)
        assert(json_path)
        self.sw_path = sw_path
        self.json_path = json_path
        self.verbose = verbose
        logfile = '/tmp/p4s.%s.log' % self.name
        self.output = open(logfile, 'w')
        self.thrift_port = thrift_port
        self.pcap_dump = pcap_dump
        if device_id is not None:
            self.device_id = device_id
            P4Switch.device_id = max(P4Switch.device_id, device_id)
        else:
            self.device_id = P4Switch.device_id
            P4Switch.device_id += 1
        self.nanomsg = "ipc:///tmp/bm-%d-log.ipc" % self.device_id

    @classmethod
    def setup( cls ):
        pass

    def start( self, controllers ):
        "Start up a new P4 switch"
        print "Starting P4 switch", self.name
        args = [self.sw_path]
        # args.extend( ['--name', self.name] )
        # args.extend( ['--dpid', self.dpid] )
        for port, intf in self.intfs.items():
            if not intf.IP():
                args.extend( ['-i', str(port) + "@" + intf.name] )
        if self.pcap_dump:
            args.append("--pcap")
            # args.append("--useFiles")
        if self.thrift_port:
            args.extend( ['--thrift-port', str(self.thrift_port)] )
        if self.nanomsg:
            args.extend( ['--nanolog', self.nanomsg] )
        args.extend( ['--device-id', str(self.device_id)] )
        P4Switch.device_id += 1
        args.append(self.json_path)

        logfile = '/tmp/p4s.%s.log' % self.name

        print ' '.join(args)

        self.cmd( ' '.join(args) + ' >' + logfile + ' 2>&1 &' )
        # self.cmd( ' '.join(args) + ' > /dev/null 2>&1 &' )

        print "switch has been started"

    def stop( self ):
        "Terminate IVS switch."
        self.output.flush()
        self.cmd( 'kill %' + self.sw_path )
        self.cmd( 'wait' )
        self.deleteIntfs()

    def attach( self, intf ):
        "Connect a data port"
        assert(0)

    def detach( self, intf ):
        "Disconnect a data port"
        assert(0)
