# SimpleSwitchGrpc - a version of SimpleSwitch with P4Runtime support

This is an alternative version of the simple_switch target, which does not use
the Thrift runtime server for table programming (unless required, see below).
Instead it starts a gRPC server which implements
[p4untime.proto](https://github.com/p4lang/PI/blob/master/proto/p4/p4runtime.proto).

Make sure you read this README and [FAQ](#faq) before asking a question on
Github or the p4-dev mailing list.

## Dependencies and build steps

 1. Follow instructions in the [PI
    README](https://github.com/p4lang/PI#dependencies) to install required
    dependencies for the `--with-profo` configure flag - as well as for the
    `--with-sysrepo` configure flag if desired.
 1. Configure, build and install PI:
    ```
    ./autogen.sh
    ./configure --with-proto --without-internal-rpc --without-cli --without-bmv2 [--with-sysrepo]
    make
    [sudo] make install
    ```
 1. Configure and build the bmv2 code; from the root of the repository:
    ```
    ./autogen.sh
    ./configure --with-pi [--without-thrift] [--without-nanomsg]
    make
    [sudo] make install  # if desired
    ```
 1. Configure and build the simple_switch_grpc code; from this directory:
    ```
    ./autogen.sh
    ./configure [--with-sysrepo]
    make
    [sudo] make install  # if desired
    ```

## Running simple_switch_grpc

For information on how to run simple_switch_grpc, use `simple_switch_grpc
--help`. In particular, take a look at the target-specific command-line options
at the end of the help message. These command-line options are specific to
simple_switch_grpc (to be precise, some of them are shared with the
simple_switch binary). When invoking simple_switch_grpc, these options will need
to be separated from standard bmv2 options using `--`.

Most users will invoke simple_switch_grpc as follows:
```
simple_switch_grpc --no-p4 \
    -i <PORT1>@<IFACE1> -i <PORT2>@<IFACE2> <more ports> \
    -- --grpc-server-addr <IP>:<TCP PORT> --cpu-port <CPU_PORT>
```

* `--no-p4` is a standard bmv2 command-line option. It means that no JSON file
  is provided when starting simple_switch_grpc and bmv2 will not initially be
  able to process packets (i.e. all packets will be dropped). You will need to
  provide a P4 "pipeline" (including a JSON file and a P4Info message) with the
  P4Runtime `SetForwardingPipelineConfig` RPC. **This is the recommended way to
  use simple_switch_grpc.**
* `--grpc-server-addr` is used to provide a socket address on which the
  P4Runtime server will be run (e.g. localhost:1234, 192.168.1.1:31416,
  [::1]:27182, ...). It will default to 0.0.0.0:50051.
* `--cpu-port` is used to provide a CPU port number and enable P4Runtime
  packet-in / packet-out support. **We recommend that you use this option** as
  otherwise you will not be able to receive / send packets using the P4Runtime
  `StreamChannel` bi-directional stream. Packets sent to this port will be sent
  to the P4Runtime controller, while packets received from the controller will
  be injected in the pipeline on this port. When using standard v1model.p4, the
  value must fit within 9 bits (as "port" metadata fields have type `bit<9>`). 0
  is not a valid value. Do not use 511 as it is reserved for the "drop port" by
  default. If you need to use 511 for the CPU port, you will need to provide a
  new drop port value using the `--drop-port` command-line option.

If you are not familiar with P4Runtime, please refer to the [latest
released specification](https://p4.org/specs/).

## Tentative gNMI support with sysrepo

We are working on supporting gNMI and OpenConfig YANG models as part of the
P4Runtime server. We are using [sysrepo](https://github.com/sysrepo/sysrepo) as
our YANG configuration data store and operational state manager. See [this
README](https://github.com/p4lang/PI/blob/master/proto/README.md) for more
information on how to try it out. After installing sysrepo, building and
installing the PI project with sysrepo support enabled, you will need to
configure simple_switch_grpc with `--with-sysrepo` and build it again.

This directory includes a Python script, [gnmi_sub_once.py](gnmi_sub_once.py),
which you can run to issue a gNMI ONCE subscription request to the P4Runtime
gRPC server running in the simple_switch_grpc process.

### OpenConfig support

These are the [OpenConfig YANG models](https://github.com/openconfig/public) we
are trying to support. For each model you will see a list of YANG leaves with
the level of support. "Yes" means support has been added, with "(stub)"
indicating that setting the leaf value has no effect on bmv2
forwarding. "Planned" means we are planning to add some form of support
soon. "No" means there is no immediate plan to support the node.

#### openconfig-interfaces (`interfaces/interface`)

For simple_switch_grpc, we require that the interface name follow this pattern:
`<device_id>-<port_num>@<interface>`.

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
| `state/counters` | Yes |
| `subinterfaces` | No |

About `state/counters`:
  * error counters (e.g. `in-discards`) are always set to 0.
  * all packets sent and received are counted as `unicast-pkts`;
  `broadcast-pkts` and `multicast-pkts` will therefore always be 0.
  * counters are cleared when the interface is deleted.

`state/last-change` and `state/counters/carrier-transitions` may not be very
accurate as their implementation is somewhat naive.

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
the gRPC/P4Runtime one (e.g. a table management API is exposed by both), there
could be inconsistency issues when using both servers to write state to the
switch. For example, if one tries to insert a table entry using Thrift, the same
cannot be read using P4Runtime. In general, to avoid such issues, we suggest to
use the Thrift server only to read state, or to write state that is not managed
by P4Runtime.

## FAQ

#### I am using P4Runtime to program table entries, but I'd like to be able to use simple_switch_CLI to check the contents of my tables.

Refer to [Enabling the Thrift server](#enabling-the-thrift-server) above. Use
`--with-thrift` when configuring bmv2 and simple_switch_grpc.

#### What's the difference between the [PI folder in this repository](https://github.com/p4lang/behavioral-model/tree/master/PI) and the [PI repository](https://github.com/p4lang/PI)? How do they relate to simple_switch and simple_switch_grpc?

The PI repository includes all the core code for the PI project: common server
code, P4Runtime & P4Info Protobuf IDL definitions, implementation of the
P4Runtime service over the PI C interface, PI C interface definition and common
PI C library code. This "core code" can be used by all targets who choose to
implement P4Runtime using the PI C interface. All these targets have to do is
implement a "few" C functions, such as `_pi_table_entry_add`.

The PI repository also includes [a reference implementation of the PI interface
for bmv2](https://github.com/p4lang/PI/tree/master/targets/bmv2). This
implementation uses the bmv2 Thrift RPC server. In other words, the P4Runtime
RPC calls are mapped to PI C function calls, which in-turn are mapped to bmv2
Thrift RPC calls. This implementation enables controlling multiple bmv2
simple_switch processes - each with its own Thrift RPC server - using a single
instance of the P4Runtime gRPC server running in its own process.

For many people, having a dependency on both Protobuf and Thrift is not very
convenient. For this reason, we have introduced simple_switch_grpc, a
slightly-modified version of simple_switch which includes a P4Runtime gRPC
server. This time, each P4Runtime RPC call is mapped to a PI C function call -
so far, it is the same -, which accesses bmv2 data structures (e.g. for
match-action tables) directly, without any extra IPC. However, because the
P4Runtime server and the bmv2 device are run in the same process
(simple_switch_grpc), you can only have a single bmv2 device for a given
P4Runtime server. The PI C implementation used by simple_switch_grpc, which
directly accesses bmv2 internal data structures, is located in the [PI folder in
this repository](https://github.com/p4lang/behavioral-model/tree/master/PI).

We encourage newcomers to use simple_switch_grpc if possible.
