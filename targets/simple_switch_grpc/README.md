# SimpleSwitchGrpc

This is an alternative version of the simple_switch target, which does not use
the Thrift runtime server for table programming (unless required, see below).
Instead it starts a gRPC server which implements
[p4untime.proto](https://github.com/p4lang/PI/blob/master/proto/p4/p4runtime.proto).

To compile, make sure you build the bmv2 code first, then run `./autogen.sh`,
`./configure --with-pi` and `make`.

## Tentative gNMI support with sysrepo

We are working on supporting gNMI and OpenConfig YANG models as part of the P4
Runtime server. We are using [sysrepo](https://github.com/sysrepo/sysrepo) as
our YANG configuration data store and operational state manager. See [this
README](https://github.com/p4lang/PI/blob/master/proto/README.md) for more
information on how to try it out. After installing sysrepo, building and
installing the PI project with sysrepo support enabled, you will need to
configure simple_switch_grpc with `--with-sysrepo` and build it again.

This directory includes a Python script, [gnmi_sub_once.py](gnmi_sub_once.py),
which you can run to issue a gNMI ONCE subscription request to the P4 Runtime
gRPC server running in the simple_switch_grpc process.

### OpenConfig support

These are the [OpenConfig YANG models](https://github.com/openconfig/public) we
are trying to support. For each model you will see a list of YANG leaves with
the level of support. "Yes" means support has been added, with "(stub)"
indicating that setting the leaf value has no effect on bmv2
forwarding. "Planned" means we are planning to add some form of support
soon. "No" means there is no immediate plan to support the node.

#### openconfig-interfaces (`interfaces/interface`)

| Node | Support |
| ---- | ------- |
| `config/name` | Yes |
| `config/type` | Yes (`iana-if-type:ethernetCsmacd` only) |
| `config/mtu` | Yes (stub) |
| `config/description` | Yes |
| `config/enabled` | Yes |
| `state/name` | Yes |
| `state/type` | Yes |
| `state/mtu` | Yes (stub) |
| `state/description` | Yes |
| `state/enabled` | Yes |
| `state/ifindex` | Yes |
| `state/admin-status` | Yes |
| `state/oper-status` | Yes |
| `state/last-change` | No |
| `state/counters` | Planned |
| `subinterfaces` | No |

#### openconfig-if-ethernet (`ethernet`)

This model augments `interfaces/interface`.

| Node | Support |
| ---- | ------- |
| `config/mac-address` | Yes (stub) |
| `config/auto-negotiate` | Yes (stub) |
| `config/duplex-mode` | Yes (stub) |
| `config/port-speed` | Yes (stub) |
| `config/enable-flow-control` | Yes (stub) |
| `state/mac-address` | Yes (stub) |
| `state/auto-negotiate` | Yes (stub) |
| `state/duplex-mode` | Yes (stub) |
| `state/port-speed` | Yes (stub) |
| `state/enable-flow-control` | Yes (stub) |
| `state/hw-mac-address` | Yes (stub) |
| `state/negotiated-duplex-mode` | Yes (stub) |
| `state/negotiated-port-speed` | Yes (stub) |
| `state/counters` | Planned |

#### openconfig-platform

TBD

## Enabling the Thrift server

**Experimental feature, use at your own risk!**

Enabling the Thrift runtime server (as in `simple_switch`) along the gRPC one,
can be useful to execute the BMv2 debugger and CLI, which otherwise would not
work on this target.

The Thrift server can be enabled by passing the `--with-thrift` argument to
`configure`.

**Warning:** because the capabilities of the Thrift server overlap with those of
the gRPC/P4 Runtime one (e.g. a table management API is exposed by both), there
could be inconsistency issues when using both servers to write state to the
switch. For example, if one tries to insert a table entry using Thrift, the same
cannot be read using P4 Runtime. In general, to avoid such issues, we suggest to
use the Thrift server only to read state, or to write state that is not managed
by P4 Runtime.
