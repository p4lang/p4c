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

    def __init__( self, name, sw_path = "dc_full",
                  thrift_port = None,
                  pcap_dump = False,
                  verbose = False, **kwargs ):
        Switch.__init__( self, name, **kwargs )
        self.sw_path = sw_path
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
        # args.extend( ['--listener', '127.0.0.1:%d' % self.listenerPort] )
        # self.listenerPort += 1
        # # FIXME
        # if self.thrift_port:
        #     thrift_port = self.thrift_port
        # else:
        #     thrift_port =  self.thriftPort
        #     self.thriftPort += 1
        # args.extend( ['--pd-server', '127.0.0.1:%d' % thrift_port] )
        # args.append( '--no-cli' )
        # if not self.pcap_dump:
        #     args.append( '--no-cli' )
        # args.append( self.opts )

        logfile = '/tmp/bfns.%s.log' % self.name

        print ' '.join(args)

        # self.cmd( ' '.join(args) + ' >' + logfile + ' 2>&1 &' )
        self.cmd( ' '.join(args) + ' > /dev/null 2>&1 &' )

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
