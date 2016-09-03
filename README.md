# BEHAVIORAL MODEL REPOSITORY

[![Build Status](https://travis-ci.org/p4lang/behavioral-model.svg?branch=master)](https://travis-ci.org/p4lang/behavioral-model)

This is the second version of the P4 software switch (aka behavioral model),
nicknamed bmv2. It is meant to replace the original version, p4c-behavioral, in
the long run, although we do not have feature equivalence yet. Unlike
p4c-behavioral, this new version is static (i.e. we do not need to auto-generate
new code and recompile every time a modification is done to the P4 program) and
written in C++11. For information on why we decided to write a new version of
the behavioral model, please look at the FAQ below.

## Dependencies

On Ubuntu 14.04, the following packages are required:

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

You also need to install [thrift](https://github.com/apache/thrift) and
[nanomsg](http://download.nanomsg.org/nanomsg-0.5-beta.tar.gz) from source. Feel
free to use the install scripts under travis/.

To use the CLI, you will need to install the
[nnpy](https://github.com/nanomsg/nnpy) Python package. Feel free to use
travis/install-nnpy.sh

To make your life easier, we provide the *install_deps.sh* script, which will
install all the dependencies needed on Ubuntu 14.04.

Our Travis regression tests run on Ubuntu 12.04. Look at .travis.yml for more
information on the Ubuntu 12.04 dependencies.

## Building the code

    1. ./autogen.sh
    2. ./configure
    3. make

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

To run your own P4 programs in bmv2, you first need to transform the P4 code
into a json representation which can be consumed by the software switch. This
representation will tell bmv2 which tables to initialize, how to configure the
parser, ... It is produced by the [p4c-bm](https://github.com/p4lang/p4c-bm)
tool. Please take a look at the
[README](https://github.com/p4lang/p4c-bm/blob/master/README.rst) for this repo
to find out how to install it. Once this is done, you can obtain the json file
as follows:

    p4c-bm --json <path to JSON file> <path to P4 file>

The json file can now be 'fed' to bmv2. Assuming you are using the
*simple_switch* target:

    sudo ./simple_switch -i 0@<iface0> -i 1@<iface1> <path to JSON file>

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

## FAQ

### Why did we need bmv2 ?

- The new C++ code is not auto-generated for each P4 program. This means that it
  becomes very easy and very fast to change your P4 program and test it
  again. The whole P4 development process becomes more efficient. Every time you
  change your P4 program, you simply need to produce the json for it using
  p4c-bm and feed it to the bmv2 executable.
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
  programming your device (parser, macth-action pipeline, deparser) with P4, you
  can use bmv2 to reproduce the behavior of your device.

### How do program my own target / switch architecture using bmv2 ?

You can take a look at the `targets/ directory` first. We have also started
writing some doxygen documentation specifically targetted at programmers who
want to implement their own switch model using the bmv2 building blocks. You can
generate this documentation yourself (if you have doxygen installed) by running
`doxygen Doxyfile`. The output can be found under the `doxygen-out`
directory. You can also browse this documentation [online]
(http://104.236.137.35/).

### What else is new in bmv2 ?

- Arithmetic is now possible on arbitrarily wide fields (no more limited to <=
  32-bit fields) and **variable-length fields are now supported**.
- We finally have unit tests!
- While it is still incomplete, we provide a convenient 'event-logger' built on
  top of nanomsg. Every time a 'significant' event happens (e.g. table hit,
  parser transition,...) a message is broadcast on a nanomsg channel and any
  client can consume it.

### Are all features supported yet ?

At this time, we believe that all P4 features have been implemented. The last
feature to have been implemented is *parse value sets*, which was scarcely used
in bmv1.

If you find more missing features or if you would like to request that a
specific feature be added, please send us an email (p4-dev@p4.org) or submit an
issue with the appropriate label on
[Github](https://github.com/p4lang/behavioral-model/issues). Do not hesitate to
contribute code yourself!

### How do I signal a bug ?

Please submit an issue with the appropriate label on
[Github](https://github.com/p4lang/behavioral-model/issues).

### How can I contribute ?

You can fork the repo and submit a pull request in Github. For more information
send us an email (p4-dev@p4.org).

All developers must sign the [P4.org](http://p4.org) CLA and return it to
(membership@p4.org) before making contributions. The CLA is available
[here](http://p4.org/wp-content/uploads/2015/07/P4_Language_Consortium_Membership_Agreement.pdf).

Any contribution to the C++ core code (in particular the [bm_sim]
(src/bm_sim) module) must respect the coding guidelines. We rely heavily on
the [Google C++ Style Guide]
(https://google.github.io/styleguide/cppguide.html), with some differences
listed in this repository's [wiki]
(https://github.com/p4lang/behavioral-model/wiki/Coding-guidelines). Every
submitted pull request will go through our Travis tests, which include running
`cpplint.py` to ensure correct style and formatting.
