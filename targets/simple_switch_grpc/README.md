# SimpleSwitchGrpc

This is an alternative version of the simple_switch target, which does not use
the Thrift runtime server for table programming. Instead it starts a gRPC server
which implements
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
