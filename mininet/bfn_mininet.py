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

class BFNHost(Host):
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
        
class BFNSwitch(Switch):
    """BFN virtual switch"""
    listenerPort = 11111
    thriftPort = 22222

    def __init__( self, name, sw_path = "", json_path = "",
                  thrift_port = None,
                  pcap_dump = False,
                  verbose = False, **kwargs ):
        Switch.__init__( self, name, **kwargs )
        self.sw_path = sw_path
        self.json_path = json_path
        self.verbose = verbose
        logfile = '/tmp/bfns.%s.log' % self.name
        self.output = open(logfile, 'w')
        self.thrift_port = thrift_port
        self.pcap_dump = pcap_dump

    @classmethod
    def setup( cls ):
        pass

    def start( self, controllers ):
        "Start up a new BFN switch"
        print "Starting BFN switch", self.name
        args = [self.sw_path]
        # args.extend( ['--name', self.name] )
        # args.extend( ['--dpid', self.dpid] )
        for intf in self.intfs.values():
            if not intf.IP():
                args.extend( ['-i', intf.name] )
        if self.pcap_dump:
            args.append("--pcap")
        args.append(self.json_path)
        # if self.thrift_port:
        #     thrift_port = self.thrift_port
        # else:
        #     thrift_port =  self.thriftPort
        #     self.thriftPort += 1

        logfile = '/tmp/bfns.%s.log' % self.name

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
        print "Connecting data port", intf, "to switch", self.name
        self.cmd( 'bfns-ctl', 'add-port', '--datapath', self.name, intf )

    def detach( self, intf ):
        "Disconnect a data port"
        self.cmd( 'bfns-ctl', 'del-port', '--datapath', self.name, intf )

    def dpctl( self, *args ):
        "Run dpctl command"
        pass
