# Performance of bmv2

**bmv2 is not meant to be a production-grade software switch**. It is meant to
be used as a tool for developing, testing and debugging P4 data planes and
control plane software written for them. As such, the performance of bmv2 - in
terms of throughput and latency - is significantly less than that of a
production-grade software switch like [Open
vSwitch](https://www.openvswitch.org/). Performance is also very inconsistent
and depends on a [variety of factors](#what-impacts-performance).

## What impacts performance

 * which P4 program you are running: the simpler the program (i.e. the fewer the
   number of parsed / deparsed headers, the fewer the number of match-action
   tables, etc.), the higher the throughput and the lower the laytency.
 * what P4 compiler you used to compile the P4 program to a BMv2 JSON file with:
   the legacy [p4c-bm](https://github.com/p4lang/p4c-bm) compiler tends to
   produce "faster" JSON files than the reference
   [p4c](https://github.com/p4lang/p4c) compiler, with simpler arithmetic
   expressions and fewer field copies. Note however that p4c-bm is no longer
   maintained and only supports P4_14 programs.
 * which version of bmv2 code you are running.
 * which flags were used to build bmv2: this can have a massive impact. Please
   refer to this [section](#which-build-flags-to-use) for information on which
   flags to use to achieve the highest possible throughput.
 * what command line options you give to `simple_switch` when starting it.
 * how many table entries you install in the tables being applied while
   processing packets: this does not really apply to exact match & LPM tables,
   but can significantly impact throughput when your P4 program contains ternary
   match tables.
 * the performance of your hardware: how many CPU cores, how much CPU cache,
   etc.
 * the traffic pattern you send:
   - if your program includes a ternary match, and all packets belong to the
     same "flow" (i.e. hit the same table entry), you may get much better
     performance depending on the number of entries in the table, as the ternary
     match lookup implementation uses a small flow cache.
   - similarly, successive packets hitting the same table entries may increase
     the likelihood of CPU cache hits.
   - `simple_switch` can take advantage of a larger number of CPU cores when
     packets come in on different input ports & go out of different output
     ports.
 * whether you running on a baremetal Linux machine or a Linux VM.
 * when running bmv2 in a Linux VM, which host OS and which hypervisor is being
   used. We have observed some significant performance degradation with some
   hypervisors on macOS hosts; in such cases the CPU is under-utilized which
   indicates that there may be some I/O issue causing poor throughput.

## Which build flags to use

To get the best performance (highest throughput and lowest latency), do a fresh
clone of bmv2 and run `./configure` with the following flags:

```bash
./configure 'CXXFLAGS=-g -O3' 'CFLAGS=-g -O3' --disable-logging-macros --disable-elogger
```

## Running the benchmark

We provide a benchmark based on [Mininet](http://mininet.org/). To run it, build
bmv2 & `simple_switch`, and make sure that Mininet, iperf and ethtool are
installed on your machine. Then run the following commands:

```bash
cd mininet
python stress_test_ipv4.py
```

The Python script creates a Mininet topology with 2 hosts connected by one
instance of `simple_switch`. `simple_switch` is running the
[`simple_router.p4`](/mininet/simple_router.p4) P4 program. It is a P4_14
program which has been pre-compiled for you using the legacy p4c-bm
compiler. After creating the topology, the necessary table entries are installed
to ensure L3 connectivity between the 2 hosts. An iperf server is then created
on one host, while the other runs an iperf client. Once the test completes, you
should see some logs similar to these ones:

```
...
Ready !
*** Ping: testing ping reachability
h1 -> h2
h2 -> h1
*** Results: 0% dropped (2/2 received)
Running iperf measurement 1 of 5
1056 Mbps
Running iperf measurement 2 of 5
1056 Mbps
Running iperf measurement 3 of 5
1047 Mbps
Running iperf measurement 4 of 5
1042 Mbps
Running iperf measurement 5 of 5
1041 Mbps
Median throughput is 1047 Mbps
```

## Suggested setup to run the benchmark consistently

The above results (`Median throughput is 1047 Mbps`) were obtained by running
the benchmark on an AWS EC2 instance (`c4.2xlarge`). If you have an AWS account
and are willing to spend a few cents, you can create the same instance (select
the Ubuntu 18.04 image), and run the following commands after logging into your
instance:

```bash
git clone https://github.com/p4lang/behavioral-model.git bmv2
cd bmv2
./install_deps.sh
./autogen.sh
./configure 'CXXFLAGS=-g -O3' 'CFLAGS=-g -O3' --disable-logging-macros --disable-elogger
make -j8
cd mininet
python stress_test_ipv4.py
```

You should get similar results as above: around 1Gbps, or 80,000pps.

*Don't forget to delete your instance when you no longer need it, or you will
 rack up AWS charges.*

## Packet drops when sending packets at slow rates

Even when sending packets at a slow rate (e.g. when sending UDP traffic with
iperf), you may observe packet drops at the sender's interface. This can be
explained by 1) burstiness of the traffic source, 2) variability in how fast
bmv2 can process individual packets. There may be ways to mitigate this
phenomenon, e.g. by adjusting bmv2 internal queue sizes.
