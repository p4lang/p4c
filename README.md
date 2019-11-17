# BEHAVIORAL MODEL (bmv2)

[![Build Status](https://travis-ci.org/p4lang/behavioral-model.svg?branch=master)](https://travis-ci.org/p4lang/behavioral-model)

This is the second version of the reference P4 software switch, nicknamed bmv2
(for behavioral model version 2). The software switch is written in C++11. It
takes as input a JSON file generated from your P4 program by a [P4
compiler](https://github.com/p4lang/p4c) and interprets it to implement the
packet-processing behavior specified by that P4 program.

This repository contains code for several variations of the behavioral
model, e.g. `simple_switch`, `simple_switch_grpc`, `psa_switch`, etc.
See [here](targets/README.md) for more details on the differences
between these.

**bmv2 is not meant to be a production-grade software switch**. It is meant to
be used as a tool for developing, testing and debugging P4 data planes and
control plane software written for them. As such, the performance of bmv2 - in
terms of throughput and latency - is significantly less than that of a
production-grade software switch like [Open
vSwitch](https://www.openvswitch.org/). For more information about the
performance of bmv2, refer to this [document](docs/performance.md).

## Dependencies

On Ubuntu 16.04, the following packages are required:

- automake
- cmake
- libjudy-dev
- libgmp-dev
- libpcap-dev
- libboost-dev
- libboost-test-dev
- libboost-program-options-dev
- libboost-system-dev
- libboost-filesystem-dev
- libboost-thread-dev
- libevent-dev
- libtool
- flex
- bison
- pkg-config
- g++
- libssl-dev

You also need to install the following from source. Feel free to use the
install scripts under travis/.

- [thrift 0.9.2](https://github.com/apache/thrift/releases/tag/0.9.2) or later
  (tested up to 0.12.1)
- [nanomsg 1.0.0](https://github.com/nanomsg/nanomsg/releases/tag/1.0.0) or
  later

To use the CLI, you will need to install the
[nnpy](https://github.com/nanomsg/nnpy) Python package. Feel free to use
travis/install-nnpy.sh

To make your life easier, we provide the *install_deps.sh* script, which will
install all the dependencies needed on Ubuntu 14.04.

Our Travis regression tests now run on Ubuntu 14.04.

On MacOS you can use the tools/macos/bootstrap_mac.sh script to
install all the above dependencies using homebrew. Note that in order
to compile the code you need [XCode 8](https://itunes.apple.com/us/app/xcode/id497799835?mt=12)
or later.

## Building the code

    1. ./autogen.sh
    2. ./configure
    3. make
    4. [sudo] make install  # if you need to install bmv2

In addition, on Linux, you may have to run `sudo ldconfig` after installing
bmv2, to refresh the shared library cache.

Debug logging is enabled by default. If you want to disable it for performance
reasons, you can pass `--disable-logging-macros` to the `configure` script.

In 'debug mode', you probably want to disable compiler optimization and enable
symbols in the binary:

    ./configure 'CXXFLAGS=-O0 -g'

The new bmv2 debugger can be enabled by passing `--enable-debugger` to
`configure`.

## Running the tests

To run the unit tests, simply do:

    make check

**If you get a nanomsg error when running the tests (make check), try running
  them as sudo**

## Running your P4 program

To run your own P4 programs in bmv2, you first need to compile the P4 code
into a json representation which can be consumed by the software switch. This
representation will tell bmv2 which tables to initialize, how to configure the
parser, ...

There are currently 2 P4 compilers available for bmv2 on p4lang:
 * [p4c](https://github.com/p4lang/p4c) includes a bmv2 backend and is the
   recommended compiler to use, as it supports both P4_14 and P4_16
   programs. Refer to the
   [README](https://github.com/p4lang/p4c/blob/master/README.md) for information
   on how to install and use p4c. At the moment, the bmv2 p4c backend supports
   the v1model architecture, with some tentative support for the PSA
   architecture. P4_16 programs written for v1model can be executed with the
   `simple_switch` binary, while programs written for PSA can be executed with
   the `psa_switch` binary. See [here](targets/README.md) for more details on
   the differences between these.
 * [p4c-bm](https://github.com/p4lang/p4c-bm) is the legacy compiler for bmv2
   (no longer actively maintained) and only supports P4_14 programs.

Assuming you have installed the p4c compiler, you can obtain the json file for a
P4_16 v1model program as follows:

    p4c --target bmv2 --arch v1model --std p4-16 <prog>.p4

This will create a `<prog>.json` output file which can now be 'fed' to the bmv2
`simple_switch` binary:

    sudo ./simple_switch -i 0@<iface0> -i 1@<iface1> <prog>.json

In this example \<iface0\> and \<iface1\> are the interfaces which are bound to
the switch (as ports 0 and 1).

## Using the CLI to populate tables...

The CLI code can be found at [tools/runtime_CLI.py](tools/runtime_CLI.py). It
can be used like this:

    ./runtime_CLI.py --thrift-port 9090

The CLI connect to the Thrift RPC server running in each switch process. 9090 is
the default value but of course if you are running several devices on your
machine, you will need to provide a different port for each. One CLI instance
can only connect to one switch device.

The CLI is realized using the Python's cmd module and supports
auto-completion. If you inspect the code, you will see that the list of
supported commands. This list includes:

    - table_set_default <table name> <action name> <action parameters>
    - table_add <table name> <action name> <match fields> => <action parameters> [priority]
    - table_delete <table name> <entry handle>

The CLI include commands to program the multicast engine. Because we provide 2
different engines (*SimplePre* and *SimplePreLAG*), you have to specify which
one your target is using when starting the CLI, using the *--pre*
option. Accepted values are: *None*, *SimplePre* (default value) and
*SimplePreLAG*. The *l2_switch* target uses the *SimplePre* engine, while the
*simple_switch* target uses the *SimplePreLAG* engine.

You can take a look at the *commands.txt* file for
[*l2_switch*](targets/l2_switch/commands.txt) and
[*simple_router*](targets/simple_router/commands.txt) to see how the CLI can be
used.

## Using the debugger

To enable the debugger, make sure that you passed the `--enable-debugger` flag
to `configure`. You will also need to use the `--debugger` command line flag
when starting the switch.

Use [tools/p4dbg.py](tools/p4dbg.py) as follows when the switch is running to
attach the debugger to the switch:

    sudo ./p4dbg.py [--thrift-port <port>]

## Displaying the event logging messages

To enable event logging when starting your switch, use the *--nanolog* command
line option. For example, to use the ipc address *ipc:///tmp/bm-log.ipc*:

    sudo ./simple_switch -i 0@<iface0> -i 1@<iface1> --nanolog ipc:///tmp/bm-log.ipc <path to JSON file>

Use [tools/nanomsg_client.py](tools/nanomsg_client.py) as follows when the
switch is running:

    sudo ./nanomsg_client.py [--thrift-port <port>]

The script will display events of significance (table hits / misses, parser
transitions, ...) for each packet.

## Loading shared objects dynamically

Some targets (simple_switch and simple_switch_grpc) let the user load shared
libraries dynamically at runtime. This is done by using the target-specific
command-line option `--load-modules`, which takes as a parameter a
comma-separated list of shared objects. This functionality is currently only
available on systems where `dlopen` is available. Make sure that the shared
objects are visible by the dynamic loader (e.g. by setting `LD_LIBRARY_PATH`
appropriately on Linux). You can control whether this feature is available by
using `--enable-modules` / `--disable-modules` when configuring bmv2. By
default, this feature is enabled when `dlopen` is available.

## Integrating with Mininet

We will provide more information in a separate document. However you can test
the Mininet integration right away using our *simple_router* target.

In a first terminal, type the following:

    - cd mininet
    - sudo python 1sw_demo.py --behavioral-exe ../targets/simple_router/simple_router --json ../targets/simple_router/simple_router.json

Then in a second terminal:

    - cd targets/simple_router
    - ./runtime_CLI < commands.txt

Now the switch is running and the tables have been populated. You can run
*pingall* in Mininet or start a TCP flow with iperf between hosts *h1* and *h2*.

When running a P4 program with *simple_switch* (instead of *simple_router* in
the above example), just provide the appropriate `simple_switch` binary to
`1sw_demo.py` with `--behavioral-exe`.

## FAQ

### Why is throughput so low / why are so many packets dropped?

bmv2 is not meant to be a production-grade software switch. For more information
on bmv2 performance, please refer to this [document](docs/performance.md).

### Why did we replace p4c-behavioral with bmv2?

- The new C++ code is not auto-generated for each P4 program. This means that it
  becomes very easy and very fast to change your P4 program and test it
  again. The whole P4 development process becomes more efficient. Every time you
  change your P4 program, you simply need to produce the json for it using
  the p4c compiler and feed it to the bmv2 executable.
- Because the bmv2 code is not auto-generated, we hope it is easier to
  understand. We hope this will encourage the community to contribute even more
  to the P4 software switch.
- Using the auto-generated PD library (which of course still needs to be
  recompiled for each P4 program) is now optional. We provide an intuitive CLI
  which can be used to program the runtime behavior of each switch device.
- The new code is target independent. While the original p4c-behavioral assumed
  a fixed abstract switch model with 2 pipelines (ingress and egress), bmv2
  makes no such assumption and can be used to represent many switch
  architectures. Three different -although similar- such architectures can be
  found in the targets/ directory. If you are a networking company interested in
  programming your device (parser, match-action pipeline, deparser) with P4, you
  can use bmv2 to reproduce the behavior of your device.

### How do program my own target / switch architecture using bmv2?

You can take a look at the `targets/ directory` first. We have also started
writing some doxygen documentation specifically targetted at programmers who
want to implement their own switch model using the bmv2 building blocks. You can
generate this documentation yourself (if you have doxygen installed) by running
`doxygen Doxyfile`. The output can be found under the `doxygen-out`
directory. You can also browse this documentation
[online](http://bmv2.org).

### What else is new in bmv2?

- Arithmetic is now possible on arbitrarily wide fields (no more limited to <=
  32-bit fields) and **variable-length fields are now supported**.
- We finally have unit tests!
- While it is still incomplete, we provide a convenient 'event-logger' built on
  top of nanomsg. Every time a 'significant' event happens (e.g. table hit,
  parser transition,...) a message is broadcast on a nanomsg channel and any
  client can consume it.

### Are all features supported yet?

At this time, we are aware of the following unsupported P4_14 features:
- direct registers

If you find more missing features or if you would like to request that a
specific feature be added, please send us an email (p4-dev@lists.p4.org) or
submit an issue with the appropriate label on
[Github](https://github.com/p4lang/behavioral-model/issues). Do not hesitate to
contribute code yourself!

### How do I signal a bug?

Please submit an issue with the appropriate label on
[Github](https://github.com/p4lang/behavioral-model/issues).

### How can I contribute?

See [CONTRIBUTING.md](CONTRIBUTING.md).
