This is an alternative version of the simple_switch target, which does not use
the Thrift runtime server for table programming. Instead it starts a gRPC server
which implements p4untime.proto
(https://github.com/p4lang/PI/blob/master/proto/p4/p4runtime.proto).

To compile, make sure you build the bmv2 code first, then run `./autogen.sh`,
`./configure --with-pi` and `make`.
